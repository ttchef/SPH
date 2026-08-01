
#include <vk/buffer.h>
#include <vk/context.h>
#include <vk/utils.h>
#include <vulkan/vulkan_core.h>

static bool buffer_create(vulkan *vulkan, VkBufferUsageFlags usage, VkMemoryPropertyFlags memory_properties, u32 size, const char *name, bool device_address, vulkan_buffer *out_buffer)
{
    assert(vulkan);
    assert(out_buffer);

    // NOTE: For release
    UNUSED(name);

    vulkan_buffer result = {0};

    result.size = size;

    VkBufferCreateInfo buffer_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size  = size,
        .usage = usage,
    };

    if (vkCreateBuffer(vulkan->device, &buffer_info, NULL, &result.handle) != VK_SUCCESS)
    {
        SDL_Log("[VULKAN] Failed to create device local buffer of size: %u", size);
        return false;
    }

    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(vulkan->device, result.handle, &memory_requirements);

    u32 memory_index = vulkan_memory_type_find(vulkan, memory_requirements.memoryTypeBits, memory_properties);
    if (memory_index == UINT32_MAX)
    {
        SDL_Log("[WARNING] Failed to find specific memory.");
        return false;
    }

    VkMemoryAllocateFlagsInfo flags_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
        .flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT,
    };

    VkMemoryAllocateInfo alloc_info = {
        .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext           = device_address ? &flags_info : NULL,
        .allocationSize  = memory_requirements.size,
        .memoryTypeIndex = memory_index,
    };

    if (vkAllocateMemory(vulkan->device, &alloc_info, NULL, &result.memory) != VK_SUCCESS)
    {
        SDL_Log("[VULKAN] Failed to allocate device memory.");
        return false;
    }

    if (vkBindBufferMemory(vulkan->device, result.handle, result.memory, 0) != VK_SUCCESS)
    {
        SDL_Log("[VULKAN] Failed to bind memory to buffer.");
        return false;
    }

    vulkan_object_name_set(vulkan, VK_OBJECT_TYPE_DEVICE_MEMORY, (u64)result.memory, "memory:%s", name);
    vulkan_object_name_set(vulkan, VK_OBJECT_TYPE_BUFFER, (u64)result.handle, "buffer:%s", name);

    *out_buffer = result;

    return true;
}

bool vulkan_buffer_device_local_create(vulkan *vulkan, VkBufferUsageFlags usage, u32 size, const void *data, const char *name, vulkan_buffer *out_buffer)
{
    assert(vulkan);
    assert(out_buffer);

    vulkan_buffer result = {0};

    bool device_address = usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    if (!buffer_create(vulkan, usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, size, name, device_address, &result))
    {
        return false;
    }

    result.type = VULKAN_BUFFER_TYPE_DEVICE_LOCAL;

    // NOTE: No data to copy into the buffer
    if (!data)
    {
        *out_buffer = result;
        return true;
    }

    vulkan_buffer staging = {0};
    if (!buffer_create(vulkan, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, size, "staging", false, &staging))
    {
        return false;
    }

    void *mapped;
    vkMapMemory(vulkan->device, staging.memory, 0, size, 0, &mapped);
    SDL_memcpy(mapped, data, size);
    vkUnmapMemory(vulkan->device, staging.memory);

    VkCommandPool   command_pool;
    VkCommandBuffer command_buffer;

    VkCommandPoolCreateInfo pool_info = {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags            = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
        .queueFamilyIndex = vulkan->graphics_queue.index,
    };

    if (vkCreateCommandPool(vulkan->device, &pool_info, NULL, &command_pool) != VK_SUCCESS)
    {
        SDL_Log("[VULKAN] Failed to create command pool.");
        vulkan_buffer_destroy(vulkan, &staging);
        return false;
    }

    VkCommandBufferAllocateInfo alloc_info = {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool        = command_pool,
        .commandBufferCount = 1,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    };

    if (vkAllocateCommandBuffers(vulkan->device, &alloc_info, &command_buffer) != VK_SUCCESS)
    {
        goto error;
    }

    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };

    if (vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS)
    {
        goto error;
    }

    VkBufferCopy region = {
        .size = size,
    };

    vkCmdCopyBuffer(command_buffer, staging.handle, result.handle, 1, &region);

    if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS)
    {
        goto error;
    }

    VkSubmitInfo submit_info = {
        .sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers    = &command_buffer,
    };

    if (vkQueueSubmit(vulkan->graphics_queue.handle, 1, &submit_info, VK_NULL_HANDLE) != VK_SUCCESS)
    {
        goto error;
    }

    if (vkQueueWaitIdle(vulkan->graphics_queue.handle))
    {
        goto error;
    }

    *out_buffer = result;

    vkDestroyCommandPool(vulkan->device, command_pool, NULL);
    vulkan_buffer_destroy(vulkan, &staging);

    return true;

