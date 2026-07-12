
#include <vk/command.h>
#include <vk/context.h>
#include <vk/buffer.h>

#include <SDL3/SDL_log.h>
#include <vulkan/vulkan_core.h>

#include <math/matrix.h>

bool vulkan_command_handler_init(VulkanContext *ctx, VulkanCommandHandler *handler)
{
	assert(ctx);
	assert(handler);

	handler->frame_index = 0;
	handler->accumulated_frame_index = 0;

	VkCommandPoolCreateInfo pool_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = ctx->graphics_queue.index,
	};

	if (vkCreateCommandPool(ctx->device, &pool_info, NULL, &handler->command_pool) != VK_SUCCESS)
	{
		SDL_Log("[VULKAN] Failed to create commnad pool.");
		return false;
	}

	VkSemaphoreCreateInfo semaphore_info = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,	
	};

	VkFenceCreateInfo fence_info = {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT,	
	};

	for (u32 i = 0; i < FRAMES_IN_FLIGHT; i++)
	{
		VkCommandBufferAllocateInfo alloc_info = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = handler->command_pool,
			.commandBufferCount = 1,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,	
		};

		if (vkAllocateCommandBuffers(ctx->device, &alloc_info, &handler->frame_data[i].command_buffer) != VK_SUCCESS)
		{
			SDL_Log("[VULKAN] Failed to allocate command buffer.");
			return false;			
		}

		if (vkCreateSemaphore(ctx->device, &semaphore_info, NULL, &handler->frame_data[i].image_available) != VK_SUCCESS)
		{
			SDL_Log("[VULKAN] Failed to create image finished semaphore.");
			return false;
		}

		if (vkCreateFence(ctx->device, &fence_info, NULL, &handler->frame_data[i].in_flight_fence) != VK_SUCCESS)
		{
			SDL_Log("[VULKAN] Failed to create in flight fence.");
			return false;
		}

	}

	return true;
}

// TODO: not hardcode vertex buffer
bool vulkan_command_handler_record(VulkanContext *ctx, VulkanCommandHandler *handler, VulkanBuffer *vertex_buffer, u32 width, u32 height)
{
	assert(ctx);
	assert(handler);
	assert(vertex_buffer);

	FrameData *frame_data = &handler->frame_data[handler->frame_index];
	assert(frame_data);

	VkCommandBufferBeginInfo begin_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,	
	};

	if (vkBeginCommandBuffer(frame_data->command_buffer, &begin_info) != VK_SUCCESS)
	{
		SDL_Log("[VULKAN] Failed to begin recording into command buffer.");
		return false;
	}

    VkImageMemoryBarrier memory_barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = ctx->swapchain.images[ctx->swapchain.image_index],
        .subresourceRange =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .layerCount = 1,
                .levelCount = 1,
            },
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    };

    vkCmdPipelineBarrier(frame_data->command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0,
                         NULL, 0, NULL, 1, &memory_barrier);

    VkViewport viewport = {
    	.width = ctx->swapchain.extent.width,
    	.height = ctx->swapchain.extent.height,
    	.maxDepth = 1.0f,	
    };

    VkRect2D scissor = {
    	.extent = ctx->swapchain.extent,
    	.offset = (VkOffset2D){0, 0},	
    };

    vkCmdSetViewport(frame_data->command_buffer, 0, 1, &viewport);
    vkCmdSetScissor(frame_data->command_buffer, 0, 1, &scissor);

    VkClearValue clear_color = {
        .color = {{0.0f, 0.0f, 0.0f, 0.0f}},
    };

    VkRenderingAttachmentInfo color_attachment_info = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = ctx->swapchain.image_views[ctx->swapchain.image_index],
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = clear_color,
    };

    VkRenderingInfo render_info = {
    	.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
    	.layerCount = 1,
    	.colorAttachmentCount = 1,
    	.pColorAttachments = &color_attachment_info,
    	.renderArea =
    	{
    		.extent = ctx->swapchain.extent,
    		.offset = (VkOffset2D){0, 0},
    	},	
    };

    vkCmdBeginRendering(frame_data->command_buffer, &render_info);

	vkCmdBindPipeline(frame_data->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->triangle_pipeline.handle);

	VkDeviceSize offsets = {0};
	vkCmdBindVertexBuffers(frame_data->command_buffer, 0, 1, &vertex_buffer->handle, &offsets);

	M4 orthographic = m4orthographic(0, width, 0, height, -1.0f, 1.0f);
	vkCmdPushConstants(frame_data->command_buffer, ctx->triangle_pipeline.layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(M4), &orthographic);

    vkCmdDraw(frame_data->command_buffer, 306, 1, 0, 0);

    vkCmdEndRendering(frame_data->command_buffer);

    memory_barrier = (VkImageMemoryBarrier){
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .image = ctx->swapchain.images[ctx->swapchain.image_index],
        .subresourceRange =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .layerCount = 1,
                .levelCount = 1,
            },
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    };

    vkCmdPipelineBarrier(frame_data->command_buffer,
                         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                         VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, NULL, 0,
                         NULL, 1, &memory_barrier);

	if (vkEndCommandBuffer(frame_data->command_buffer) != VK_SUCCESS)
	{
		SDL_Log("[VULKAN] Failed to end recording into command buffer.");
		return false;
	}

	return true;	
}

void vulkan_command_handler_deinit(VulkanContext *ctx, VulkanCommandHandler *handler)
{
	assert(ctx);
	assert(handler);

	vkDestroyCommandPool(ctx->device, handler->command_pool, NULL);

	for (u32 i = 0; i < FRAMES_IN_FLIGHT; i++)
	{
		vkDestroySemaphore(ctx->device, handler->frame_data[i].image_available, NULL);
		vkDestroyFence(ctx->device, handler->frame_data[i].in_flight_fence, NULL);
	}
}
