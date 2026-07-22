
#pragma once

#include <types.h>
#include <vk/types.h>
#include <vk/image.h>

#include <vulkan/vulkan_core.h>

#define SWAPCHAIN_GRAVEYARD_SIZE 4

typedef struct
{
	VkSwapchainKHR handle;
	
	VkImageView *image_views;
	VkImage *images;
	VkSemaphore *finished;
	vulkan_image *depth_images;
	
	u32 image_count;
	u64 frame_retired;

	bool valid;
} vulkan_swapchain_zombie;

typedef struct
{
	VkSwapchainKHR handle;
	
	VkImageView *image_views;
	VkImage *images;
	vulkan_image *depth_images;
	VkSemaphore *finished;
	
	u32 image_count;
	u32 image_index;

	VkFormat fmt;
	VkExtent2D extent;

	vulkan_swapchain_zombie graveyard[SWAPCHAIN_GRAVEYARD_SIZE];
} vulkan_swapchain;

bool vulkan_swapchain_create(vulkan_context *ctx, vulkan_swapchain *swapchain, u32 w, u32 h);

void vulkan_swapchain_drain(vulkan_context *ctx, vulkan_swapchain *swapchain, u64 accumulated_frame_index);

bool vulkan_swapchain_recreate(vulkan_context *ctx, vulkan_swapchain *swapchain, u32 w, u32 h, u64 accumulated_frame_index);

void vulkan_swapchain_destroy(vulkan_context *ctx, vulkan_swapchain *swapchain);
