
#pragma once

#include <types.h>
#include <vk/context.h>

#include <SDL3/SDL_log.h>
#include <vulkan/vulkan_core.h>

// NOTE: returns UINT32_MAX if not found
static inline u32 vulkan_memory_type_find(vulkan *vulkan, u32 type_filter, VkMemoryPropertyFlags memory_properties)
{
    VkPhysicalDeviceMemoryProperties device_properties;
    vkGetPhysicalDeviceMemoryProperties(vulkan->physical_device, &device_properties);

    for (u32 i = 0; i < device_properties.memoryTypeCount; i++)
    {
        if ((type_filter & (1 << i)) != 0)
        {
            if ((device_properties.memoryTypes[i].propertyFlags & memory_properties) == memory_properties)
            {
                return i;
            }
        }
    }

    SDL_Log("[VULKAN] Warning: Couldnt find fitting memory type.");
    return UINT32_MAX;
}

//
// NOTE: Debug
//

#if defined(DEBUG)
static void vulkan_object_name_set(vulkan *vulkan, VkObjectType type, u64 handle, const char *format, ...)
{
    if (!vulkan->debug.vkSetDebugUtilsObjectNameEXT || handle == 0)
    {
        return;
    }

    static char name_buffer[256];
    va_list     args;
    va_start(args, format);
    SDL_vsnprintf(name_buffer, sizeof(name_buffer), format, args);
    va_end(args);

    VkDebugUtilsObjectNameInfoEXT info = {
        .sType        = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .objectType   = type,
        .objectHandle = handle,
        .pObjectName  = name_buffer,
    };
    vulkan->debug.vkSetDebugUtilsObjectNameEXT(vulkan->device, &info);
}
#else
#define vulkan_object_name_set(...) (void)0
#endif
