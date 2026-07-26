
#pragma once

#include <types.h>

bool png_create(u32 size, void *data, image_raw *out_image);

void png_destroy(image_raw *image);
