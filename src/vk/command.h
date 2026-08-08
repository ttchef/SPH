
#pragma once

#include <types.h>
#include <vk/buffer.h>
#include <vk/pipeline.h>
#include <vk/types.h>

#include <math/types.h>

// TODO: SPH in vk is cooked
#include <sph/memory.h>

#include <vulkan/vulkan_core.h>

typedef struct
{
    VkSemaphore     image_available;
    VkFence         in_flight_fence;
    VkCommandBuffer command_buffer;
} vulkan_frame_data;

// NOTE: Gets typedef in vk/types.h
struct vulkan_command_queue
{
    void *base;
    u32   size;

    void *at;
    u32   available;

    bool present;
};

typedef struct
{
    VkCommandPool pool;
    VkCommandBuffer buffer;
    VkSemaphore timeline;
    u64 value;
} vulkan_immeadiate_context;

typedef struct
{
    VkCommandPool     command_pool;
    vulkan_frame_data frame_data[FRAMES_IN_FLIGHT];

    vulkan_immeadiate_context contexts[12];
    u32 next_context;

    u32 frame_index;
    u64 accumulated_frame_index;
} vulkan_command_handler;

//
// NOTE: Render commands
//

bool vulkan_command_barrier(vulkan_command_queue *queue, VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage, VkAccessFlags src_access, VkAccessFlags dst_access);

bool vulkan_command_image_barrier(vulkan_command_queue *queue, vulkan_image image, vulkan_image_info src, vulkan_image_info dst);

bool vulkan_command_begin_rendering(vulkan_command_queue *queue);

bool vulkan_command_end_rendering(vulkan_command_queue *queue);

bool vulkan_command_bind_pipeline(vulkan_command_queue *queue, vulkan_pipeline_id id);

bool vulkan_command_bind_vertex_buffer(vulkan_command_queue *queue, vulkan_buffer buffer, vulkan_pipeline_id pipeline);

bool vulkan_command_push_constants(vulkan_command_queue *queue, u32 size, void *data, VkShaderStageFlags stage, vulkan_pipeline_id pipeline);

bool vulkan_command_draw(vulkan_command_queue *queue, u32 vertex_count);

bool vulkan_command_dispatch(vulkan_command_queue *queue, u32 size_x, u32 size_y, u32 size_z);

bool vulkan_command_set_viewport(vulkan_command_queue *queue, f32 x, f32 y, f32 width, f32 height);

bool vulkan_command_copy_buffer(vulkan_command_queue *queue, vulkan_buffer src, vulkan_buffer dst);

bool vulkan_command_copy_image(vulkan_command_queue *queue, vulkan_buffer src, vulkan_image dst, vulkan_image_info dst_info);

bool vulkan_command_bind_scene_ubo(vulkan_command_queue *queue, vulkan_bindless_scene_ubo scene_ubo, vulkan_pipeline_id pipeline);

//
// NOTE: Special ones
// 
vulkan_command_queue *vulkan_command_begin(memory_arena *arena);

bool vulkan_command_end(vulkan_command_queue *queue, vulkan *vulkan, bool wait);

void vulkan_command_set_present(vulkan_command_queue *queue);

#if defined(DEBUG)
bool vulkan_command_label_begin(vulkan_command_queue *queue, const char *name, color4 color);

bool vulkan_command_label_end(vulkan_command_queue *queue);
#else
#define vulkan_command_label_begin(...) (void)0

#define vulkan_command_label_end(...) (void)0
#endif

// ----------

bool vulkan_command_handler_create(vulkan *vulkan, vulkan_command_handler *out_handler);

vulkan_immeadiate_context *vulkan_command_handler_immeadiate_get(vulkan *vulkan, vulkan_command_handler *handler);

void vulkan_command_handler_destroy(vulkan *vulkan, vulkan_command_handler *handler);
