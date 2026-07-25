
#include <vk/descriptor.h>
#include <vk/context.h>

#include <SDL3/SDL_log.h>
#include <vulkan/vulkan_core.h>

bool descriptor_create(vulkan_context *ctx, VkDescriptorPoolSize pool_size, u32 binding, VkShaderStageFlagBits stage, vulkan_descriptor *out_descriptor)
{
	vulkan_descriptor result = {0};

	VkDescriptorPoolSize pool_sizes[] = {
		pool_size,
	};

	VkDescriptorPoolCreateInfo pool_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.maxSets = 1,
		.pPoolSizes = pool_sizes,
		.poolSizeCount = ARRAY_COUNT(pool_sizes),
	};

	if (vkCreateDescriptorPool(ctx->device, &pool_info, NULL, &result.pool) != VK_SUCCESS)
	{
		SDL_Log("[VULKAN] Failed to create storage buffer descriptor pool.");
		return false;
	}

	VkDescriptorSetLayoutBinding binding_info = {
		.binding = binding,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.stageFlags = stage,
	};

	VkDescriptorSetLayoutCreateInfo layout_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = 1,
		.pBindings = &binding_info,
	};

	if (vkCreateDescriptorSetLayout(ctx->device, &layout_info, NULL, &result.layout) != VK_SUCCESS)
	{
		SDL_Log("[VULKAN] Failed to create storage buffer descriptor set layout.");
		return false;
	}

	VkDescriptorSetAllocateInfo alloc_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorSetCount = 1,
		.pSetLayouts = &result.layout,
		.descriptorPool = result.pool,	
	};

	if (vkAllocateDescriptorSets(ctx->device, &alloc_info, &result.set) != VK_SUCCESS)
	{
		SDL_Log("[VULKAN] Failed to allocate storage buffer descriptor set.");
		return false;
	}

	*out_descriptor = result;

	return true;
}

bool vulkan_descriptor_storage_buffer_create(vulkan_context *ctx, vulkan_buffer buffer, u32 binding, VkShaderStageFlags stage, vulkan_descriptor *out_descriptor)
{
	vulkan_descriptor result = {0};

	VkDescriptorPoolSize pool_size = {
		.descriptorCount = 1,
		.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,	
	};

	if (!descriptor_create(ctx, pool_size, binding, stage, &result))
	{
		return false;
	}
	
	VkDescriptorBufferInfo buffer_info = {
		.buffer = buffer.handle,
		.offset = 0,
		.range = buffer.size,
	};

	VkWriteDescriptorSet write = {
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.descriptorCount = 1,
		.dstSet = result.set,
		.dstBinding = binding,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.pBufferInfo = &buffer_info,
	};

	vkUpdateDescriptorSets(ctx->device, 1, &write, 0, NULL);

	*out_descriptor = result;

	return true;	
}

bool vulkan_descriptor_image_create(vulkan_context *ctx, vulkan_image image, vulkan_sampler sampler, u32 binding, VkShaderStageFlags stage, vulkan_descriptor *out_descriptor)
{
	vulkan_descriptor result = {0};

	VkDescriptorPoolSize pool_size = {
		.descriptorCount = 1,
		.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,	
	};

	if (!descriptor_create(ctx, pool_size, binding, stage, &result))
	{
		return false;
	}

	VkDescriptorImageInfo image_info = {
		.imageLayout = image.layout,
		.imageView = image.view,
		.sampler = sampler.handle,
	};

	VkWriteDescriptorSet write = {
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.descriptorCount = 1,
		.dstSet = result.set,
		.dstBinding = binding,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.pImageInfo = &image_info,
	};

	vkUpdateDescriptorSets(ctx->device, 1, &write, 0, NULL);

	*out_descriptor = result;

	return true;	
}

void vulkan_descriptor_destroy(vulkan_context *ctx, vulkan_descriptor *descriptor)
{
	vkDestroyDescriptorSetLayout(ctx->device, descriptor->layout, NULL);
	vkDestroyDescriptorPool(ctx->device, descriptor->pool, NULL);
}
