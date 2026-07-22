
#include "sph/render.h"
#include "sph/ttf.h"
#include <types.h>
#include <sph/ui.h>
#include <math/core.h>
#include <vk/command.h>

#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_log.h>

#define UI_BACKGROUND color4gray(0.02f, 1.0f)

ui ui_create(vulkan_context *vulkan, u32 window_width, u32 window_height, const char *jet_brains)
{
	ui result = {0};

	result.settings_width = MIN(440, window_width * 0.27);
	result.orthographic = m4orthographic(0, window_width, window_height, 0, -1.0f, 1.0f);

	usize jet_brains_size;
	u8 *jet_brains_data = SDL_LoadFile(jet_brains, &jet_brains_size);

	if (!ttf_create(vulkan, jet_brains_size, jet_brains_data, &result.jet_brains))
	{
		SDL_Log("[ENGINE] Failed to load jet brains font.");
		return result;
	}

	return result;
}

void ui_destroy(vulkan_context *vulkan, ui *ui)
{
	ttf_destroy(vulkan, &ui->jet_brains);
}

void ui_update(ui *ui, u32 window_width, u32 window_height)
{
	ui->window_width = window_width;
	ui->window_height = window_height;
	ui->settings_width = MIN(440, window_width * 0.27);
	ui->orthographic = m4orthographic(0, window_width, window_height, 0, -1.0f, 1.0f);
}

void ui_draw(vulkan_context *vulkan, render *render, ui *ui)
{
	vulkan_command_set_viewport(vulkan, 0, 0, ui->window_width, ui->window_height);

	render_screen_quad(vulkan, render, v2make(ui->window_width - ui->settings_width, 0), v2make(ui->settings_width, ui->window_height), UI_BACKGROUND, ui->orthographic);
	render_text(vulkan, render, &ui->jet_brains, "Hello", v2make(0, 0), 2.0f, ui->orthographic);
}

