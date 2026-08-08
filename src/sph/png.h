
#pragma once

#include <sph/memory.h>
#include <types.h>

bool png_create(memory_arena *arena, u32 size, void *data, image_raw *out_image);

void png_destroy(image_raw *image);
