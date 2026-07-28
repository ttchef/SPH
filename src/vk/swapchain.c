
#include <vk/swapchain.h>
#include <vk/context.h>

#include <SDL3/SDL_log.h> 
#include <vulkan/vulkan_core.h>

static bool swapchain_build(vulkan *vulkan, vulkan_swapchain *swapchain, u32 w, u32 h, VkSwapchainKHR old_handle)
{	
	VkSurfaceCapabilitiesKHR caps;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vulkan->physical_device, vulkan->surface, &caps);

	u32 format_count;
	vkGetPhysicalDeviceSurfaceFormatsKHR(vulkan->physical_device, vulkan->surface, &format_count, NULL);

	if (format_count == 0)
	{
		SDL_Log("[VULKAN] Failed to find a surface format.");
		return false;
	}

	VkSurfaceFormatKHR formats[format_count];	
	vkGetPhysicalDeviceSurfaceFormatsKHR(vulkan->physical_device, vulkan->surface, &format_count, formats);

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
	vkGetPhysicalDeviceSurfacePresentModesKHR(vulkan->physical_device, vulkan->surface, &present_mode_count, NULL);

	if (present_mode_count == 0)
	{
		SDL_Log("[VULKAN] Failed to find a present mode.");
		return false;
	}

	VkPresentModeKHR present_modes[present_mode_count];
	vkGetPhysicalDeviceSurfacePresentModesKHR(vulkan->physical_device, vulkan->surface, &present_mode_count, present_modes);

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
		.surface = vulkan->surface,
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

	if (vkCreateSwapchainKHR(vulkan->device, &info, NULL, &swapchain->handle) != VK_SUCCESS)
	{
		SDL_Log("[VULKAN] Failed to create swapchain.");
		return false;
	}

	swapchain->extent = extent;
	swapchain->fmt = fmt.format;

	vkGetSwapchainImagesKHR(vulkan->device, swapchain->handle, &swapchain->image_count, NULL);

	swapchain->images = SDL_calloc(swapchain->image_count, sizeof(VkImage));
	assert(swapchain->images);

	vkGetSwapchainImagesKHR(vulkan->device, swapchain->handle, &swapchain->image_count, swapchain->images);

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

		if (vkCreateImageView(vulkan->device, &view_info, NULL, &swapchain->image_views[i]) != VK_SUCCESS)
		{
			SDL_Log("[VULKAN] Failed to create image view: %u.", i);
			goto error;			
		}

		if (!vulkan_image_create(vulkan, v2umake(extent.width, extent.height), VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT, false, "depth", &swapchain->depth_images[i]))
		{
			goto error;
		}

		vulkan_image_transition(vulkan, &swapchain->depth_images[i], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 0, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);
	
		if (vkCreateSemaphore(vulkan->device, &semaphore_info, NULL, &swapchain->finished[i]) != VK_SUCCESS)
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

bool vulkan_swapchain_create(vulkan *vulkan, vulkan_swapchain *swapchain, u32 w, u32 h)
{
	assert(vulkan);
	assert(swapchain);

	return swapchain_build(vulkan, swapchain, w, h, VK_NULL_HANDLE);	
}

static void zoombie_destroy(vulkan *vulkan, vulkan_swapchain_zombie *zombie)
{
	for (u32 i = 0; i < zombie->image_count; i++)
	{
		vkDestroyImageView(vulkan->device, zombie->image_views[i], NULL);
		vkDestroySemaphore(vulkan->device, zombie->finished[i], NULL);
		vulkan_image_destroy(vulkan, zombie->depth_images[i]);
	}

	vkDestroySwapchainKHR(vulkan->device, zombie->handle, NULL);

	assert(zombie->finished);
	assert(zombie->image_views);
	assert(zombie->images);
	assert(zombie->depth_images);

	SDL_free(zombie->finished);
	SDL_free(zombie->image_views);
	SDL_free(zombie->images);
	SDL_free(zombie->depth_images);

	zombie->finished = NULL;
	zombie->image_views = NULL;
	zombie->images = NULL;
	zombie->depth_images = NULL;
	
	zombie->valid = false;
}

