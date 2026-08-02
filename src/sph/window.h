
#pragma once

#include <types.h>

#include <SDL3/SDL.h>

typedef struct
{
    SDL_Window *handle;

    // NOTE: Automatically gets updated
    u32 width;
    u32 height;
} window;

bool window_create(window *window, u32 width, u32 height);

void window_resize(window *window);

void window_destroy(window *window);
