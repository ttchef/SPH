
#pragma once

#include <types.h>
#include <math/types.h>
#include <vk/pipeline.h>

typedef struct render
{
	vulkan_pipeline_id cube_pipeline;
	vulkan_pipeline_id cube_lines_pipeline;
} render;

bool render_create(vulkan_context *vulkan, render *render);

void render_cube(vulkan_context *vulkan, render *render, v3 pos, v3 size, color4 color, m4 view_proj);

void render_cube_lines(vulkan_context *vulkan, render *render, v3 pos, v3 size, color4 color, m4 view_proj);

