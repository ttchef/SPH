
#pragma once

#include <types.h>
#include <sph/types.h>

#include <SDL3/SDL.h>

struct window
{
    SDL_Window *handle;

    // NOTE: Automatically gets updated
    u32 width;
    u32 height;
};

bool window_create(window *window, u32 width, u32 height);

void window_resize(window *window);

void window_destroy(window *window);
