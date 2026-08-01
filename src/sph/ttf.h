
#pragma once

#include <math/types.h>
#include <types.h>
#include <vk/image.h>
#include <vk/types.h>

typedef struct
{
    u16 *skyline;
    u16  max_width;
    u16  max_height;
    u16  skyline_count;
} ttf_pack;

typedef struct
{
    v2 uv_min;
    v2 uv_max;
    v2 bearing;
    v2u size_px;
    f32 advance;
} ttf_glyph;

typedef struct
{
    ttf_pack     pack;
    ttf_glyph    glyphs[100];
    vulkan_image atlas;
    f32 ascent;
    f32 descent;
    f32 line_gap;
    f32 size_px;
} ttf_font;

bool ttf_create(vulkan *vulkan, u32 size, void *data, const char *name, ttf_font *out_font);

void ttf_destroy(vulkan *vulkan, ttf_font *font);

