
#pragma once

#include <types.h>
#include <vk/types.h>

#include <vulkan/vulkan_core.h>

typedef struct
{
	VkDescriptorPool pool;
	VkDescriptorSet set;

	// NOTE: bindless laylout which every shader adobts
	VkDescriptorSetLayout layout;
} vulkan_bindless;

bool vulkan_bindless_create(vulkan *vulkan, vulkan_bindless *out_bindless);

void vulkan_bindless_destroy(vulkan *vulkan, vulkan_bindless *bindless);
