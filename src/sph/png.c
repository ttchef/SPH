#include <SDL3/SDL.h>
#include <sph/png.h>

bool png_create(memory_arena *arena, u32 size, void *data, image_raw *out_image)
{
    SDL_IOStream *io = SDL_IOFromConstMem(data, size);
    if (!io)
    {
        SDL_Log("[PNG] Failed to create IO stream from memory: %s", SDL_GetError());
        return false;
    }

    SDL_Surface *surface = SDL_LoadPNG_IO(io, true);
    if (!surface)
    {
        SDL_Log("[PNG] Failed to load PNG: %s", SDL_GetError());
        return false;
    }

    SDL_Surface *rgba = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(surface);

    if (!rgba)
    {
        SDL_Log("[PNG] Failed to convert surface to RGBA32: %s", SDL_GetError());
        return false;
    }

    size_t buffer_size = (size_t)rgba->pitch * rgba->h;
    u8    *pixel_data  = (u8 *)memory_arena_alloc(arena, buffer_size);

    if (!pixel_data)
    {
        SDL_Log("[PNG] Failed to allocate pixel buffer");
        SDL_DestroySurface(rgba);
        return false;
    }

    SDL_memcpy(pixel_data, rgba->pixels, buffer_size);

    out_image->width  = (u32)rgba->w;
    out_image->height = (u32)rgba->h;
    out_image->data   = pixel_data;

    SDL_DestroySurface(rgba);

    return true;
}

void png_destroy(image_raw *image)
{
    if (image && image->data)
    {
        SDL_free(image->data);
        image->data = NULL;
    }
}
