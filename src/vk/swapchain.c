
#include "vk/image.h"
#include <vk/swapchain.h>
#include <vk/context.h>

#include <SDL3/SDL_log.h> 
#include <vulkan/vulkan_core.h>

static bool swapchain_build(vulkan_context *ctx, vulkan_swapchain *swapchain, u32 w, u32 h, VkSwapchainKHR old_handle)
{
	assert(ctx);
	assert(swapchain);
	
	VkSurfaceCapabilitiesKHR caps;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx->physical_device, ctx->surface, &caps);

	u32 format_count;
	vkGetPhysicalDeviceSurfaceFormatsKHR(ctx->physical_device, ctx->surface, &format_count, NULL);

	if (format_count == 0)
	{
		SDL_Log("[VULKAN] Failed to find a surface format.");
		return false;
	}

	VkSurfaceFormatKHR formats[format_count];	
	vkGetPhysicalDeviceSurfaceFormatsKHR(ctx->physical_device, ctx->surface, &format_count, formats);

	VkSurfaceFormatKHR fmt = formats[0];
	for (u32 i = 0; i < format_count; i++)
	{
		if (formats[i].format == VK_FORMAT_R8G8B8_SRGB && formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			fmt = formats[i];
			break;
		}
	}

	u32 present_mode_count;
	vkGetPhysicalDeviceSurfacePresentModesKHR(ctx->physical_device, ctx->surface, &present_mode_count, NULL);

	if (present_mode_count == 0)
	{
		SDL_Log("[VULKAN] Failed to find a present mode.");
		return false;
	}

	VkPresentModeKHR present_modes[present_mode_count];
	vkGetPhysicalDeviceSurfacePresentModesKHR(ctx->physical_device, ctx->surface, &present_mode_count, present_modes);

	VkPresentModeKHR present_mode = present_modes[0];
	for (u32 i = 0; i < present_mode_count; i++)
	{
		if (present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			present_mode = present_modes[i];
			break;
		}
	}

	VkExtent2D extent = (VkExtent2D){
		.width = w,
		.height = h,
	};

	extent.width = CLAMP(extent.width, caps.minImageExtent.width, caps.maxImageExtent.width);
	extent.height = CLAMP(extent.height, caps.minImageExtent.height, caps.maxImageExtent.height);

	u32 image_count = caps.minImageCount + 1;
	if (caps.maxImageCount > 0 && image_count > caps.maxImageCount)
	{
		image_count = caps.maxImageCount;
	}

	VkSwapchainCreateInfoKHR info = {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.oldSwapchain = old_handle,
		.surface = ctx->surface,
		.clipped = VK_TRUE,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.imageExtent = extent,
		.imageFormat = fmt.format,
		.imageColorSpace = fmt.colorSpace,
		.presentMode = present_mode,
		.minImageCount = image_count,
		.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
	};

	if (vkCreateSwapchainKHR(ctx->device, &info, NULL, &swapchain->handle) != VK_SUCCESS)
	{
		SDL_Log("[VULKAN] Failed to create swapchain.");
		return false;
	}

	swapchain->extent = extent;
	swapchain->fmt = fmt.format;

	vkGetSwapchainImagesKHR(ctx->device, swapchain->handle, &swapchain->image_count, NULL);

	swapchain->images = SDL_calloc(swapchain->image_count, sizeof(VkImage));
	assert(swapchain->images);

	vkGetSwapchainImagesKHR(ctx->device, swapchain->handle, &swapchain->image_count, swapchain->images);

	swapchain->image_views = SDL_calloc(swapchain->image_count, sizeof(VkImageView));
	assert(swapchain->image_views);
	
	swapchain->depth_images = SDL_calloc(swapchain->image_count, sizeof(vulkan_image));
	assert(swapchain->depth_images);

	VkSemaphoreCreateInfo semaphore_info = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,	
	};

	swapchain->finished = SDL_calloc(swapchain->image_count, sizeof(VkSemaphore));
	assert(swapchain->finished);

	for (u32 i = 0; i < swapchain->image_count; i++)
	{
		VkImageViewCreateInfo view_info = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = swapchain->images[i],
			.format = swapchain->fmt,
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.subresourceRange =
			{
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.layerCount = 1,
				.levelCount = 1,
			},
		};

		if (vkCreateImageView(ctx->device, &view_info, NULL, &swapchain->image_views[i]) != VK_SUCCESS)
		{
			SDL_Log("[VULKAN] Failed to create image view: %u.", i);
			goto error;			
		}

		if (!vulkan_image_create(ctx, v2umake(extent.width, extent.height), VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT, &swapchain->depth_images[i]))
		{
			goto error;
		}

		vulkan_image_transition(ctx, &swapchain->depth_images[i], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 0, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);
	
		if (vkCreateSemaphore(ctx->device, &semaphore_info, NULL, &swapchain->finished[i]) != VK_SUCCESS)
		{
			SDL_Log("[VULKAN] Failed to create swapchain semaphore: %u.", i);
			goto error;
		}
	}

	swapchain->image_index = 0;
	
	return true;

