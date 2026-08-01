
#pragma once

#include <types.h>
#include <vk/command.h>
#include <vk/descriptor.h>
#include <vk/pipeline.h>
#include <vk/swapchain.h>
#include <vk/types.h>

#include <SDL3/SDL_video.h>
#include <vulkan/vulkan.h>

typedef struct
{
    VkQueue handle;
    u32     index;
} vulkan_queue;

#if defined(DEBUG)
typedef struct
{
    PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT;
    PFN_vkCmdBeginDebugUtilsLabelEXT vkCmdBeginDebugUtilsLabelEXT;
    PFN_vkCmdEndDebugUtilsLabelEXT   vkCmdEndDebugUtilsLabelEXT;
} vulkan_debug_utils;
#endif

struct vulkan
{
#if defined(DEBUG)
    VkDebugUtilsMessengerEXT debug_messenger;
    vulkan_debug_utils       debug;
#endif

    VkInstance       instance;
    VkSurfaceKHR     surface;
    VkPhysicalDevice physical_device;
    usize            storage_buffer_descriptor_size;

    vulkan_queue graphics_queue;
    vulkan_queue present_queue;

    VkDevice device;

    vulkan_swapchain          swapchain;
    vulkan_command_handler    command_handler;
    vulkan_pipeline_manager   pipeline_manager;
    vulkan_bindless           bindless;
    vulkan_descriptor_buffers descriptor_buffers;

    struct
    {
        PFN_vkGetDescriptorEXT            vkGetDescriptorEXT;
        PFN_vkCmdBindDescriptorBuffersEXT vkCmdBindDescriptorBuffersEXT;
    } ext;
};

bool vulkan_create(SDL_Window *window, vulkan *vulkan, u32 global_ubo_size);

void vulkan_resize(vulkan *vulkan, u32 w, u32 h);

void vulkan_draw(vulkan *vulkan, u32 window_width, u32 window_height);

void vulkan_destroy(vulkan *vulkan);

// NOTE: Returns the current frame index
u32 vulkan_frame_index(vulkan *vulkan);
