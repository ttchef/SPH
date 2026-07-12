
#pragma once

#include <types.h>
#include <vk/types.h>

#include <vulkan/vulkan_core.h>

#define SWAPCHAIN_GRAVEYARD_SIZE 4

typedef struct VulkanSwapchainZoombie
{
	VkSwapchainKHR handle;
	
	VkImageView *image_views;
	VkImage *images;
	VkSemaphore *finished;
	
	u32 image_count;
	u64 frame_retired;

	bool valid;
} VulkanSwapchainZoombie;

typedef struct VulkanSwapchain
{
	VkSwapchainKHR handle;
	
	VkImageView *image_views;
	VkImage *images;
	VkSemaphore *finished;
	
	u32 image_count;
	u32 image_index;

	VkFormat fmt;
	VkExtent2D extent;

	VulkanSwapchainZoombie graveyard[SWAPCHAIN_GRAVEYARD_SIZE];
} VulkanSwapchain;

bool vulkan_swapchain_init(VulkanContext *ctx, VulkanSwapchain *swapchain, u32 w, u32 h);

void vulkan_swapchain_drain(VulkanContext *ctx, VulkanSwapchain *swapchain, u64 accumulated_frame_index);

bool vulkan_swapchain_recreate(VulkanContext *ctx, VulkanSwapchain *swapchain, u32 w, u32 h, u64 accumulated_frame_index);

void vulkan_swapchain_deinit(VulkanContext *ctx, VulkanSwapchain *swapchain);
