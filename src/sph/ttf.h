
#pragma once

#include <types.h>
#include <math/types.h>
#include <vk/image.h>

typedef struct ttf_pack
{
	u16 *skyline;
	u16 max_width;
	u16 max_height;
	u16 skyline_count;
	bool is_insitialized;
} ttf_pack;

typedef struct ttf_glyph
{
	v2u pos;
	v2u size;
	f32 advance;	
} ttf_glyph;

typedef struct ttf_font
{
	ttf_pack pack;
	ttf_glyph glyphs[100];
	vulkan_image atlas;
} ttf_font;

bool ttf_create(u32 size, void *data, ttf_font *out_font);

void ttf_destroy(ttf_font *font);
