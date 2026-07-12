
#pragma once

#include <types.h>
#include <vk/types.h>

#include <vulkan/vulkan_core.h>


typedef struct VulkanBuffer
{
	VkBuffer handle;
	VkDeviceMemory memory;
} VulkanBuffer;

// NOTE: automatically copies data into buffer
bool vulkan_buffer_device_local_init(VulkanContext *ctx, VkBufferUsageFlags usage, usize size, const void *data, VulkanBuffer *out_buffer);

void vulkan_buffer_deinit(VulkanContext *ctx, VulkanBuffer *buffer);
