
#pragma once

#include <types.h>
#include <vk/types.h>
#include <vk/pipeline.h>
#include <vk/buffer.h>

#include <math/types.h>

#include <vulkan/vulkan_core.h>

typedef struct
{
	VkSemaphore image_available;
	VkFence in_flight_fence;
	VkCommandBuffer command_buffer;
} vulkan_frame_data;

typedef struct
{
	void *base;
	u32 size;

	void *at;
	u32 available;
} vulkan_command_queue;

typedef struct
{
	VkCommandPool command_pool;
	vulkan_frame_data frame_data[FRAMES_IN_FLIGHT];

	u32 frame_index;
	u64 accumulated_frame_index;

	vulkan_command_queue render_commands;
} vulkan_command_handler;

//
// NOTE: Render commands
//

bool vulkan_command_barrier(vulkan *vulkan, VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage, VkAccessFlags src_access, VkAccessFlags dst_access);

bool vulkan_command_begin_rendering(vulkan *vulkan);

bool vulkan_command_end_rendering(vulkan *vulkan);

bool vulkan_command_bind_pipeline(vulkan *vulkan, vulkan_pipeline_id id);

bool vulkan_command_bind_vertex_buffer(vulkan *vulkan, vulkan_buffer buffer, vulkan_pipeline_id pipeline);

bool vulkan_command_push_constants(vulkan *vulkan, u32 size, void *data, VkShaderStageFlags stage, vulkan_pipeline_id pipeline);

bool vulkan_command_draw(vulkan *vulkan, u32 vertex_count);

bool vulkan_command_dispatch(vulkan *vulkan, u32 size_x, u32 size_y, u32 size_z);

bool vulkan_command_set_viewport(vulkan *vulkan, u32 x, u32 y, u32 width, u32 height);

// ----------

bool vulkan_command_handler_create(vulkan *vulkan, vulkan_command_handler *handler);

bool vulkan_command_handler_record(vulkan *vulkan, vulkan_command_handler *handler);

void vulkan_command_handler_destroy(vulkan *vulkan, vulkan_command_handler *handler);
