
#pragma once

#include <types.h>
#include <vk/types.h>
#include <sph/render.h>
#include <sph/ttf.h>

typedef struct ui
{
	u32 window_width;
	u32 window_height;
	u32 settings_width;
	m4 orthographic;

	ttf_font jet_brains;
} ui;

ui ui_create(vulkan_context *vulkan, u32 window_width, u32 window_height, const char *jet_brains);

void ui_destroy(vulkan_context *vulkan, ui* ui);

void ui_update(ui *ui, u32 window_width, u32 window_height);

void ui_draw(vulkan_context *vulkan, render *render, ui *ui);
