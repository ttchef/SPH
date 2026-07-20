
#pragma once

#include <types.h>
#include <vk/types.h>
#include <sph/render.h>

typedef struct ui
{
	u32 window_width;
	u32 window_height;
	u32 settings_width;
	m4 orthographic;
} ui;

ui ui_create(u32 window_width, u32 window_height);

void ui_update(ui *ui, u32 window_width, u32 window_height);

void ui_draw(vulkan_context *vulkan, render *render, ui *ui);
