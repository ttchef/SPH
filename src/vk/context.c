
#include "vk/pipeline.h"
#include "vk/swapchain.h"
#include <vk/context.h>

#include <SDL3/SDL_vulkan.h>
#include <SDL3/SDL_log.h>
#include <vulkan/vulkan_core.h>

#define MAX_EXTENSIONS 16

static bool extensions_add(const char **extensions, u32 *extension_count, const char *extension)
{
	assert(extensions);
	assert(extension_count);
	assert(extension);

	if (*extension_count + 1 > MAX_EXTENSIONS)
	{
		SDL_Log("[VULKAN] Failed to add instance extension: %s (%u/%u).\n", extension, *extension_count, MAX_EXTENSIONS);
		return false;
	}

	extensions[(*extension_count)++] = extension;

	return true;
}

static bool instance_init(VulkanContext *ctx)
{
	assert(ctx);
	
	VkApplicationInfo app_info = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.apiVersion = VK_API_VERSION_1_3, // NOTE: required?
		.engineVersion = VK_MAKE_VERSION(1, 0, 0),
		.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
		.pEngineName = "SPH",
		.pApplicationName = "SPH",		
	};

	u32 sdl_extension_count;
	const char * const *sdl_extensions = SDL_Vulkan_GetInstanceExtensions(&sdl_extension_count);
	assert(sdl_extensions);

	u32 extension_count = 0;
	const char *extensions[MAX_EXTENSIONS];

	for (u32 i = 0; i < sdl_extension_count; i++)
	{
		extensions_add(extensions, &extension_count, sdl_extensions[i]);
	}

#if defined(DEBUG)
	extensions_add(extensions, &extension_count, VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

	const char *layers[] = {
		"VK_LAYER_KHRONOS_validation",	
	};
#endif

	VkInstanceCreateInfo info = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &app_info,
		.enabledExtensionCount = extension_count,
		.ppEnabledExtensionNames = extensions,
#if defined(DEBUG)
		.enabledLayerCount = ARRAY_COUNT(layers),
		.ppEnabledLayerNames = layers,
#endif	
	};

	if (vkCreateInstance(&info, NULL, &ctx->instance) != VK_SUCCESS)
	{
		SDL_Log("[VULKAN] Failed to create instance.");
		return false;
	}

	return true;
}

#if defined(DEBUG)

static VKAPI_ATTR VkBool32 VKAPI_CALL
debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
               VkDebugUtilsMessageTypeFlagsEXT message_type,
               const VkDebugUtilsMessengerCallbackDataEXT *callback_data, void *user_data)
{
    (void)message_severity;
    (void)message_type;
    (void)user_data;

    SDL_Log("[VULKAN] Debug Messenger: %s", callback_data->pMessage);

    return VK_FALSE;
}

static bool debug_messenger_init(VulkanContext *ctx)
{
	assert(ctx);
	
    VkDebugUtilsMessengerCreateInfoEXT info = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = debug_callback,
    };

    PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT =(PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(ctx->instance, "vkCreateDebugUtilsMessengerEXT");
    if (!vkCreateDebugUtilsMessengerEXT)
    {
        SDL_Log("[VULKAN] Failed to load debug utils messenger function pointer.");
        return false;
    }

    if (vkCreateDebugUtilsMessengerEXT(ctx->instance, &info, NULL, &ctx->debug_messenger) != VK_SUCCESS)
    {
        SDL_Log("[VULKAN] Failed to create debug messenger.");
        return false;
    }

    return true;
}

#endif // DEBUG

static bool surface_init(SDL_Window *window, VulkanContext *ctx)
{
	assert(window);
	assert(ctx);
	
	if (!SDL_Vulkan_CreateSurface(window, ctx->instance, NULL, &ctx->surface))
	{
		SDL_Log("[VULKAN] Failed to create surface.");
		return false;
	}
	return true;
}

