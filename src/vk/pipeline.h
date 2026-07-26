
#pragma once

#include <types.h>
#include <vk/buffer.h>
#include <vk/descriptor.h>
#include <vk/types.h>

#include <vulkan/vulkan_core.h>

typedef u32 vulkan_pipeline_id;
#define VULKAN_INVALID_PIPELINE 0

typedef enum vulkan_pipeline_type
{
    VULKAN_PIPELINE_TYPE_GRAPHICS,
    VULKAN_PIPELINE_TYPE_COMPUTE,
} vulkan_pipeline_type;

typedef struct vulkan_pipeline_desc
{
    vulkan_pipeline_type type;

    const char *vertex_path;
    const char *fragment_path;
    const char *compute_path;

    const char *vertex_entry;
    const char *fragment_entry;
    const char *compute_entry;

    VkVertexInputBindingDescription   vertex_binding;
    VkVertexInputAttributeDescription vertex_attributes[8];
    u32                               vertex_attribute_count;

    u32                push_constant_size;
    VkShaderStageFlags push_constants_stages;

    VkPolygonMode       polygon_mode;
    VkPrimitiveTopology topology;

    VkBool32 depth_test;
    VkBool32 depth_write;
} vulkan_pipeline_desc;

typedef struct vulkan_pipeline
{
    vulkan_pipeline_type type;

    VkPipeline       handle;
    VkPipelineLayout layout;
} vulkan_pipeline;

typedef struct vulkan_pipeline_manager
{
    vulkan_pipeline pipelines[128];
    u32             count;
} vulkan_pipeline_manager;

//
// NOTE: Pipeline builder
//

vulkan_pipeline_desc vulkan_pipeline_default(vulkan_pipeline_type type);

void vulkan_pipeline_desc_set_vertex_input(vulkan_pipeline_desc *desc, u32 vertex_stride, VkVertexInputAttributeDescription *attribues, u32 attribute_count);

void vulkan_pipeline_desc_set_push_constant(vulkan_pipeline_desc *desc, u32 size, VkShaderStageFlags stages);

void vulkan_pipeline_desc_set_shaders(vulkan_pipeline_desc *desc, const char *vertex, const char *fragment, const char *compute);

void vulkan_pipeline_desc_set_shaders_entries(vulkan_pipeline_desc *desc, const char *vertex_entry, const char *fragment_entry, const char *compute_entry);

void vulkan_pipeline_desc_set_specialization_constant(vulkan_pipeline_desc *desc, u32 size, void *data, VkShaderStageFlags stage);

void vulkan_pipeline_desc_set_polygon_mode(vulkan_pipeline_desc *desc, VkPolygonMode polygon_mode);

void vulkan_pipeline_desc_set_topology(vulkan_pipeline_desc *desc, VkPrimitiveTopology topology);

void vulkan_pipeline_desc_set_depth(vulkan_pipeline_desc *desc, VkBool32 depth_test, VkBool32 depth_write);

//
// NOTE: Pipeline manger
//       Is used to manage the pipelines so they dont need to be destroyed manually
//

bool vulkan_pipeline_manager_create(vulkan_pipeline_manager *manager);

void               vulkan_pipeline_manager_destroy(vulkan *vulkan, vulkan_pipeline_manager *manager);
vulkan_pipeline_id vulkan_pipeline_create(vulkan *vulkan, vulkan_pipeline_desc *desc);

vulkan_pipeline *vulkan_pipeline_get(vulkan *vulkan, vulkan_pipeline_id id);
