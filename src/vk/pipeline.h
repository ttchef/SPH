
#pragma once

#include <types.h>
#include <vk/types.h>

#include <vulkan/vulkan_core.h>

typedef enum vulkan_pipeline_type
{
	PIPELINE_TYPE_GRAPHICS,
	PIPELINE_TYPE_COMPUTE,
} VulkanPipelineType;

typedef struct vulkan_pipeline_desc
{
	VulkanPipelineType type;	
} vulkan_pipeline_desc;

typedef struct vulkan_pipeline
{
	VkPipeline handle;
	VkPipelineLayout layout;
} vulkan_pipeline;

bool vulkan_pipeline_create(vulkan_context *ctx, vulkan_pipeline_desc *desc, vulkan_pipeline *out_pipeline);

void vulkan_pipeline_destroy(vulkan_context *ctx, vulkan_pipeline *pipeline);
