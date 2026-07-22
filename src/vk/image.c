
#include <vk/image.h>
#include <vk/context.h>
#include <vk/buffer.h>
#include <vk/utils.h>

#include <SDL3/SDL_log.h>
#include <vulkan/vulkan_core.h>

bool vulkan_image_create(vulkan_context *ctx, v2u dimensions, VkFormat format, VkImageUsageFlags usage, VkImageAspectFlags aspect, vulkan_image *out_image)
{
	vulkan_image result = {0};
	
	VkImageCreateInfo image_info = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.extent.width = dimensions.x,
		.extent.height = dimensions.y,
		.extent.depth = 1,
		.mipLevels = 1,
		.arrayLayers = 1,
		.format = format,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.usage = usage,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
	};

	if (vkCreateImage(ctx->device, &image_info, NULL, &result.handle) != VK_SUCCESS)
	{
		SDL_Log("[VULKAN] Failed to create image.");
		return false;
	}

	VkMemoryRequirements memory_requirements;
	vkGetImageMemoryRequirements(ctx->device, result.handle, &memory_requirements);

	VkMemoryAllocateInfo alloc_info = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = memory_requirements.size,
		.memoryTypeIndex = vulkan_memory_type_find(ctx, memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),	
	};

	if (vkAllocateMemory(ctx->device, &alloc_info, NULL, &result.memory) != VK_SUCCESS)
	{
		SDL_Log("[VULKAN] Failed to allocate image memory.");
		return false;
	}

	if (vkBindImageMemory(ctx->device, result.handle, result.memory, 0) != VK_SUCCESS)
	{
		SDL_Log("[VULKAN] Failed to bind image memory.");
		return false;
	}

	VkImageViewCreateInfo view_info = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image = result.handle,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = format,
		.subresourceRange.aspectMask = aspect,
		.subresourceRange.layerCount = 1,
		.subresourceRange.levelCount = 1,	
	};

	if (vkCreateImageView(ctx->device, &view_info, NULL, &result.view) != VK_SUCCESS)
	{
		SDL_Log("[VULKAN] Failed to create image view.");
		return false;
	}

	result.access = VK_ACCESS_NONE;
	result.layout = VK_IMAGE_LAYOUT_UNDEFINED;
	result.aspect = aspect;

	*out_image = result;
	
	return true;
}

void vulkan_image_destroy(vulkan_context *ctx, vulkan_image image)
{
	vkDestroyImageView(ctx->device, image.view, NULL);
	vkDestroyImage(ctx->device, image.handle, NULL);
	vkFreeMemory(ctx->device, image.memory, NULL);
}

bool vulkan_image_transition(vulkan_context *ctx, vulkan_image *image,
                         VkImageLayout old_layout, VkImageLayout new_layout,
                         VkAccessFlags src_access, VkAccessFlags dst_access,
                         VkPipelineStageFlags src_stage,
                         VkPipelineStageFlags dst_stage,
                         VkImageAspectFlags aspect_mask)
{
    VkCommandPool command_pool;
    VkCommandBuffer command_buffer;

    VkCommandPoolCreateInfo pool_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
        .queueFamilyIndex = ctx->graphics_queue.index,
    };

    if (vkCreateCommandPool(ctx->device, &pool_info, NULL, &command_pool) != VK_SUCCESS)
    {
    	SDL_Log("[VULKAN] Failed to create command pool to transition image.");
        return false;
    }

    VkCommandBufferAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
        .commandPool = command_pool,
    };

    if (vkAllocateCommandBuffers(ctx->device, &alloc_info, &command_buffer) != VK_SUCCESS)
    {
    	SDL_Log("[VULKAN] Failed to allocate command buffer to transition image.");
        goto error;
    }

    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };

    if (vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS)
    {
    	SDL_Log("[VULKAN] Failed to begin recording into command buffer to transition image.");
        goto error;
    }

    VkImageMemoryBarrier image_barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .oldLayout = old_layout,
        .newLayout = new_layout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image->handle,
        .subresourceRange =
            {
                .aspectMask = aspect_mask,
                .layerCount = 1,
                .levelCount = 1,
            },
        .srcAccessMask = src_access,
        .dstAccessMask = dst_access,
    };

    vkCmdPipelineBarrier(command_buffer, src_stage, dst_stage, 0, 0, 0, 0, 0, 1, &image_barrier);

    if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS) {
    	SDL_Log("[VULKAN] Failed to end recording into command buffer to transition image");
        goto error;
    }

    VkSubmitInfo sub_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &command_buffer,
    };

    if (vkQueueSubmit(ctx->graphics_queue.handle, 1, &sub_info, VK_NULL_HANDLE) != VK_SUCCESS)
    {
    	SDL_Log("[VULKAN] Failed to submit command buffer to transition image.");
        goto error;
    }

    if (vkQueueWaitIdle(ctx->graphics_queue.handle) != VK_SUCCESS)
    {
    	SDL_Log("[VULKAN] Failed to wait for queue to transition image.");
        goto error;
    }

    vkDestroyCommandPool(ctx->device, command_pool, NULL);

	image->access = dst_access;
	image->layout = new_layout;
	image->aspect = aspect_mask;

    return true;

