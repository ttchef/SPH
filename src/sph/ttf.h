
#pragma once

#include <types.h>
#include <math/types.h>
#include <vk/image.h>
#include <vk/types.h>

typedef struct
{
	u16 *skyline;
	u16 max_width;
	u16 max_height;
	u16 skyline_count;
} ttf_pack;

typedef struct
{
	v2u pos;
	v2u size;
	f32 advance;	
} ttf_glyph;

typedef struct
{
	ttf_pack pack;
	ttf_glyph glyphs[100];
	vulkan_image atlas;
} ttf_font;

bool ttf_create(vulkan_context *vulkan, u32 size, void *data, ttf_font *out_font);

void ttf_destroy(vulkan_context *vulkan, ttf_font *font);
