
#pragma once

#include <types.h>
#include <vk/types.h>

typedef void (*vulkan_destroy_func)(vulkan *vulkan, void *data);

typedef struct
{
    void               *data;
    vulkan_destroy_func destroy_func;
    u32                 retired_frame;
} vulkan_destroy_entry;

typedef struct
{
    vulkan_destroy_entry entries[512];
    u32                  head;
    u32                  tail;
    u32                  free_entry_count;
} vulkan_destroy_queue;

vulkan_destroy_queue vulkan_destroy_queue_create(void);

void vulkan_destroy_queue_destroy(vulkan *vulkan, vulkan_destroy_queue *destroy_queue);

void vulkan_destroy_queue_flush(vulkan *vulkan, vulkan_destroy_queue *destroy_queue);

//
// NOTE: Public api
//
void vulkan_object_destroy(vulkan *vulkan, u32 data_size, void *data, vulkan_destroy_func destroy_function);
