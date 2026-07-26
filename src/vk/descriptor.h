
#pragma once

#include <types.h>
#include <vk/types.h>
#include <vk/buffer.h>
#include <vk/image.h>

#include <vulkan/vulkan_core.h>

typedef struct
{
	VkDescriptorPool pool;
	VkDescriptorSetLayout layout;
	VkDescriptorSet set;
} vulkan_descriptor;

bool vulkan_descriptor_storage_buffer_create(vulkan *vulkan, vulkan_buffer buffer, u32 binding, VkShaderStageFlags stage, vulkan_descriptor *out_descriptor);

bool vulkan_descriptor_image_create(vulkan *vulkan, vulkan_image image, vulkan_sampler sampler, u32 binding, VkShaderStageFlags stage, vulkan_descriptor *out_descriptor);

void vulkan_descriptor_destroy(vulkan *vulkan, vulkan_descriptor *descriptor);
