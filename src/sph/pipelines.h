
#pragma once

#include <types.h>
#include <vk/pipeline.h>
#include <vk/types.h>

//
// NOTE: Push constants need to be public
//

typedef struct
{
    m4                      model;
    vulkan_bindless_image   image;
    vulkan_bindless_sampler sampler;
    f32                     roundness;
    f32                     aspect_ratio;
    v2                      uv_min;
    v2                      uv_max;
    v4                      color;
} textured_quad_pc;

typedef struct
{
    m4 model;
} cube_lines_pc;

typedef struct
{
    VkDeviceAddress positions_addr;
    u32             particle_count;
} particle_render_pc;

typedef struct
{
    VkDeviceAddress positions_addr;
    VkDeviceAddress velocities_addr;
    v4 box_pos;
    v4 box_size;
} particle_update_pc;

enum
{
    PIPELINE_TEXTURED_QUAD,
    PIPELINE_CUBE_LINES,
    PIPELINE_PARTICLE_RENDER,
    PIPELINE_PARTICLE_UPDATE,
    PIPELINE_COUNT,
};

bool pipelines_create(vulkan *vulkan, vulkan_pipeline_id *pipelines, u32 count);