error:
	SDL_Log("[VULKAN] Failed to create swapchain.");

	if (swapchain->image_views)
	{
		SDL_free(swapchain->image_views);
		swapchain->image_views = NULL;
	}

	if (swapchain->images)
	{
		SDL_free(swapchain->images);
		swapchain->images = NULL;
	}

	if (swapchain->depth_images)
	{
		SDL_free(swapchain->depth_images);
		swapchain->depth_images = NULL;
	}

	if (swapchain->finished)
	{
		SDL_free(swapchain->finished);
		swapchain->finished = NULL;
	}

	return false;
}

bool vulkan_swapchain_create(vulkan_context *ctx, vulkan_swapchain *swapchain, u32 w, u32 h)
{
	assert(ctx);
	assert(swapchain);

	return swapchain_build(ctx, swapchain, w, h, VK_NULL_HANDLE);	
}

static void zoombie_destroy(vulkan_context *ctx, vulkan_swapchain_zoombie *zoombie)
{
	for (u32 i = 0; i < zoombie->image_count; i++)
	{
		vkDestroyImageView(ctx->device, zoombie->image_views[i], NULL);
		vkDestroySemaphore(ctx->device, zoombie->finished[i], NULL);
		vulkan_image_destroy(ctx, zoombie->depth_images[i]);
	}

	vkDestroySwapchainKHR(ctx->device, zoombie->handle, NULL);

	assert(zoombie->finished);
	assert(zoombie->image_views);
	assert(zoombie->images);

	SDL_free(zoombie->finished);
	SDL_free(zoombie->image_views);
	SDL_free(zoombie->images);

	zoombie->finished = NULL;
	zoombie->image_views = NULL;
	zoombie->images = NULL;
	
	zoombie->valid = false;
}

void vulkan_swapchain_drain(vulkan_context *ctx, vulkan_swapchain *swapchain, u64 accumulated_frame_index)
{
	assert(ctx);
	assert(swapchain);

	for (u32 i = 0; i < SWAPCHAIN_GRAVEYARD_SIZE; i++)
	{
		vulkan_swapchain_zoombie *zoombie = &swapchain->graveyard[i];
		assert(zoombie);

		if (!zoombie->valid)
		{
			continue;
		}

		if (accumulated_frame_index - zoombie->frame_retired < FRAMES_IN_FLIGHT)
		{
			continue;
		}

		zoombie_destroy(ctx, zoombie);
	}
}

bool vulkan_swapchain_recreate(vulkan_context *ctx, vulkan_swapchain *swapchain, u32 w, u32 h, u64 accumulated_frame_index)
{
	assert(ctx);
	assert(swapchain);

	i32 zoombie_index = -1;
	for (u32 i = 0; i < SWAPCHAIN_GRAVEYARD_SIZE; i++)
	{
		vulkan_swapchain_zoombie *zoombie = &swapchain->graveyard[i];
		assert(zoombie);

		if (!zoombie->valid)
		{
			zoombie_index = i;
			break;
		}
	}

	if (zoombie_index == -1)
	{
		SDL_Log("[VULKAN] Swapchain graveyard is full.");
		
		vulkan_swapchain_destroy(ctx, swapchain);

		return swapchain_build(ctx, swapchain, w, h, VK_NULL_HANDLE);
	}

	vulkan_swapchain_zoombie *zoombie = &swapchain->graveyard[zoombie_index];
	assert(zoombie);

	zoombie->handle = swapchain->handle;
	zoombie->image_views = swapchain->image_views;
	zoombie->depth_images = swapchain->depth_images;
	zoombie->images = swapchain->images;
	zoombie->finished = swapchain->finished;
	zoombie->image_count = swapchain->image_count;
	zoombie->frame_retired = accumulated_frame_index;

	zoombie->valid = true;	

	swapchain->handle = VK_NULL_HANDLE;
	swapchain->image_views = NULL;
	swapchain->images = NULL;
	swapchain->depth_images = NULL;
	swapchain->finished = NULL;

	SDL_Log("[VULKAN] Swapchain recreated (%ux%u)", w, h);

	return swapchain_build(ctx, swapchain, w, h, zoombie->handle);
}

void vulkan_swapchain_destroy(vulkan_context *ctx, vulkan_swapchain *swapchain)
{
	assert(ctx);
	assert(swapchain);

	for (u32 i = 0; i < SWAPCHAIN_GRAVEYARD_SIZE; i++)
	{
		vulkan_swapchain_zoombie *zoombie = &swapchain->graveyard[i];
		assert(zoombie);

		if (!zoombie->valid)
		{
			continue;
		}

		zoombie_destroy(ctx, zoombie);
	}
	
	for (u32 i = 0; i < swapchain->image_count; i++)
	{
		vkDestroyImageView(ctx->device, swapchain->image_views[i], NULL);
		vkDestroySemaphore(ctx->device, swapchain->finished[i], NULL);
	}

	vkDestroySwapchainKHR(ctx->device, swapchain->handle, NULL);

	assert(swapchain->finished);
	assert(swapchain->image_views);
	assert(swapchain->images);

	SDL_free(swapchain->finished);
	SDL_free(swapchain->image_views);
	SDL_free(swapchain->images);

	swapchain->finished = NULL;
	swapchain->image_views = NULL;
	swapchain->images = NULL;
}
