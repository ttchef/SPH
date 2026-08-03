
#pragma once

#include <types.h>
#include <sph/memory.h>

bool png_create(memory_arena *arena, u32 size, void *data, image_raw *out_image);

void png_destroy(image_raw *image);
