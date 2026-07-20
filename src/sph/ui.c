
#include <types.h>
#include <sph/ui.h>
#include <math/core.h>
#include <vk/command.h>

#define UI_BACKGROUND color4gray(0.02f, 1.0f)

ui ui_create(u32 window_width, u32 window_height)
{
	ui result = {0};

	result.settings_width = MIN(440, window_width * 0.27);
	result.orthographic = m4orthographic(0, window_width, window_height, 0, -1.0f, 1.0f);

	return result;
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
}

