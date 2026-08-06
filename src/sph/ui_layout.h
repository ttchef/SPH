
#pragma once

#include <sph/ui.h>

//
// NOTE: This file uses sph/ui.h to build the ui for the application
//

typedef struct
{
	// NOTE: Width of the layout at the moment (will change later when the layout is more complex)
	u32 width;
	f32 height_scale;
	ui_id active_id;

	bool show_color_picker;
}  ui_layout_context;

ui_layout_context ui_layout_create(void);

void ui_layout_calculate(app *app);


