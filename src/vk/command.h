
#pragma once

#include <types.h>
#include <vk/types.h>
#include <vk/buffer.h>

#include <vulkan/vulkan_core.h>

typedef struct FrameData
{
	VkSemaphore image_available;
	VkFence in_flight_fence;
	VkCommandBuffer command_buffer;
} FrameData;

typedef struct VulkanCommandHandler
{
	VkCommandPool command_pool;
	FrameData frame_data[FRAMES_IN_FLIGHT];

	u32 frame_index;
	u64 accumulated_frame_index;
} VulkanCommandHandler;

bool vulkan_command_handler_init(VulkanContext *ctx, VulkanCommandHandler *handler);

bool vulkan_command_handler_record(VulkanContext *ctx, VulkanCommandHandler *handler, VulkanBuffer *vertex_buffer, u32 width, u32 height);

void vulkan_command_handler_deinit(VulkanContext *ctx, VulkanCommandHandler *handler);
