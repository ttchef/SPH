
#pragma once

#include <types.h>
#include <vulkan/vulkan.h>

#include <SDL3/SDL_video.h>

typedef struct VulkanQueue
{
	VkQueue handle;
	u32 index;
} VulkanQueue;

typedef struct VulkanContext
{
	VkInstance instance;
	VkSurfaceKHR surface;
	VkPhysicalDevice physical_device;

	VulkanQueue graphics_queue;
	VulkanQueue present_queue;
} VulkanContext;

bool vulkan_init(SDL_Window *window, VulkanContext *ctx);
void vulkan_deinit(VulkanContext *ctx);