static bool physical_device_init(VulkanContext *ctx)
{
	assert(ctx);

	u32 physcial_device_count;
	vkEnumeratePhysicalDevices(ctx->instance, &physcial_device_count, NULL);

	if (physcial_device_count == 0)
	{
		SDL_Log("[VULKAN] Didnt find any fitting GPU which supports vulkan.");
		return false;
	}

	// NOTE: Who has more than 12 GPUs?
	VkPhysicalDevice devs[12];
	vkEnumeratePhysicalDevices(ctx->instance, &physcial_device_count, devs);

	if (physcial_device_count > ARRAY_COUNT(devs))
	{
		SDL_Log("[VULKAN] Warning: You have (%u/%lu) GPUs only the first %lu will be examed.", physcial_device_count, ARRAY_COUNT(devs), ARRAY_COUNT(devs));
		physcial_device_count = ARRAY_COUNT(devs);
	}

	for (u32 i = 0; i < physcial_device_count; i++)
	{
		VkPhysicalDevice dev = devs[i];

		u32 queue_count;
		vkGetPhysicalDeviceQueueFamilyProperties(dev, &queue_count, NULL);

		VkQueueFamilyProperties props[8];
		vkGetPhysicalDeviceQueueFamilyProperties(dev, &queue_count, props);
		queue_count = MIN(queue_count, ARRAY_COUNT(props));

		i32 graphics_index = -1;
		i32 present_index = -1;

		for (u32 j = 0; j < queue_count; j++)
		{
			if (props[j].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				graphics_index = j;
			}

			VkBool32 supported = VK_FALSE;
			vkGetPhysicalDeviceSurfaceSupportKHR(dev, j, ctx->surface, &supported);

			if (supported)
			{
				present_index = j;
				if (graphics_index != -1)
				{
					break;
				}
			}
		}

		if (graphics_index != -1 && present_index != -1)
		{
			ctx->physical_device = dev;
			ctx->graphics_queue.index = graphics_index;
			ctx->present_queue.index = present_index;

			break;
		}
		else if (i == physcial_device_count - 1)
		{
			SDL_Log("[VULKAN] Failed to find sutable GPU.");
			return false;
		}
	}

	VkPhysicalDeviceProperties props;
	vkGetPhysicalDeviceProperties(ctx->physical_device, &props);

	SDL_Log("[VULKAN] Picked GPU: %s", props.deviceName);

	return true;
}

static bool logical_device_init(VulkanContext *ctx)
{
	assert(ctx);
	
	VkDeviceQueueCreateInfo queue_infos[2];

	float priority = 1.0f;

	u32 queue_count = 0;
	queue_infos[queue_count++] = (VkDeviceQueueCreateInfo){
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.queueFamilyIndex = ctx->graphics_queue.index,
		.queueCount = 1,
		.pQueuePriorities = &priority,
	};

	if (ctx->graphics_queue.index != ctx->present_queue.index)
	{
		queue_infos[queue_count++] = (VkDeviceQueueCreateInfo){
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.queueFamilyIndex = ctx->present_queue.index,
			.queueCount = 1,
			.pQueuePriorities = &priority,
		};
	}

	const char *device_extensions[] = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,	
	};

	VkPhysicalDeviceDynamicRenderingFeatures dynamic_rendering = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES,
		.dynamicRendering = VK_TRUE,	
	};

	VkDeviceCreateInfo info = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = &dynamic_rendering,
		.queueCreateInfoCount = queue_count,
		.pQueueCreateInfos = queue_infos,
		.enabledExtensionCount = ARRAY_COUNT(device_extensions),
		.ppEnabledExtensionNames = device_extensions,
	};

	if (vkCreateDevice(ctx->physical_device, &info, NULL, &ctx->device) != VK_SUCCESS)
	{
		SDL_Log("[VULKAN] Failed to create logical device.");
		return false;
	}

	return true;
}

bool vulkan_init(SDL_Window *window, VulkanContext *ctx)
{
	assert(window);
	assert(ctx);

#define CHECK(x) \
	if (!x) \
	{ \
		return false; \
	}

	CHECK(instance_init(ctx));

#if defined(DEBUG)
	CHECK(debug_messenger_init(ctx));
#endif
	
	CHECK(surface_init(window, ctx));
	CHECK(physical_device_init(ctx));
	CHECK(logical_device_init(ctx));
	CHECK(vulkan_swapchain_init(ctx, &ctx->swapchain, 600, 600));
	CHECK(vulkan_pipeline_init(ctx, &ctx->triangle_pipeline));

#undef CHECK

	return true;
}

void vulkan_deinit(VulkanContext *ctx)
{
	assert(ctx);

	vkDeviceWaitIdle(ctx->device);

	vulkan_pipeline_deinit(ctx, &ctx->triangle_pipeline);
	vulkan_swapchain_deinit(ctx, &ctx->swapchain);

	vkDestroySurfaceKHR(ctx->instance, ctx->surface, NULL);
	vkDestroyDevice(ctx->device, NULL);

#if defined(DEBUG)
	PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(ctx->instance, "vkDestroyDebugUtilsMessengerEXT");
	if (vkDestroyDebugUtilsMessengerEXT)
	{
		vkDestroyDebugUtilsMessengerEXT(ctx->instance, ctx->debug_messenger, NULL);
	}
#endif
	
	vkDestroyInstance(ctx->instance, NULL);
}