error:
    vkDestroyCommandPool(vulkan->device, command_pool, NULL);
    vulkan_buffer_destroy(vulkan, &staging);

    SDL_Log("[VULKAN] Error occured while creating device local buffer.");

    return false;
}

bool vulkan_buffer_host_visible_create(vulkan *vulkan, VkBufferUsageFlags usage, u32 size, const void *data, const char *name, vulkan_buffer *out_buffer)
{
    vulkan_buffer result = {0};

    result.type = VULKAN_BUFFER_TYPE_HOST_VISIBLE;

    bool device_address = usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    if (!buffer_create(vulkan, usage, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, size, name, device_address, &result))
    {
        return false;
    }

    if (vkMapMemory(vulkan->device, result.memory, 0, result.size, 0, &result.host_visible.data) != VK_SUCCESS)
    {
        SDL_Log("[VULKAN] Failed to map host visible buffer memory.");
        return false;
    }

    *out_buffer = result;

    if (data)
    {
        SDL_memcpy(out_buffer->host_visible.data, data, size);
    }

    return true;
}

bool vulkan_buffer_device_local_get_data(vulkan *vulkan, vulkan_buffer buffer, const char *name, vulkan_buffer *out_buffer)
{
    vkDeviceWaitIdle(vulkan->device);

    vulkan_buffer result = {0};
    if (!buffer_create(vulkan, VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT, buffer.size, name, false, &result))
    {
        return false;
    }

    result.type = VULKAN_BUFFER_TYPE_HOST_VISIBLE;

    VkCommandPool   command_pool;
    VkCommandBuffer command_buffer;

    VkCommandPoolCreateInfo pool_info = {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags            = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
        .queueFamilyIndex = vulkan->graphics_queue.index,
    };

    if (vkCreateCommandPool(vulkan->device, &pool_info, NULL, &command_pool) != VK_SUCCESS)
    {
        SDL_Log("[VULKAN] Failed to create command pool.");
        vulkan_buffer_destroy(vulkan, &result);
        return false;
    }

    VkCommandBufferAllocateInfo alloc_info = {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool        = command_pool,
        .commandBufferCount = 1,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    };

    if (vkAllocateCommandBuffers(vulkan->device, &alloc_info, &command_buffer) != VK_SUCCESS)
    {
        goto error;
    }

    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };

    if (vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS)
    {
        goto error;
    }

    VkBufferCopy region = {
        .size = buffer.size,
    };

    vkCmdCopyBuffer(command_buffer, buffer.handle, result.handle, 1, &region);

    if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS)
    {
        goto error;
    }

    VkSubmitInfo submit_info = {
        .sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers    = &command_buffer,
    };

    if (vkQueueSubmit(vulkan->graphics_queue.handle, 1, &submit_info, VK_NULL_HANDLE) != VK_SUCCESS)
    {
        goto error;
    }

    if (vkQueueWaitIdle(vulkan->graphics_queue.handle))
    {
        goto error;
    }

    vkDestroyCommandPool(vulkan->device, command_pool, NULL);

    vkMapMemory(vulkan->device, result.memory, 0, buffer.size, 0, &result.host_visible.data);

    *out_buffer = result;

    return true;

error:
    vkDestroyCommandPool(vulkan->device, command_pool, NULL);
    vulkan_buffer_destroy(vulkan, &result);

    SDL_Log("[VULKAN] Error occured while getting device local data.");

    return false;
}

VkDeviceAddress vulkan_buffer_address_get(vulkan *vulkan, vulkan_buffer buffer)
{
    VkBufferDeviceAddressInfo addr_info = {
        .sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .buffer = buffer.handle,
    };

    return vkGetBufferDeviceAddress(vulkan->device, &addr_info);
}

void vulkan_buffer_destroy(vulkan *vulkan, vulkan_buffer *buffer)
{
    assert(vulkan);
    assert(buffer);

    vkDeviceWaitIdle(vulkan->device);

    if (buffer->type == VULKAN_BUFFER_TYPE_HOST_VISIBLE && buffer->host_visible.data)
    {
        vkUnmapMemory(vulkan->device, buffer->memory);
    }

    vkDestroyBuffer(vulkan->device, buffer->handle, NULL);
    vkFreeMemory(vulkan->device, buffer->memory, NULL);
}
