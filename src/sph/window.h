
#pragma once

#include <types.h>

#include <SDL3/SDL.h>

//
// NOTE: This file is mainly to make the window struct visible
//       for other files 
// 

typedef struct window
{
    SDL_Window *handle;

    // NOTE: Automatically gets updated
    u32 width;
    u32 height;
} window;