void vulkan_swapchain_drain(vulkan *vulkan, vulkan_swapchain *swapchain, u64 accumulated_frame_index)
{
	assert(vulkan);
	assert(swapchain);

	for (u32 i = 0; i < SWAPCHAIN_GRAVEYARD_SIZE; i++)
	{
		vulkan_swapchain_zombie *zombie = &swapchain->graveyard[i];
		assert(zombie);

		if (!zombie->valid)
		{
			continue;
		}

		if (accumulated_frame_index - zombie->frame_retired < FRAMES_IN_FLIGHT)
		{
			continue;
		}

		zoombie_destroy(vulkan, zombie);
	}
}

bool vulkan_swapchain_recreate(vulkan *vulkan, vulkan_swapchain *swapchain, u32 w, u32 h, u64 accumulated_frame_index)
{
	assert(vulkan);
	assert(swapchain);

	i32 zoombie_index = -1;
	for (u32 i = 0; i < SWAPCHAIN_GRAVEYARD_SIZE; i++)
	{
		vulkan_swapchain_zombie *zombie = &swapchain->graveyard[i];
		assert(zombie);

		if (!zombie->valid)
		{
			zoombie_index = i;
			break;
		}
	}

	if (zoombie_index == -1)
	{
		SDL_Log("[VULKAN] Swapchain graveyard is full.");
		
		vulkan_swapchain_destroy(vulkan, swapchain);

		return swapchain_build(vulkan, swapchain, w, h, VK_NULL_HANDLE);
	}

	vulkan_swapchain_zombie *zombie = &swapchain->graveyard[zoombie_index];
	assert(zombie);

	zombie->handle = swapchain->handle;
	zombie->image_views = swapchain->image_views;
	zombie->depth_images = swapchain->depth_images;
	zombie->images = swapchain->images;
	zombie->finished = swapchain->finished;
	zombie->image_count = swapchain->image_count;
	zombie->frame_retired = accumulated_frame_index;

	zombie->valid = true;	

	swapchain->handle = VK_NULL_HANDLE;
	swapchain->image_views = NULL;
	swapchain->images = NULL;
	swapchain->depth_images = NULL;
	swapchain->finished = NULL;

	SDL_Log("[VULKAN] Swapchain recreated (%ux%u)", w, h);

	return swapchain_build(vulkan, swapchain, w, h, zombie->handle);
}

void vulkan_swapchain_destroy(vulkan *vulkan, vulkan_swapchain *swapchain)
{
	assert(vulkan);
	assert(swapchain);

	for (u32 i = 0; i < SWAPCHAIN_GRAVEYARD_SIZE; i++)
	{
		vulkan_swapchain_zombie *zombie = &swapchain->graveyard[i];
		assert(zombie);

		if (!zombie->valid)
		{
			continue;
		}

		zoombie_destroy(vulkan, zombie);
	}
	
	for (u32 i = 0; i < swapchain->image_count; i++)
	{
		vkDestroyImageView(vulkan->device, swapchain->image_views[i], NULL);
		vkDestroySemaphore(vulkan->device, swapchain->finished[i], NULL);
		vulkan_image_destroy(vulkan, swapchain->depth_images[i]);
	}

	vkDestroySwapchainKHR(vulkan->device, swapchain->handle, NULL);

	assert(swapchain->finished);
	assert(swapchain->image_views);
	assert(swapchain->images);
	assert(swapchain->depth_images);

	SDL_free(swapchain->finished);
	SDL_free(swapchain->image_views);
	SDL_free(swapchain->images);
	SDL_free(swapchain->depth_images);

	swapchain->finished = NULL;
	swapchain->image_views = NULL;
	swapchain->images = NULL;
	swapchain->depth_images = NULL;
}
