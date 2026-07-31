
#pragma once

#include <types.h>

#include <SDL3/SDL_log.h>
#include <SDL3/SDL_stdinc.h>

//
// NOTE: Darray with shadowdata
//

#define DARRAY_START_CAPACITY 10
#define DARRAY_MAGIC          0x6767187F

typedef struct
{
    u32 capacity;
    u32 len;
    u32 element_size;
    u32 magic;
} darray_header;

static inline darray_header *darray_base(void *darray)
{
    return ((darray_header *)darray) - 1;
}

static inline void *darray_create(u32 element_size)
{
    darray_header *header = SDL_malloc(element_size * DARRAY_START_CAPACITY + sizeof(darray_header));
    assert(header);

    header->capacity     = DARRAY_START_CAPACITY;
    header->len          = 0;
    header->element_size = element_size;
    header->magic        = DARRAY_MAGIC;

    return (void *)(header + 1);
}

static inline void darray_destroy(void *darray)
{
    if (!darray)
    {
        SDL_Log("[DARRAY] Array ptr is NULL.");
        return;
    }

    darray_header *header = darray_base(darray);
    assert(header->magic == DARRAY_MAGIC);
    SDL_free(header);
}

static inline u32 darray_magic(void *darray)
{
    return darray_base(darray)->magic;
}

static inline u32 darray_element_size(void *darray)
{
    return darray_base(darray)->element_size;
}

static inline u32 darray_len(void *darray)
{
    return darray_base(darray)->len;
}

static inline u32 darray_capacity(void *darray)
{
    return darray_base(darray)->capacity;
}

static inline void darray_len_set(void *darray, u32 len)
{
    darray_base(darray)->len = len;   
}

// NOTE: Returns pointer to pushed element in the array and NULL on error
static inline void *darray_push(void ** darray_ptr, void *element)
{
    void          *darray = *darray_ptr;
    darray_header *header = darray_base(darray);

    assert(header->magic == DARRAY_MAGIC);

    if (header->len + 1 > header->capacity)
    {
        u32            new_capacity = header->capacity * 2;
        darray_header *new_header   = SDL_realloc(header, header->element_size * new_capacity + sizeof(darray_header));
        if (!new_header)
        {
            SDL_Log("[DARRAY] Out of memory.");
            return NULL;
        }

        new_header->capacity = new_capacity;
        header               = new_header;

        *darray_ptr = (void *)(new_header + 1);
        darray      = *darray_ptr;
    }

    u8 *data = darray;
    void *pos = data + header->element_size * header->len;
    
    SDL_memcpy(pos, element, header->element_size);

    header->len++;
    return pos;
}
