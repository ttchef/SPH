
#include <vk/buffer.h>
#include <vk/context.h>
#include <vk/image.h>
#include <vk/utils.h>

#include <SDL3/SDL_log.h>

bool vulkan_image_create(vulkan *vulkan, v2u dimensions, VkFormat format, VkImageUsageFlags usage, VkImageAspectFlags aspect, const char *name, vulkan_image *out_image)
{
    // NOTE: For release
    UNUSED(name);

    vulkan_image result = {0};

    result.type = VULKAN_IMAGE_TYPE_2D;
    result.width  = dimensions.x;
    result.height = dimensions.y;

    VkImageCreateInfo image_info = {
        .sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType     = VK_IMAGE_TYPE_2D,
        .extent.width  = dimensions.x,
        .extent.height = dimensions.y,
        .extent.depth  = 1,
        .mipLevels     = 1,
        .arrayLayers   = 1,
        .format        = format,
        .tiling        = VK_IMAGE_TILING_OPTIMAL,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .usage         = usage,
        .samples       = VK_SAMPLE_COUNT_1_BIT,
        .sharingMode   = VK_SHARING_MODE_EXCLUSIVE,
    };

    if (vkCreateImage(vulkan->device, &image_info, NULL, &result.handle) != VK_SUCCESS)
    {
        SDL_Log("[VULKAN] Failed to create image.");
        return false;
    }

    VkMemoryRequirements memory_requirements;
    vkGetImageMemoryRequirements(vulkan->device, result.handle, &memory_requirements);

    VkMemoryAllocateInfo alloc_info = {
        .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize  = memory_requirements.size,
        .memoryTypeIndex = vulkan_memory_type_find(vulkan, memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
    };

    if (vkAllocateMemory(vulkan->device, &alloc_info, NULL, &result.memory) != VK_SUCCESS)
    {
        SDL_Log("[VULKAN] Failed to allocate image memory.");
        return false;
    }

    if (vkBindImageMemory(vulkan->device, result.handle, result.memory, 0) != VK_SUCCESS)
    {
        SDL_Log("[VULKAN] Failed to bind image memory.");
        return false;
    }

    VkImageViewCreateInfo view_info = {
        .sType                       = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image                       = result.handle,
        .viewType                    = VK_IMAGE_VIEW_TYPE_2D,
        .format                      = format,
        .subresourceRange.aspectMask = aspect,
        .subresourceRange.layerCount = 1,
        .subresourceRange.levelCount = 1,
    };

    if (vkCreateImageView(vulkan->device, &view_info, NULL, &result.view) != VK_SUCCESS)
    {
        SDL_Log("[VULKAN] Failed to create image view.");
        return false;
    }

    vulkan_object_name_set(vulkan, VK_OBJECT_TYPE_IMAGE_VIEW, (u64)result.view, "image_view:%s", name);
    vulkan_object_name_set(vulkan, VK_OBJECT_TYPE_IMAGE, (u64)result.handle, "image:%s", name);

    *out_image = result;

    return true;
}

bool vulkan_image_cube_create(vulkan *vulkan, u32 face_size, VkFormat format, VkImageUsageFlags usage, const char *name, vulkan_image *out_image)
{
    // NOTE: For relase
    UNUSED(name);

    vulkan_image result = {0};

    result.type = VULKAN_IMAGE_TYPE_CUBE;
    result.width  = face_size;
    result.height = face_size;

    VkImageCreateInfo image_info = {
        .sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType     = VK_IMAGE_TYPE_2D,
        .extent.width  = face_size,
        .extent.height = face_size,
        .extent.depth  = 1,
        .mipLevels     = 1,
        .arrayLayers   = 6,
        .format        = format,
        .tiling        = VK_IMAGE_TILING_OPTIMAL,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .usage         = usage,
        .samples       = VK_SAMPLE_COUNT_1_BIT,
        .sharingMode   = VK_SHARING_MODE_EXCLUSIVE,
        .flags         = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
    };

    if (vkCreateImage(vulkan->device, &image_info, NULL, &result.handle) != VK_SUCCESS)
    {
        SDL_Log("[VULKAN] Failed to create image.");
        return false;
    }

    VkMemoryRequirements memory_requirements;
    vkGetImageMemoryRequirements(vulkan->device, result.handle, &memory_requirements);

    VkMemoryAllocateInfo alloc_info = {
        .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize  = memory_requirements.size,
        .memoryTypeIndex = vulkan_memory_type_find(vulkan, memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
    };

    if (vkAllocateMemory(vulkan->device, &alloc_info, NULL, &result.memory) != VK_SUCCESS)
    {
        SDL_Log("[VULKAN] Failed to allocate image memory.");
        return false;
    }

    if (vkBindImageMemory(vulkan->device, result.handle, result.memory, 0) != VK_SUCCESS)
    {
        SDL_Log("[VULKAN] Failed to bind image memory.");
        return false;
    }

    VkImageViewCreateInfo view_info = {
        .sType                       = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image                       = result.handle,
        .viewType                    = VK_IMAGE_VIEW_TYPE_CUBE,
        .format                      = format,
        .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS,
        .subresourceRange.levelCount = 1,
    };

    if (vkCreateImageView(vulkan->device, &view_info, NULL, &result.view) != VK_SUCCESS)
    {
        SDL_Log("[VULKAN] Failed to create image view.");
        return false;
    }

    vulkan_object_name_set(vulkan, VK_OBJECT_TYPE_IMAGE_VIEW, (u64)result.view, "image_view:%s", name);
    vulkan_object_name_set(vulkan, VK_OBJECT_TYPE_IMAGE, (u64)result.handle, "image:%s", name);

    *out_image = result;

    return true;
}

void vulkan_image_destroy(vulkan *vulkan, vulkan_image *image)
{
    vkDestroyImageView(vulkan->device, image->view, NULL);
    vkDestroyImage(vulkan->device, image->handle, NULL);
    vkFreeMemory(vulkan->device, image->memory, NULL);
}

bool vulkan_image_transition(vulkan *vulkan, memory_arena *arena, vulkan_image image, vulkan_image_info src, vulkan_image_info dst)
{
    vulkan_command_queue *queue = vulkan_command_begin(arena);
    vulkan_command_image_barrier(queue, image, src, dst);
    if (!vulkan_command_end(queue, vulkan, true))
    {
        return false;
    }

    return true;
}

bool vulkan_image_data_upload(vulkan *vulkan, memory_arena *arena, vulkan_image image, u32 size, void *data, vulkan_image_info src, vulkan_image_info dst)
{
    vulkan_buffer staging;
    if (!vulkan_buffer_host_visible_create(vulkan, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, size, data, "staging", &staging))
    {
        SDL_Log("[VULKAN] Failed to create staging buffer when uploading data to image.");
        return false;
    }

    vulkan_image_info copy_info = {
        .layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .access = VK_ACCESS_TRANSFER_WRITE_BIT,
        .stage  = VK_PIPELINE_STAGE_TRANSFER_BIT,
        .aspect = src.aspect,
    };

    vulkan_command_queue *queue = vulkan_command_begin(arena);

    vulkan_command_image_barrier(queue, image, src, copy_info);
    vulkan_command_copy_image(queue, staging, image, copy_info, 0);
    vulkan_command_image_barrier(queue, image, copy_info, dst);

    if (!vulkan_command_end(queue, vulkan, true))
    {
        vulkan_buffer_destroy(vulkan, &staging);
        return false;
    }
    vulkan_buffer_destroy(vulkan, &staging);

    return true;
}

bool vulkan_image_cube_data_upload(vulkan *vulkan, memory_arena *arena, vulkan_image image, u32 size, void *data, u32 face, vulkan_image_info src, vulkan_image_info dst)
{
    vulkan_buffer staging;
    if (!vulkan_buffer_host_visible_create(vulkan, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, size, data, "staging", &staging))
    {
        SDL_Log("[VULKAN] Failed to create staging buffer when uploading data to image.");
        return false;
    }

    vulkan_image_info copy_info = {
        .layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .access = VK_ACCESS_TRANSFER_WRITE_BIT,
        .stage  = VK_PIPELINE_STAGE_TRANSFER_BIT,
        .aspect = src.aspect,
    };

    vulkan_command_queue *queue = vulkan_command_begin(arena);

    vulkan_command_image_barrier(queue, image, src, copy_info);
    vulkan_command_copy_image(queue, staging, image, copy_info, face);
    vulkan_command_image_barrier(queue, image, copy_info, dst);

    if (!vulkan_command_end(queue, vulkan, true))
    {
        vulkan_buffer_destroy(vulkan, &staging);
        return false;
    }
    vulkan_buffer_destroy(vulkan, &staging);

    return true;
}

bool vulkan_sampler_create(vulkan *vulkan, const char *name, vulkan_sampler *out_sampler)
{
    // NOTE: For release
    UNUSED(name);

    vulkan_sampler result = {0};

    VkSamplerCreateInfo info = {
        .sType         = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .addressModeU  = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeV  = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeW  = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .mipmapMode    = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .minLod        = 0.0f,
        .maxLod        = 1.0f,
        .maxAnisotropy = 1.0f,
        .minFilter     = VK_FILTER_LINEAR,
        .magFilter     = VK_FILTER_LINEAR,
    };

    if (vkCreateSampler(vulkan->device, &info, NULL, &result.handle) != VK_SUCCESS)
    {
        SDL_Log("[VULKAN] Failed to create sampler.");
        return false;
    }

    vulkan_object_name_set(vulkan, VK_OBJECT_TYPE_SAMPLER, (u64)result.handle, "sampler:%s", name);

    *out_sampler = result;

    return true;
}

void vulkan_sampler_destroy(vulkan *vulkan, vulkan_sampler *sampler)
{
    vkDestroySampler(vulkan->device, sampler->handle, NULL);
}
