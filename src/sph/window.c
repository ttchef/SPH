
#include <sph/window.h>

bool window_create(window *window, u32 width, u32 height)
{
    window->handle = SDL_CreateWindow("SPH", width, height, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
    if (!window->handle)
    {
        SDL_Log("[ENGINE] %s", SDL_GetError());
        return false;
    }

    window->width  = width;
    window->height = height;

    return true;
}

void window_resize(window *window)
{
    i32 w, h;
    SDL_GetWindowSize(window->handle, &w, &h);

    window->width  = (u32)w;
    window->height = (u32)h;
}

void window_destroy(window *window)
{
    SDL_DestroyWindow(window->handle);
}
