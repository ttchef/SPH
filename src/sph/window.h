
#pragma once

#include <sph/types.h>
#include <types.h>

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

void window_text_input_start(window *window);

void window_text_input_end(window *window);
