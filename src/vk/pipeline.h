
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

    // NOTE: Used for debugging
    const char *name;

    const char *vertex_path;
    const char *fragment_path;
    const char *compute_path;

    const char *vertex_entry;
    const char *fragment_entry;
    const char *compute_entry;

    VkVertexInputBindingDescription    vertex_binding;
    VkVertexInputAttributeDescription *vertex_attributes;
    u32                                vertex_attribute_count;

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
// NOTE: Pipeline manger
//       Is used to manage the pipelines so they dont need to be destroyed manually
//

bool vulkan_pipeline_manager_create(vulkan_pipeline_manager *manager);

void               vulkan_pipeline_manager_destroy(vulkan *vulkan, vulkan_pipeline_manager *manager);
vulkan_pipeline_id vulkan_pipeline_create(vulkan *vulkan, vulkan_pipeline_desc *desc);

vulkan_pipeline *vulkan_pipeline_get(vulkan *vulkan, vulkan_pipeline_id id);
