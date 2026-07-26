
#pragma once

#include <types.h>

#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_log.h>

// NOTE: From the great and mighty cheesecake
static inline const char *path_abs(const char *name)
{
    assert(name);

    // NOTE: very long just to be sure
    static char buffer[2048];

    const char *BasePath = SDL_GetBasePath();
    if (!BasePath)
    {
        SDL_Log("[ENGINE] path_abs failed.");
        return NULL;
    }

    i32 PathLen = SDL_snprintf(buffer, sizeof(buffer), "%s%s", BasePath, name);
    if (PathLen < 0 || (usize)PathLen >= sizeof(buffer))
    {
        SDL_Log("[ENGINE] path_abs failed.");
        return NULL;
    }

    return buffer;
}