error:

    vkDestroyCommandPool(ctx->device, command_pool, NULL);
    return false;
}

bool vulkan_image_data_upload(vulkan_context *ctx, vulkan_image *image, u32 size, void *data, v2u dimensions, VkImageLayout layout, VkAccessFlags access, VkPipelineStageFlags dst_stage)
{
	vulkan_buffer staging;
	if (!vulkan_buffer_host_visible_create(ctx, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, size, data, &staging))
	{
		SDL_Log("[VULKAN] Failed to create staging buffer when uploading data to image.");
		return false;
	}
	
	VkCommandPool command_pool;
    VkCommandBuffer command_buffer;

    VkCommandPoolCreateInfo pool_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
        .queueFamilyIndex = ctx->graphics_queue.index,
    };

    if (vkCreateCommandPool(ctx->device, &pool_info, NULL, &command_pool) != VK_SUCCESS)
    {
    	SDL_Log("[VULKAN] Failed to create command pool to transition image.");
        return false;
    }

    VkCommandBufferAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
        .commandPool = command_pool,
    };

    if (vkAllocateCommandBuffers(ctx->device, &alloc_info, &command_buffer) != VK_SUCCESS)
    {
    	SDL_Log("[VULKAN] Failed to allocate command buffer to transition image.");
        goto error;
    }

    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };

    if (vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS)
    {
    	SDL_Log("[VULKAN] Failed to begin recording into command buffer to transition image.");
        goto error;
    }

    VkImageMemoryBarrier image_barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .oldLayout = image->layout,
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image->handle,
        .subresourceRange =
            {
                .aspectMask = image->aspect,
                .layerCount = 1,
                .levelCount = 1,
            },
        .srcAccessMask = image->access,
        .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
    };

    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, 0, 0, 0, 1, &image_barrier);

	VkBufferImageCopy copy = {
		.imageSubresource = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.layerCount = 1,
		},
		.imageExtent = {
			.width = dimensions.x,
			.height = dimensions.y,
			.depth = 1,
		},
	};

	vkCmdCopyBufferToImage(command_buffer, staging.handle, image->handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);

    image_barrier = (VkImageMemoryBarrier){
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .newLayout = layout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image->handle,
        .subresourceRange =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .layerCount = 1,
                .levelCount = 1,
            },
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstAccessMask = access,
    };

    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, dst_stage, 0, 0, 0, 0, 0, 1, &image_barrier);

    if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS) {
    	SDL_Log("[VULKAN] Failed to end recording into command buffer to transition image");
        goto error;
    }

    VkSubmitInfo sub_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &command_buffer,
    };

    if (vkQueueSubmit(ctx->graphics_queue.handle, 1, &sub_info, VK_NULL_HANDLE) != VK_SUCCESS)
    {
    	SDL_Log("[VULKAN] Failed to submit command buffer to transition image.");
        goto error;
    }

    if (vkQueueWaitIdle(ctx->graphics_queue.handle) != VK_SUCCESS)
    {
    	SDL_Log("[VULKAN] Failed to wait for queue to transition image.");
        goto error;
    }

    vkDestroyCommandPool(ctx->device, command_pool, NULL);
    vulkan_buffer_destroy(ctx, &staging);

	image->layout = layout;
	image->access = access;
	
	return true;

error:
    vkDestroyCommandPool(ctx->device, command_pool, NULL);
    vulkan_buffer_destroy(ctx, &staging);
    return false;
}

