
#pragma once

#include <types.h>
#include <vk/types.h>

#include <vulkan/vulkan_core.h>

typedef enum vulkan_buffer_type
{
	VULKAN_BUFFER_TYPE_HOST_VISIBLE,
	VULKAN_BUFFER_TYPE_DEVICE_LOCAL,
} vulkan_buffer_type;

typedef struct vulkan_buffer
{
	vulkan_buffer_type type;
	
	VkBuffer handle;
	VkDeviceMemory memory;
	VkDeviceSize size;

	struct
	{
		void *data;
	} host_visible;
} vulkan_buffer;

// NOTE: automatically copies data into buffer
bool vulkan_buffer_device_local_create(vulkan_context *ctx, VkBufferUsageFlags usage, u32 size, const void *data, vulkan_buffer *out_buffer);

bool vulkan_buffer_host_visible_create(vulkan_context *ctx, VkBufferUsageFlags usage, u32 size, const void *data, vulkan_buffer *out_buffer);

// NOTE: out_buffer is of type host visible
bool vulkan_buffer_device_local_get_data(vulkan_context *ctx, vulkan_buffer buffer, vulkan_buffer *out_buffer);

void vulkan_buffer_destroy(vulkan_context *ctx, vulkan_buffer *buffer);
