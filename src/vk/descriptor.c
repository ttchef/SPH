
#include <vk/context.h>
#include <vk/descriptor.h>

#include <SDL3/SDL_log.h>
#include <vulkan/vulkan_core.h>

enum
{
    UBO_BINDING           = 0,
    SAMPLED_IMAGE_BINDING = 1,
    SAMPLER_BINDING       = 2,
};

bool vulkan_bindless_create(vulkan *vulkan, u32 ubo_size, vulkan_bindless *out_bindless)
{
    vulkan_bindless result = {0};

    VkDescriptorPoolSize sizes[] = {
        {
            .type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
        },
        {
            .type            = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            .descriptorCount = VULKAN_MAX_SAMPLED_IMAGE_COUNT,
        },
        {
            .type            = VK_DESCRIPTOR_TYPE_SAMPLER,
            .descriptorCount = VULKAN_MAX_SAMPLER_COUNT,
        }};

    VkDescriptorPoolCreateInfo pool_info = {
        .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets       = 1,
        .flags         = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
        .pPoolSizes    = sizes,
        .poolSizeCount = ARRAY_COUNT(sizes),
    };

    if (vkCreateDescriptorPool(vulkan->device, &pool_info, NULL, &result.pool) != VK_SUCCESS)
    {
        SDL_Log("[VULKAN] Failed to create descriptor pool.");
        return false;
    }

    VkDescriptorSetLayoutBinding bindings[] = {
        {
            .binding         = UBO_BINDING,
            .descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_ALL,
        },
        {
            .binding         = SAMPLED_IMAGE_BINDING,
            .descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            .descriptorCount = VULKAN_MAX_SAMPLED_IMAGE_COUNT,
            // TODO: Is is only fragment??
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        },
        {
            .binding         = SAMPLER_BINDING,
            .descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLER,
            .descriptorCount = VULKAN_MAX_SAMPLER_COUNT,
            // TODO: Is is only fragment??
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        }};

    VkDescriptorBindingFlags flags[] = {
        0,
        VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT,
        VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT,
    };

    static_assert(ARRAY_COUNT(flags) == ARRAY_COUNT(bindings), "binding and flags are not the same lenght.");

    VkDescriptorSetLayoutBindingFlagsCreateInfo flags_info = {
        .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
        .bindingCount  = ARRAY_COUNT(bindings),
        .pBindingFlags = flags,
    };

    VkDescriptorSetLayoutCreateInfo layout_info = {
        .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext        = &flags_info,
        .flags        = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT,
        .pBindings    = bindings,
        .bindingCount = ARRAY_COUNT(bindings),
    };

    if (vkCreateDescriptorSetLayout(vulkan->device, &layout_info, NULL, &result.layout) != VK_SUCCESS)
    {
        SDL_Log("[VULKAN] Failed to create descriptor set layout.");
        return false;
    }

    VkDescriptorSetAllocateInfo alloc_info = {
        .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool     = result.pool,
        .pSetLayouts        = &result.layout,
        .descriptorSetCount = 1,
    };

    if (vkAllocateDescriptorSets(vulkan->device, &alloc_info, &result.set) != VK_SUCCESS)
    {
        SDL_Log("[VULKAn] Failed to allocate descriptor sets.");
        return false;
    }

    result.free_image_count = VULKAN_MAX_SAMPLED_IMAGE_COUNT - 1;
    for (vulkan_bindless_image i = 0; i < result.free_image_count; i++)
    {
        result.free_images[i] = result.free_image_count - i;
    }

    result.free_sampler_count = VULKAN_MAX_SAMPLER_COUNT - 1;
    for (vulkan_bindless_sampler i = 0; i < result.free_sampler_count; i++)
    {
        result.free_samplers[i] = result.free_sampler_count - i;
    }

    // NOTE: Ubo
    if (!vulkan_buffer_host_visible_create(vulkan, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, ubo_size, NULL, &result.ubo))
    {
        SDL_Log("[VULKAN] Failed to create descriptor ubo.");
        return false;
    }

    VkDescriptorBufferInfo ubo_info = {
        .buffer = result.ubo.handle,
        .range = result.ubo.size,  
        .offset = 0,
    };

    VkWriteDescriptorSet ubo_write = {
          .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          .dstBinding = UBO_BINDING,  
          .descriptorCount = 1,
          .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
          .dstArrayElement = 0,
          .dstSet = result.set,
          .pBufferInfo = &ubo_info,
    };

    vkUpdateDescriptorSets(vulkan->device, 1, &ubo_write, 0, NULL);

    *out_bindless = result;

    return true;
}

void vulkan_bindless_destroy(vulkan *vulkan, vulkan_bindless *bindless)
{
    vulkan_buffer_destroy(vulkan, &bindless->ubo);
    vkDestroyDescriptorSetLayout(vulkan->device, bindless->layout, NULL);
    vkDestroyDescriptorPool(vulkan->device, bindless->pool, NULL);
}

void *vulkan_bindless_ubo_get(vulkan_bindless *bindless)
{
    assert(bindless->ubo.host_visible.data);
    return bindless->ubo.host_visible.data;
}

void vulkan_bindless_image_aquire(vulkan *vulkan, vulkan_bindless *bindless, vulkan_image *image)
{
    if (image->descriptor == VULKAN_INVALID_BINDING)
    {
        if (bindless->free_image_count == 0)
        {
            SDL_Log("[VULKAN] No free sampled images left to allocate.");
        }

        image->descriptor = bindless->free_images[--bindless->free_image_count];
        assert(image->descriptor != VULKAN_INVALID_BINDING);
    }

    // NOTE: if image is passed in write it to set
    VkDescriptorImageInfo image_info = {
        .imageLayout = image->layout,
        .imageView   = image->view,
    };

    VkWriteDescriptorSet write = {
        .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
        .dstArrayElement = image->descriptor,
        .descriptorCount = 1,
        .dstBinding      = SAMPLED_IMAGE_BINDING,
        .dstSet          = bindless->set,
        .pImageInfo      = &image_info,
    };

    vkUpdateDescriptorSets(vulkan->device, 1, &write, 0, NULL);
}

void vulkan_bindless_image_release(vulkan_bindless *bindless, vulkan_bindless_image handle)
{
    bindless->free_images[bindless->free_image_count++] = handle;
}

void vulkan_bindless_sampler_aquire(vulkan *vulkan, vulkan_bindless *bindless, vulkan_sampler *sampler)
{
    if (sampler->descriptor == VULKAN_INVALID_BINDING)
    {
        if (bindless->free_sampler_count == 0)
        {
            SDL_Log("[VULKAN] No free samplers left to allocate.");
            return;
        }

        sampler->descriptor = bindless->free_samplers[--bindless->free_sampler_count];
        assert(sampler->descriptor != VULKAN_INVALID_BINDING);
    }

    // NOTE: if image is passed in write it to set
    VkDescriptorImageInfo sampler_info = {
        .sampler = sampler->handle,
    };

    VkWriteDescriptorSet write = {
        .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLER,
        .dstArrayElement = sampler->descriptor,
        .descriptorCount = 1,
        .dstBinding      = SAMPLER_BINDING,
        .dstSet          = bindless->set,
        .pImageInfo      = &sampler_info,
    };

    vkUpdateDescriptorSets(vulkan->device, 1, &write, 0, NULL);
}

void vulkan_bindless_sampler_release(vulkan_bindless *bindless, vulkan_bindless_sampler handle)
{
    bindless->free_samplers[bindless->free_sampler_count++] = handle;
}
