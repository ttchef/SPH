
#pragma once

#include <types.h>
#include <vk/types.h>
#include <vk/image.h>

#include <vulkan/vulkan_core.h>

//
// NOTE: Bindless setup
//       binding 0 -> sampled image
//       binding 1 -> samplers
//

#define VULKAN_MAX_SAMPLED_IMAGE_COUNT 256
#define VULKAN_MAX_SAMPLER_COUNT 256
#define VULKAN_INVALID_BINDING 0

typedef struct
{
    VkDescriptorPool pool;
    VkDescriptorSet  set;

    // NOTE: bindless laylout which every shader adobts
    VkDescriptorSetLayout layout;

    vulkan_bindless_image free_images[VULKAN_MAX_SAMPLED_IMAGE_COUNT];
    u32 free_image_count;

    vulkan_bindless_sampler free_samplers[VULKAN_MAX_SAMPLER_COUNT];
    u32 free_sampler_count;
} vulkan_bindless;

bool vulkan_bindless_create(vulkan *vulkan, vulkan_bindless *out_bindless);

void vulkan_bindless_destroy(vulkan *vulkan, vulkan_bindless *bindless);

// NOTE Image
void vulkan_bindless_image_aquire(vulkan *vulkan, vulkan_bindless *bindless, vulkan_image *image);

void vulkan_bindless_image_release(vulkan_bindless *bindless, vulkan_bindless_image handle);

// ----

// NOTE: Sampler
void vulkan_bindless_sampler_aquire(vulkan *vulkan, vulkan_bindless *bindless, vulkan_sampler *sampler);

void vulkan_bindless_sampler_release(vulkan_bindless *bindless, vulkan_bindless_sampler handle);

// ----
