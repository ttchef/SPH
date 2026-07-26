
#include "types.h"
#include <vk/descriptor.h>
#include <vk/context.h>

#include <SDL3/SDL_log.h>
#include <vulkan/vulkan_core.h>

#define MAX_COMBINED_IMAGE_SAMPLER_COUNT 256

bool vulkan_bindless_create(vulkan *vulkan, vulkan_bindless *out_bindless)
{
	vulkan_bindless result = {0};

	VkDescriptorPoolSize sizes[] = {
		{
			.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = MAX_COMBINED_IMAGE_SAMPLER_COUNT,
		}	
	};
	
	VkDescriptorPoolCreateInfo pool_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.maxSets = 1,
		.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
		.pPoolSizes = sizes,
		.poolSizeCount = ARRAY_COUNT(sizes),
	};

	if (vkCreateDescriptorPool(vulkan->device, &pool_info, NULL, &result.pool) != VK_SUCCESS)
	{
		SDL_Log("[VULKAN] Failed to create descriptor pool.");
		return false;
	}

	VkDescriptorSetLayoutBinding bindings[] = {
		(VkDescriptorSetLayoutBinding){
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = MAX_COMBINED_IMAGE_SAMPLER_COUNT,
			// TODO: Is is only fragment??
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		}	
	};

	VkDescriptorBindingFlags flags[] = {
		VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT,	
	};

	assert(ARRAY_COUNT(flags) == ARRAY_COUNT(bindings));

	VkDescriptorSetLayoutBindingFlagsCreateInfo flags_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
		.bindingCount = ARRAY_COUNT(bindings),
		.pBindingFlags = flags,
	};

	VkDescriptorSetLayoutCreateInfo layout_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = &flags_info,
		.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT,
		.pBindings = bindings,
		.bindingCount = ARRAY_COUNT(bindings),
	};

	if (vkCreateDescriptorSetLayout(vulkan->device, &layout_info, NULL, &result.layout) != VK_SUCCESS)
	{
		SDL_Log("[VULKAN] Failed to create descriptor set layout.");
		return false;
	}

	VkDescriptorSetAllocateInfo alloc_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = result.pool,
		.pSetLayouts = &result.layout,
		.descriptorSetCount = 1,
	};

	if (vkAllocateDescriptorSets(vulkan->device, &alloc_info, &result.set) != VK_SUCCESS)
	{
		SDL_Log("[VULKAn] Failed to allocate descriptor sets.");
		return false;
	}

	*out_bindless = result;
	
	return true;
}

void vulkan_bindless_destroy(vulkan *vulkan, vulkan_bindless *bindless)
{
	vkDestroyDescriptorPool(vulkan->device, bindless->pool, NULL);
	vkDestroyDescriptorSetLayout(vulkan->device, bindless->layout, NULL);
}
