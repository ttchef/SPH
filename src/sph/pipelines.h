
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
    u32 rounded;
    u32 padding;
    v2                      uv_min;
    v2                      uv_max;
    v4                      color;
} textured_quad_pc;

typedef struct
{
    m4 model;
} cube_lines_pc;

enum
{
    PIPELINE_TEXTURED_QUAD,
    PIPELINE_CUBE_LINES,
    PIPELINE_COUNT,
};

bool pipelines_create(vulkan *vulkan, vulkan_pipeline_id *pipelines, u32 count);
