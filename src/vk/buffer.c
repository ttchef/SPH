
#include <vk/buffer.h>
#include <vk/context.h>
#include <vk/utils.h>
#include <vulkan/vulkan_core.h>

static bool buffer_create(vulkan_context *ctx, VkBufferUsageFlags usage, VkMemoryPropertyFlags memory_properties, usize size, vulkan_buffer *out_buffer)
{
	assert(ctx);
	assert(out_buffer);

	vulkan_buffer result = {0};

	result.size = size;

	VkBufferCreateInfo buffer_info = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = size,
		.usage = usage,	
	};

	if (vkCreateBuffer(ctx->device, &buffer_info, NULL, &result.handle) != VK_SUCCESS)
	{
		SDL_Log("[VULKAN] Failed to create device local buffer of size: %zu", size);
		return false;
	}

	VkMemoryRequirements memory_requirements;
	vkGetBufferMemoryRequirements(ctx->device, result.handle, &memory_requirements);

	u32 memory_index = vulkan_memory_type_find(ctx, memory_requirements.memoryTypeBits, memory_properties);
	if (memory_index == UINT32_MAX)
	{
		SDL_Log("[WARNING] Failed to find specific memory.");
		return false;
	}

	VkMemoryAllocateInfo alloc_info = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = memory_requirements.size,
		.memoryTypeIndex = memory_index,	
	};

	if (vkAllocateMemory(ctx->device, &alloc_info, NULL, &result.memory) != VK_SUCCESS)
	{
		SDL_Log("[VULKAN] Failed to allocate device memory.");
		return false;
	}

	if (vkBindBufferMemory(ctx->device, result.handle, result.memory, 0) != VK_SUCCESS)
	{
		SDL_Log("[VULKAN] Failed to bind memory to buffer.");
		return false;
	}
	
	*out_buffer = result;
	
	return true;
}

bool vulkan_buffer_device_local_create(vulkan_context *ctx, VkBufferUsageFlags usage, usize size, const void *data, vulkan_buffer *out_buffer)
{
	assert(ctx);
	assert(data);
	assert(out_buffer);

	vulkan_buffer result = {0};
	buffer_create(ctx, usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, size, &result);

	vulkan_buffer staging = {0};
	buffer_create(ctx, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, size, &staging);

	void *mapped;
	vkMapMemory(ctx->device, staging.memory, 0, size, 0, &mapped);
	SDL_memcpy(mapped, data, size);
	vkUnmapMemory(ctx->device, staging.memory);

	VkCommandPool command_pool;
	VkCommandBuffer command_buffer;

	VkCommandPoolCreateInfo pool_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
		.queueFamilyIndex = ctx->graphics_queue.index,	
	};

	if (vkCreateCommandPool(ctx->device, &pool_info, NULL, &command_pool) != VK_SUCCESS)
	{
		SDL_Log("[VULKAN] Failed to create command pool.");
		vulkan_buffer_destroy(ctx, &staging);
		return false;
	}

	VkCommandBufferAllocateInfo alloc_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = command_pool,
		.commandBufferCount = 1,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
	};

	if (vkAllocateCommandBuffers(ctx->device, &alloc_info, &command_buffer) != VK_SUCCESS)
	{
		goto error;
	}

	VkCommandBufferBeginInfo begin_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,	
	};

	if (vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS)
	{
		goto error;
	}

	VkBufferCopy region = {
		.size = size,	
	};

	vkCmdCopyBuffer(command_buffer, staging.handle, result.handle, 1, &region);

	if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS)
	{
		goto error;
	}

	VkSubmitInfo submit_info = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &command_buffer,	
	};

	if (vkQueueSubmit(ctx->graphics_queue.handle, 1, &submit_info, VK_NULL_HANDLE) != VK_SUCCESS)
	{
		goto error;
	}

	if (vkQueueWaitIdle(ctx->graphics_queue.handle))
	{
		goto error;
	}

	*out_buffer = result;

	vkDestroyCommandPool(ctx->device, command_pool, NULL);
	vulkan_buffer_destroy(ctx, &staging);

	return true;

error:
	vkDestroyCommandPool(ctx->device, command_pool, NULL);
	vulkan_buffer_destroy(ctx, &staging);

	SDL_Log("[VULKAN] Error occured while creating device local buffer.");
	
	return false;
}

void vulkan_buffer_destroy(vulkan_context *ctx, vulkan_buffer *buffer)
{
	assert(ctx);
	assert(buffer);

	vkDeviceWaitIdle(ctx->device);

	vkDestroyBuffer(ctx->device, buffer->handle, NULL);
	vkFreeMemory(ctx->device, buffer->memory, NULL);
}
