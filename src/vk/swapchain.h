
#pragma once

#include <types.h>
#include <vk/image.h>
#include <vk/types.h>

// TODO: no sph in vk
#include <sph/memory.h>

typedef struct
{
    VkSwapchainKHR handle;

    VkImageView *image_views;
    VkSemaphore *finished;
    VkImage     *images;

    vulkan_image depth_image;

    u32 image_count;
    u32 image_index;

    VkFormat   fmt;
    VkExtent2D extent;

    memory_arena arena;
} vulkan_swapchain;

bool vulkan_swapchain_create(vulkan *vulkan, vulkan_swapchain *swapchain, u32 w, u32 h);

bool vulkan_swapchain_recreate(vulkan *vulkan, vulkan_swapchain *swapchain, u32 w, u32 h);

void vulkan_swapchain_destroy(vulkan *vulkan, vulkan_swapchain *swapchain);
