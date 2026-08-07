
#pragma once

#include <sph/ui.h>

//
// NOTE: This file uses sph/ui.h to build the ui for the application
//

typedef struct
{
    v2   triangle_point;
    bool triangle_point_valid;
    f32  hue;
} color_picker_data;

typedef struct
{
    // NOTE: Width of the layout at the moment (will change later when the layout is more complex)
    u32   width;
    f32   height_scale;
    ui_id active_id;

    bool              show_color_picker;
    bool              color_picker_triangle_active;
    v2                color_picker_pos;
    color_picker_data color_picker_data;
    color4            color_picker_color;
} ui_layout_context;

ui_layout_context ui_layout_create(void);

void ui_layout_calculate(app *app);
