
#pragma once

#include <types.h>
#include <vk/types.h>

#include <vulkan/vulkan_core.h>

typedef struct VulkanPipeline
{
	VkPipeline handle;
	VkPipelineLayout layout;
} VulkanPipeline;

bool vulkan_pipeline_init(VulkanContext *ctx, VulkanPipeline *pipeline);

void vulkan_pipeline_deinit(VulkanContext *ctx, VulkanPipeline *pipeline);
