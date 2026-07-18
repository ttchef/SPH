
#pragma once

#include <types.h>
#include <vk/types.h>
#include <vk/buffer.h>

#include <vulkan/vulkan_core.h>

typedef struct vulkan_descriptor
{
	VkDescriptorPool pool;
	VkDescriptorSetLayout layout;
	VkDescriptorSet set;
} vulkan_descriptor;

bool vulkan_descriptor_storage_buffer_create(vulkan_context *ctx, vulkan_buffer buffer, u32 binding, VkShaderStageFlags stage, vulkan_descriptor *out_descriptor);

void vulkan_descriptor_destroy(vulkan_context *ctx, vulkan_descriptor *descriptor);
