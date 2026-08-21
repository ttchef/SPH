
#pragma once

#include <math/types.h>
#include <types.h>
#include <vk/types.h>

#include <volk.h>

// TODO: no sph in here
#include <sph/memory.h>

typedef enum
{
    VULKAN_IMAGE_TYPE_2D,
    VULKAN_IMAGE_TYPE_CUBE,
} vulkan_image_type;

typedef struct
{
    vulkan_image_type type;
    
    VkImage        handle;
    VkImageView    view;
    VkDeviceMemory memory;

    u32 width;
    u32 height;

    // NOTE: VULKAN_INVALID_BINDING by default
    union
    {
        vulkan_bindless_image image_2d;
        vulkan_bindless_cube_image image_cube;
    } descriptor;
} vulkan_image;

typedef struct
{
    VkSampler handle;
    // NOTE: VULKAN_INVALID_BINDING by default
    vulkan_bindless_sampler descriptor;
} vulkan_sampler;

typedef struct
{
    VkImageLayout        layout;
    VkAccessFlags        access;
    VkPipelineStageFlags stage;
    VkImageAspectFlags   aspect;
} vulkan_image_info;

bool vulkan_image_create(vulkan *vulkan, v2u dimensions, VkFormat format, VkImageUsageFlags usage, VkImageAspectFlags aspect, const char *name, vulkan_image *out_image);

bool vulkan_image_cube_create(vulkan *vulkan, u32 face_size, VkFormat format, VkImageUsageFlags usage, const char *name, vulkan_image *out_image);

void vulkan_image_destroy(vulkan *vulkan, vulkan_image *image);

bool vulkan_image_transition(vulkan *vulkan, memory_arena *arena, vulkan_image image, vulkan_image_info src, vulkan_image_info dst);

bool vulkan_image_data_upload(vulkan *vulkan, memory_arena *arena, vulkan_image image, u32 size, void *data, vulkan_image_info src, vulkan_image_info dst);

bool vulkan_image_cube_data_upload(vulkan *vulkan, memory_arena *arena, vulkan_image image, u32 size, void *data, u32 face, vulkan_image_info src, vulkan_image_info dst);

bool vulkan_sampler_create(vulkan *vulkan, const char *name, vulkan_sampler *out_sampler);

void vulkan_sampler_destroy(vulkan *vulkan, vulkan_sampler *sampler);
