
#pragma once

#include <types.h>
#include <vulkan/vulkan.h>

typedef struct VulkanContext
{
	VkInstance instance;	
} VulkanContext;

bool vulkan_init(VulkanContext *ctx);
void vulkan_deinit(VulkanContext *ctx);
