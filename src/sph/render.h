
#pragma once

#include <types.h>
#include <math/types.h>
#include <vk/pipeline.h>
#include <sph/ttf.h>

typedef struct render
{
	vulkan_pipeline_id cube_pipeline;
	vulkan_pipeline_id cube_lines_pipeline;
	vulkan_pipeline_id quad_pipeline;
	vulkan_pipeline_id quad_scene_pipeline;
} render;

bool render_create(vulkan_context *vulkan, render *render);

void render_cube(vulkan_context *vulkan, render *render, v3 pos, v3 size, color4 color, m4 view_proj);

void render_cube_lines(vulkan_context *vulkan, render *render, v3 pos, v3 size, color4 color, m4 view_proj);

void render_quad(vulkan_context *vulkan, render *render, v3 pos, v2 size, color4 color, m4 view_proj);

void render_screen_quad(vulkan_context *vulkan, render *render, v2 pos, v2 size, color4 color, m4 orthographic);

void render_text(vulkan_context *vulkan, render *render, ttf_font *font, const char *text, v2 pos, f32 scale, m4 orthographic);
