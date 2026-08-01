
#include <vk/context.h>
#include <vk/destroy_queue.h>

#include <SDL3/SDL_log.h>

vulkan_destroy_queue vulkan_destroy_queue_create(void)
{
    vulkan_destroy_queue result = {0};

    result.free_entry_count = ARRAY_COUNT(result.entries);
    result.head             = 0;
    result.tail             = 0;

    return result;
}

void vulkan_destroy_queue_destroy(vulkan *vulkan, vulkan_destroy_queue *destroy_queue)
{
    destroy_queue->tail = (destroy_queue->tail + 1) % ARRAY_COUNT(destroy_queue->entries);

    // NOTE: <= only here!!
    while (destroy_queue->tail <= destroy_queue->head)
    {
        vulkan_destroy_entry *entry = &destroy_queue->entries[destroy_queue->tail];
        entry->destroy_func(vulkan, entry->data);
        SDL_free(entry->data);

        destroy_queue->tail = (destroy_queue->tail + 1) % ARRAY_COUNT(destroy_queue->entries);
    }
}

void vulkan_destroy_queue_flush(vulkan *vulkan, vulkan_destroy_queue *destroy_queue)
{
    if (destroy_queue->free_entry_count == ARRAY_COUNT(destroy_queue->entries))
    {
        return;
    }

    u32                   next  = (destroy_queue->tail + 1) % ARRAY_COUNT(destroy_queue->entries);
    vulkan_destroy_entry *entry = &destroy_queue->entries[next];

    u32 smallest_index = entry->retired_frame;

    if ((i32)vulkan->command_handler.accumulated_frame_index - (i32)smallest_index < FRAMES_IN_FLIGHT)
    {
        return;
    }

    while (entry->retired_frame == smallest_index && destroy_queue->tail != destroy_queue->head)
    {
        entry->destroy_func(vulkan, entry->data);
        SDL_free(entry->data);

        destroy_queue->tail = next;
        ++destroy_queue->free_entry_count;

        next  = (next + 1) % ARRAY_COUNT(destroy_queue->entries);
        entry = &destroy_queue->entries[next];
    }
}

// NOTE: Push to the queue or smth
void vulkan_object_destroy(vulkan *vulkan, u32 data_size, void *data, vulkan_destroy_func destroy_function)
{
    vulkan_destroy_queue *queue = &vulkan->destroy_queue;
    assert(queue->free_entry_count != 0);

    u32 index = (queue->head + 1) % ARRAY_COUNT(queue->entries);
    assert(index != queue->tail);

    vulkan_destroy_entry entry = {
        .data          = SDL_malloc(data_size),
        .destroy_func  = destroy_function,
        .retired_frame = vulkan->command_handler.accumulated_frame_index,
    };

    SDL_memcpy(entry.data, data, data_size);

    queue->entries[index] = entry;

    queue->head = index;
    --queue->free_entry_count;
}
