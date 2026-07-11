
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
		// .apiVersion = VK_API_VERSION_1_3, // NOTE: required?
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

bool vulkan_init(VulkanContext *ctx)
{
	assert(ctx);

#define CHECK(x) \
	if (!x) \
	{ \
		return false; \
	}

	CHECK(instance_init(ctx));

#undef CHECK

	return true;
}

void vulkan_deinit(VulkanContext *ctx)
{
	assert(ctx);	
}
