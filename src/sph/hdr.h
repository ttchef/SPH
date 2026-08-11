
#pragma once

#include <types.h>
#include <sph/memory.h>

typedef struct
{
	i32 idk;
} image_hdr;

bool hdr_create(memory_arena *arena, u32 size, void *data, image_hdr *out_image);

void hdr_destroy(image_hdr *image);
