
#pragma once

#include <types.h>

#include <SDL3/SDL_filesystem.h>

// NOTE: From the great and mighty cheesecake
static inline bool get_abs_path(char *buf, usize size, const char *name)
{
    assert(buf);
    assert(name);
    assert(size > 0);

    const char *BasePath = SDL_GetBasePath();
    if (!BasePath)
    {
        return false;
    }

    i32 PathLen = SDL_snprintf(buf, size, "%s%s", BasePath, name);
    if (PathLen < 0 || (usize)PathLen >= size)
    {
        return false;
    }

    return true;
}

