
#pragma once

#include <types.h>
#include <vk/command.h>
#include <vk/descriptor.h>
#include <vk/destroy_queue.h>
#include <vk/pipeline.h>
#include <vk/swapchain.h>
#include <vk/types.h>

#include <SDL3/SDL_video.h>

typedef struct
{
    VkQueue handle;
    u32     index;
} vulkan_queue;

struct vulkan
{
#if defined(DEBUG)
    VkDebugUtilsMessengerEXT debug_messenger;
#endif

    VkInstance                 instance;
    VkSurfaceKHR               surface;
    VkPhysicalDevice           physical_device;
    VkPhysicalDeviceProperties physical_device_props;

    vulkan_queue graphics_queue;
    vulkan_queue present_queue;

    VkDevice device;

    vulkan_destroy_queue    destroy_queue;
    vulkan_swapchain        swapchain;
    vulkan_command_handler  command_handler;
    vulkan_pipeline_manager pipeline_manager;
    vulkan_bindless         bindless;
};

bool vulkan_create(SDL_Window *window, vulkan *vulkan, u32 global_ubo_size);

void vulkan_resize(vulkan *vulkan, u32 w, u32 h);

void vulkan_draw(vulkan *vulkan, u32 window_width, u32 window_height, vulkan_command_queue *queue);

void vulkan_destroy(vulkan *vulkan);

// NOTE: Returns the current frame index
u32 vulkan_frame_index(vulkan *vulkan);
