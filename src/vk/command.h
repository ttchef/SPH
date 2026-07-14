
#pragma once

#include <types.h>
#include <vk/types.h>
#include <vk/buffer.h>

#include <vulkan/vulkan_core.h>

typedef struct vulkan_frame_data
{
	VkSemaphore image_available;
	VkFence in_flight_fence;
	VkCommandBuffer command_buffer;
} vulkan_frame_Data;

typedef struct vulkan_command_handler
{
	VkCommandPool command_pool;
	vulkan_frame_Data frame_data[FRAMES_IN_FLIGHT];

	u32 frame_index;
	u64 accumulated_frame_index;
} vulkan_command_handler;

bool vulkan_command_handler_create(vulkan_context *ctx, vulkan_command_handler *handler);

bool vulkan_command_handler_record(vulkan_context *ctx, vulkan_command_handler *handler, vulkan_buffer *vertex_buffer, u32 width, u32 height);

void vulkan_command_handler_destroy(vulkan_context *ctx, vulkan_command_handler *handler);
