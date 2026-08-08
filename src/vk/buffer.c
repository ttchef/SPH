
#include <vk/buffer.h>
#include <vk/context.h>
#include <vk/utils.h>

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

bool vulkan_buffer_device_local_create(vulkan *vulkan, memory_arena *arena, VkBufferUsageFlags usage, u32 size, const void *data, const char *name, vulkan_buffer *out_buffer)
{
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

    vulkan_command_queue *queue = vulkan_command_begin(arena);
    vulkan_command_copy_buffer(queue, staging, result);
    if (!vulkan_command_end(queue, vulkan, true))
    {
        return false;
    }

    *out_buffer = result;

    vulkan_buffer_destroy(vulkan, &staging);

    return true;
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

bool vulkan_buffer_device_local_get_data(vulkan *vulkan, memory_arena *arena, vulkan_buffer buffer, const char *name, vulkan_buffer *out_buffer)
{
    vkDeviceWaitIdle(vulkan->device);

    vulkan_buffer result = {0};
    if (!buffer_create(vulkan, VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT, buffer.size, name, false, &result))
    {
        return false;
    }
    result.type = VULKAN_BUFFER_TYPE_HOST_VISIBLE;

    vulkan_command_queue *queue = vulkan_command_begin(arena);
    vulkan_command_copy_buffer(queue, buffer, result);
    if (!vulkan_command_end(queue, vulkan, true))
    {
        SDL_Log("[VULKAN] Failed to get device local buffer data.");
        return false;
    }

    vkMapMemory(vulkan->device, result.memory, 0, buffer.size, 0, &result.host_visible.data);

    *out_buffer = result;

    return true;
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
