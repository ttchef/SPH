
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
} ui_layout_color_picker_data;

typedef struct
{
    color4 colors[4];
    f32    positions[4];

    bool                        show_color_picker;
    bool                        color_picker_triangle_active;
    v2                          color_picker_pos;
    ui_layout_color_picker_data color_picker_data;
    // NOTE: Index into the gradient
    u32 color_picker_color;
} ui_layout_gradient;

typedef struct
{
    char text[6];
    i32 text_len;
    i32 number;

    v2 press_pos;
    bool input_active;
    bool dont_activate;
} ui_layout_number_box;

typedef struct
{
    char text[20];
    f32 value;
} ui_layout_slider;

typedef struct
{
    // NOTE: Width of the layout at the moment (will change later when the layout is more complex)
    u32   width;
    f32   height_scale;
    ui_id active_id;

    ui_layout_number_box box_size_x;
    ui_layout_number_box box_size_y;
    ui_layout_number_box box_size_z;

    ui_layout_slider particle_radius;
    ui_layout_slider simulation_speed;

    ui_layout_slider target_density;
    ui_layout_slider pressure_multiplier;
    ui_layout_slider viscosity_multiplier;
    ui_layout_slider smoothing_radius;

    ui_layout_gradient particle_gradient;
} ui_layout_context;

ui_layout_context ui_layout_create(app *app);

void ui_layout_calculate(app *app);
