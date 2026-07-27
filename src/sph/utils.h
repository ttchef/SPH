
#pragma once

#include <types.h>

#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_log.h>

// NOTE: From the great and mighty cheesecake
static inline const char *path_abs(const char *name)
{
    // NOTE: Used for pipline shaders
    if (!name)
    {
        return NULL;
    }

    // NOTE: very long just to be sure
    static char buffer[2048];

    const char *base_path = SDL_GetBasePath();
    if (!base_path)
    {
        SDL_Log("[ENGINE] path_abs failed.");
        return NULL;
    }

    i32 len = SDL_snprintf(buffer, sizeof(buffer), "%s%s", base_path, name);
    if (len < 0 || (usize)len >= sizeof(buffer))
    {
        SDL_Log("[ENGINE] path_abs failed.");
        return NULL;
    }

    return buffer;
}
