
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
    v4 gradient_color;
    u32 gradient;
} textured_quad_pc;

typedef struct
{
    m4 model;
} cube_lines_pc;

typedef struct
{
    VkDeviceAddress positions_addr;
    VkDeviceAddress velocities_addr;
    u32             particle_count;
    float particle_radius;
} particle_render_pc;

typedef struct
{
    VkDeviceAddress positions_read_addr;
    VkDeviceAddress positions_write_addr;
    VkDeviceAddress velocities_read_addr;
    VkDeviceAddress velocities_write_addr;
    VkDeviceAddress densities_addr;
    VkDeviceAddress spatial_lookup_addr;
    VkDeviceAddress start_indices_addr;
    VkDeviceAddress padding;
    v4              box_pos;
    v4              box_size;
    f32             dt;
    f32             first_run;
} particle_update_pc;

typedef struct
{
    VkDeviceAddress densities_addr;
    VkDeviceAddress positions_read_addr;
    VkDeviceAddress spatial_lookup_addr;
    VkDeviceAddress start_indices_addr;
} particle_density_pc;

typedef struct
{
    VkDeviceAddress spatial_lookup_addr;
    VkDeviceAddress start_indices_addr;
    VkDeviceAddress positions_addr;
} spatial_lookup_write_pc;

typedef struct
{
    VkDeviceAddress spatial_lookup_addr;
    VkDeviceAddress spatial_lookup_histograms_addr;
    u32 shift;
    u32 workgroup_count;
    u32 blocks_per_workgroup;
} spatial_lookup_histograms_pc;

typedef struct
{
    VkDeviceAddress spatial_lookup_read_addr;
    VkDeviceAddress spatial_lookup_write_addr;
    VkDeviceAddress spatial_lookup_histograms_addr;
    u32 shift;
    u32 workgroup_count;
    u32 blocks_per_workgroup;
} spatial_lookup_sort_pc;

typedef struct
{
    VkDeviceAddress spatial_lookup_addr;
    VkDeviceAddress start_indices_addr;
} start_indices_pc;

typedef struct
{
    m4 model;
} color_picker_pc;

enum
{
    // NOTE: Draw Helper
    PIPELINE_TEXTURED_QUAD,
    PIPELINE_CUBE_LINES,

    // NOTE: Particle stuff
    PIPELINE_PARTICLE_RENDER,
    PIPELINE_PARTICLE_UPDATE,
    PIPELINE_PARTICLE_DENSITY,
    PIPELINE_SPATIAL_LOOKUP_WRITE,
    PIPELINE_SPATIAL_LOOKUP_HISTOGRAMS,
    PIPELINE_SPATIAL_LOOKUP_SORT,
    PIPELINE_START_INDICES,

    // NOTE: Ui
    PIPELINE_COLOR_PICKER,
    PIPELINE_COUNT,
};

bool pipelines_create(vulkan *vulkan, vulkan_pipeline_id *pipelines, u32 count);
