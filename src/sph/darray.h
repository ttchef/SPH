
#pragma once

#include <types.h>

#include <SDL3/SDL_log.h>
#include <SDL3/SDL_stdinc.h>

//
// NOTE: Darray with shadowdata
// 
// 

#define DARRAY_START_CAPACITY 10
#define DARRAY_MAGIC          0x6767187F

enum
{
    DARRAY_CAPACITY_INDEX,
    DARRAY_LEN_INDEX,
    DARRAY_ELEMENT_SIZE_INDEX,
    DARRAY_MAGIC_INDEX,
    DARRAY_INDEX_COUNT,
};

static inline void *darray_create(u32 element_size)
{
    u32 *ptr = SDL_malloc(element_size * DARRAY_START_CAPACITY + sizeof(u32) * 4);
    assert(ptr);

    ptr[DARRAY_CAPACITY_INDEX] = DARRAY_START_CAPACITY;
    ptr[DARRAY_LEN_INDEX] = 0;
    ptr[DARRAY_ELEMENT_SIZE_INDEX] = element_size;
    ptr[DARRAY_MAGIC_INDEX] = DARRAY_MAGIC;

    return (void *)&ptr[DARRAY_INDEX_COUNT];
}

static inline u32 *darray_base_ptr(void *darray)
{
    u32 *ptr = darray;
    ptr -= DARRAY_INDEX_COUNT;
    return ptr;
}

static inline u32 darray_magic(void *darray)
{
    return darray_base_ptr(darray)[DARRAY_MAGIC_INDEX];
}

static inline u32 darray_element_size(void *darray)
{
    return darray_base_ptr(darray)[DARRAY_ELEMENT_SIZE_INDEX];
}

static inline u32 darray_len(void *darray)
{
    return darray_base_ptr(darray)[DARRAY_LEN_INDEX];
}

static inline u32 darray_capacity(void *darray)
{
    return darray_base_ptr(darray)[DARRAY_CAPACITY_INDEX];
}

static inline bool darray_push(void *darray, void *element)
{
    assert(darray_magic(darray) == DARRAY_MAGIC);

    if (darray_len(darray) + 1 > darray_capacity(darray))
    {
        u32 new_capacity = darray_capacity(darray) * 2;
        u32 *new_data = SDL_realloc(darray_base_ptr(darray), new_capacity * darray_element_size(darray));
        if (!new_data)
        {
            SDL_Log("[DARRAY] Out of memory.");
            return false;
        }

        new_data[DARRAY_CAPACITY_INDEX] = new_capacity;

    }

    u8 *data = darray;

    SDL_memcpy(data + darray_element_size(darray) * darray_len(darray), element, darray_element_size(darray));

    return true;
}
