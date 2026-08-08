
#pragma once

#include <types.h>
#include <vk/buffer.h>
#include <vk/image.h>
#include <vk/types.h>

//
// NOTE: Bindless setup
//       binding 0 -> ubo
//       binding 1 -> sampled image
//       binding 2 -> samplers
//       binding 3 -> scene ubo
//

#define VULKAN_MAX_SAMPLED_IMAGE_COUNT 256
#define VULKAN_MAX_SAMPLER_COUNT       256
#define VULKAN_MAX_SCENE_UBO_COUNT     32
#define VULKAN_INVALID_BINDING         0

// NOTE: Can be changed later
#define VULKAN_MAX_SCENE_UBO_SIZE 2048

typedef struct
{
    VkDescriptorPool pool;
    VkDescriptorSet  set;

    // NOTE: bindless layout which every shader adobts
    VkDescriptorSetLayout layout;

    vulkan_buffer ubo;

    vulkan_bindless_image free_images[VULKAN_MAX_SAMPLED_IMAGE_COUNT];
    u32                   free_image_count;

    vulkan_bindless_sampler free_samplers[VULKAN_MAX_SAMPLER_COUNT];
    u32                     free_sampler_count;

    VkDescriptorSetLayout scene_layout;
    VkDescriptorSet       scene_set;

    vulkan_buffer             scene_ubo;
    u32                       scene_ubo_stride;
    vulkan_bindless_scene_ubo free_scene_ubos[VULKAN_MAX_SCENE_UBO_COUNT];
    u32                       free_scene_ubo_count;
} vulkan_bindless;

bool vulkan_bindless_create(vulkan *vulkan, u32 ubo_size, vulkan_bindless *out_bindless);

void vulkan_bindless_destroy(vulkan *vulkan, vulkan_bindless *bindless);

void *vulkan_bindless_ubo_get(vulkan_bindless *bindless);

//
// NOTE: Public api
//

// NOTE: Image
void vulkan_bindless_image_aquire(vulkan *vulkan, vulkan_image *image, VkImageLayout layout);

void vulkan_bindless_image_release(vulkan *vulkan, vulkan_bindless_image handle);

// ----

// NOTE: Sampler
void vulkan_bindless_sampler_aquire(vulkan *vulkan, vulkan_sampler *sampler);

void vulkan_bindless_sampler_release(vulkan *vulkan, vulkan_bindless_sampler handle);

// ----

// NOTE: Scene ubo
vulkan_bindless_scene_ubo vulkan_bindless_scene_ubo_aquire(vulkan *vulkan);

void vulkan_bindless_scene_ubo_release(vulkan *vulkan, vulkan_bindless_scene_ubo scene_ubo);

void *vulkan_bindless_scene_ubo_get(vulkan *vulkan, vulkan_bindless_scene_ubo scene_ubo);
// ---
