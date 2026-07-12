
#pragma once

#include <types.h>
#include <vk/context.h>

#include <vulkan/vulkan_core.h>
#include <SDL3/SDL_log.h>

// NOTE: returns UINT32_MAX if not found
static inline u32 vulkan_memory_type_find(VulkanContext *ctx, u32 type_filter, VkMemoryPropertyFlags memory_properties)
{
	VkPhysicalDeviceMemoryProperties device_properties;
	vkGetPhysicalDeviceMemoryProperties(ctx->physical_device, &device_properties);

	for (u32 i = 0; i < device_properties.memoryTypeCount; i++)
	{
		if ((type_filter & (1 << i)) != 0)
		{
			if ((device_properties.memoryTypes[i].propertyFlags & memory_properties) == memory_properties)
			{
				return i;
			}
		}
	}

	SDL_Log("[VULKAN] Warning: Couldnt find fitting memory type.");
	return UINT32_MAX;
}
