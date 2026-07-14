
#pragma once

#include <types.h>
#include <vk/types.h>

#include <vulkan/vulkan_core.h>

typedef struct vulkan_buffer
{
	VkBuffer handle;
	VkDeviceMemory memory;
} vulkan_buffer;

// NOTE: automatically copies data into buffer
bool vulkan_buffer_device_local_create(vulkan_context *ctx, VkBufferUsageFlags usage, usize size, const void *data, vulkan_buffer *out_buffer);

void vulkan_buffer_destroy(vulkan_context *ctx, vulkan_buffer *buffer);
