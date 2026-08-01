
#pragma once

#include <types.h>
#include <vk/buffer.h>
#include <vk/image.h>
#include <vk/types.h>

#include <vulkan/vulkan_core.h>

//
// NOTE: Bindless setup
//       binding 0 -> ubo
//       binding 1 -> sampled image
//       binding 2 -> samplers
//

#define VULKAN_MAX_SAMPLED_IMAGE_COUNT 256
#define VULKAN_MAX_SAMPLER_COUNT       256
#define VULKAN_INVALID_BINDING         0

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
} vulkan_bindless;

bool vulkan_bindless_create(vulkan *vulkan, u32 ubo_size, vulkan_bindless *out_bindless);

void vulkan_bindless_destroy(vulkan *vulkan, vulkan_bindless *bindless);

void *vulkan_bindless_ubo_get(vulkan_bindless *bindless);

// NOTE: Image
void vulkan_bindless_image_aquire(vulkan *vulkan, vulkan_bindless *bindless, vulkan_image *image);

void vulkan_bindless_image_release(vulkan_bindless *bindless, vulkan_bindless_image handle);

// ----

// NOTE: Sampler
void vulkan_bindless_sampler_aquire(vulkan *vulkan, vulkan_bindless *bindless, vulkan_sampler *sampler);

void vulkan_bindless_sampler_release(vulkan_bindless *bindless, vulkan_bindless_sampler handle);

// ----

//
// NOTE: Descriptor buffer
// 

typedef struct
{
    vulkan_buffer ssbo_descriptor;
    u32 ssbo_offset;
} vulkan_descriptor_buffers;

bool vulkan_descriptor_buffers_create(vulkan *vulkan, vulkan_descriptor_buffers *out_descriptor_buffers);

void vulkan_descriptor_buffer_bind(vulkan *vulkan, vulkan_descriptor_buffers *descriptor_buffers, VkCommandBuffer command_buffer);

void vulkan_descriptor_buffers_destroy(vulkan *vulkan, vulkan_descriptor_buffers *descriptor_buffers);

//
// NOTE: Public descriptor buffer api
// 

void vulkan_descriptor_add_ssbo(vulkan *vulkan, vulkan_buffer *buffer);
