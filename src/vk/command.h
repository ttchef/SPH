
#pragma once

#include "vk/pipeline.h"
#include <types.h>
#include <vk/types.h>
#include <vk/buffer.h>

#include <math/types.h>

#include <vulkan/vulkan_core.h>

typedef struct vulkan_frame_data
{
	VkSemaphore image_available;
	VkFence in_flight_fence;
	VkCommandBuffer command_buffer;
} vulkan_frame_data;

typedef enum vulkan_render_command_type
{
	VULKAN_RENDER_COMMAND_BIND_PIPELINE,
	VULKAN_RENDER_COMMAND_BIND_VERTEX_BUFFER,
	VULKAN_RENDER_COMMAND_PUSH_CONSTANTS,
	// NOTE: vkCmdDraw, uses currently bound pipeline 
	VULKAN_RENDER_COMMAND_DRAW, 
} vulkan_render_command_type;

typedef struct vulkan_render_command_header
{
	u32 type;
	u32 size;
} vulkan_render_command_header;

typedef struct vulkan_render_command_bind_pipeline
{
	vulkan_render_command_header header;
	vulkan_pipeline_id id;
} vulkan_render_command_bind_pipeline;

typedef struct vulkan_render_command_bind_vertex_buffer
{
	vulkan_render_command_header header;
	vulkan_buffer buffer;
	vulkan_pipeline_id pipeline;
} vulkan_render_command_bind_vertex_buffer;

typedef struct vulkan_render_command_push_constants
{
	vulkan_render_command_header header;
	u32 size;
	void *data;
	VkShaderStageFlags stage;
	vulkan_pipeline_id pipeline;
} vulkan_render_command_push_constants;

typedef struct vulkan_render_command_draw
{
	vulkan_render_command_header header;
	u32 vertex_count;
} vulkan_render_command_draw;

typedef struct vulkan_command_queue
{
	void *base;
	u32 size;

	void *at;
	u32 available;
} vulkan_command_queue;

typedef struct vulkan_command_handler
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

bool vulkan_command_bind_pipeline(vulkan_context *ctx, vulkan_pipeline_id id);

bool vulkan_command_bind_vertex_buffer(vulkan_context *ctx, vulkan_buffer buffer, vulkan_pipeline_id pipeline);

bool vulkan_command_push_constants(vulkan_context *ctx, u32 size, void *data, VkShaderStageFlags stage, vulkan_pipeline_id pipeline);

bool vulkan_command_draw(vulkan_context *ctx, u32 vertex_count);

// ----------

bool vulkan_command_handler_create(vulkan_context *ctx, vulkan_command_handler *handler);

bool vulkan_command_handler_record(vulkan_context *ctx, vulkan_command_handler *handler);

void vulkan_command_handler_destroy(vulkan_context *ctx, vulkan_command_handler *handler);
