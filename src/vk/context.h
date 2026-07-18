
#pragma once

#include <types.h>
#include <vk/types.h>
#include <vk/swapchain.h>
#include <vk/pipeline.h>
#include <vk/command.h>

#include <vulkan/vulkan.h>
#include <SDL3/SDL_video.h>

typedef struct vulkan_queue
{
	VkQueue handle;
	u32 index;
} vulkan_queue;

struct vulkan_context
{
#if defined(DEBUG)
	VkDebugUtilsMessengerEXT debug_messenger;
#endif
	
	VkInstance instance;
	VkSurfaceKHR surface;
	VkPhysicalDevice physical_device;

	vulkan_queue graphics_queue;
	vulkan_queue present_queue;

	VkDevice device;

	vulkan_swapchain swapchain;
	vulkan_command_handler command_handler;
	vulkan_pipeline_manager pipeline_manager;
};

bool vulkan_init(SDL_Window *window, vulkan_context *ctx);

void vulkan_resize(vulkan_context *ctx, u32 w, u32 h);

void vulkan_draw(vulkan_context *ctx, u32 window_width, u32 window_height);

void vulkan_deinit(vulkan_context *ctx);

// NOTE: Returns the current frame index
u32 vulkan_frame_index(vulkan_context *ctx);
