
#pragma once

#include <types.h>
#include <vk/swapchain.h>

#include <vulkan/vulkan.h>
#include <SDL3/SDL_video.h>

typedef struct VulkanContext VulkanContext;

typedef struct VulkanQueue
{
	VkQueue handle;
	u32 index;
} VulkanQueue;

struct VulkanContext
{
#if defined(DEBUG)
	VkDebugUtilsMessengerEXT debug_messenger;
#endif
	
	VkInstance instance;
	VkSurfaceKHR surface;
	VkPhysicalDevice physical_device;

	VulkanQueue graphics_queue;
	VulkanQueue present_queue;

	VkDevice device;

	VulkanSwapchain swapchain;
};

bool vulkan_init(SDL_Window *window, VulkanContext *ctx);

void vulkan_deinit(VulkanContext *ctx);
