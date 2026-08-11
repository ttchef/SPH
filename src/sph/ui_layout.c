
#include <math/core.h>
#include <sph/app.h>
#include <sph/color.h>
#include <sph/ui_layout.h>

#define UI_COLOR0 color4gray(0.04, 1.0)
#define UI_COLOR1 color4gray(0.08, 1.0)
#define UI_COLOR2 color4gray(0.12, 1.0)
#define UI_COLOR3 color4gray(0.2, 1.0)
#define UI_COLOR4 color4gray(0.27, 1.0)
#define UI_COLOR5 color4gray(0.36, 1.0)
#define UI_COLOR6 color4gray(0.48, 1.0)
#define UI_COLOR7 color4gray(0.56, 1.0)
#define UI_COLOR8 color4gray(0.68, 1.0)
#define UI_COLOR9 color4gray(0.80, 1.0)

const f32 STD_ROUNDNESS = 0.3f;

ui_layout_context ui_layout_create(void)
{
    ui_layout_context result = {0};

    result.active_id    = UI_INVALID_ID;
    result.height_scale = 1.0f;

    result.particle_gradient = (ui_layout_gradient){
        .colors = {
            BLUE,
            CYAN,
            YELLOW,
            RED,
        },
        .positions = {0.0f, 0.33f, 0.66f, 1.0f},
    };

    return result;
}

static bool is_active(app *app, ui_id id)
{
    return (app->ui_layout.active_id == id);
}

static void set_active(app *app, ui_id id)
{
    app->ui_layout.active_id = id;
}

static void slider(app *app, f32 *value, f32 min, f32 max, const char *label)
{
    ui_layout_context *layout      = &app->ui_layout;
    const f32          bubble_size = 20 * layout->height_scale;

    *value -= min;
    *value /= (max - min);

    UI({
        .layout      = LAYOUT_TO_BOTTOM,
        .width       = GROW(0),
        .height      = FIT(0),
        .child_align = LEFT,
        .color       = UI_COLOR1,
        .padding     = PAD_ALL(16 * layout->height_scale),
        .child_gap   = 16 * layout->height_scale,
        .roundness   = STD_ROUNDNESS,
    })
    {
        if (HOVERED() && input_pressed(&app->input, INPUT_LMB))
        {
            set_active(app, CURRENT()->id);
        }

        v2 world_pos = WORLD_POS(CURRENT()->id);
        world_pos.x += CURRENT()->padding.left + bubble_size * 0.5f;

        v2 size = SIZE(CURRENT()->id);
        size.x -= CURRENT()->padding.right + CURRENT()->padding.left + bubble_size;

        if (is_active(app, CURRENT()->id) && size.x > FLT_EPSILON)
        {
            *value = (app->input.mouse_pos.x - world_pos.x) / size.x;
        }

        UI({
            .width  = FIT(0),
            .height = FIT(0),
            .text   = {
                .chars     = label,
                .font_size = 20 * layout->height_scale,
                .font      = &app->jet_brains,
            },
        });

        *value = CLAMP(*value, 0.0f, 1.0f);

        UI({
            .width     = GROW(0),
            .height    = FIXED(10 * layout->height_scale),
            .color     = UI_COLOR3,
            .roundness = 0.9f,
        })
        {
            UI({
                .pos       = RELATIVE(0, -bubble_size / 4),
                .width     = FIXED(bubble_size),
                .height    = FIXED(bubble_size),
                .roundness = 1.0f,
                .color     = HOVERED() ? UI_COLOR9 : UI_COLOR6,
            })
            {
                POS(CURRENT())->x = *value * size.x;
            }
        }
    }

    *value *= (max - min);
    *value += min;
}

static void checkbox(app *app, bool *value, const char *label)
{
    ui_layout_context *layout = &app->ui_layout;

    UI({
        .width       = GROW(0),
        .height      = FIT(0),
        .color       = UI_COLOR1,
        .roundness   = STD_ROUNDNESS,
        .padding     = PAD_ALL(16),
        .child_gap   = 12,
        .child_align = CENTER,
    })
    {
        UI({
            .height    = FIXED(32 * layout->height_scale),
            .padding   = PAD_ALL(5),
            .aspect_ratio = 1.0f,
            .roundness = 0.4f,
        })
        {
            CURRENT()->color = HOVERED() ? UI_COLOR4 : UI_COLOR3;

            if (HOVERED() && input_pressed(&app->input, INPUT_LMB))
            {
                *value = !*value;
            }

            if (*value)
            {
                UI({
                    .width     = GROW(0),
                    .height    = GROW(0),
                    .color     = UI_COLOR8,
                    .roundness = 0.4f,
                });
            }
        }

        UI({
            .width  = FIT(0),
            .height = FIT(0),
            .text   = {
                .chars     = label,
                .font_size = 20 * layout->height_scale,
                .font      = &app->jet_brains,
            },
        });
    }
}

// NOTE: Returns true when pressed
static bool button(app *app, const char *label)
{
    ui_layout_context *layout = &app->ui_layout;

    bool result = false;

    UI({
        .width     = GROW(0),
        .height    = FIT(0),
        .color     = UI_COLOR1,
        .padding   = PAD_ALL(8),
        .roundness = STD_ROUNDNESS,
    })
    {
        UI({
            .width       = GROW(0),
            .height      = FIXED(32 * layout->height_scale),
            .roundness   = 0.4f,
            .child_align = CENTER,
            .text   = {
                .chars     = label,
                .font_size = 20 * layout->height_scale,
                .font      = &app->jet_brains,
                .align_x = CENTER,
                .align_y = CENTER,
            },
        })
        {
            CURRENT()->color = HOVERED() ? UI_COLOR4 : UI_COLOR3;

            if (HOVERED() && input_pressed(&app->input, INPUT_LMB))
            {
                CURRENT()->color = UI_COLOR6;
                result           = true;
            }
        }
    }

    return result;
}

static void draw_color_picker(app *app, vulkan_command_queue *queue, v2 pos, v2 size, void *data)
{
    ui_layout_color_picker_data *color_picker = (ui_layout_color_picker_data *)data;

    v2 center = v2add(pos, v2scale(size, 0.5f));

    m4 scale_m   = m4scale(size.x, size.y, 1.0);
    m4 translate = m4translate(center.x, center.y, 0.0f);
    m4 model     = m4mul(translate, scale_m);

    color_picker_pc pc = {
        .model  = model,
        .data.x = color_picker->triangle_point.x,
        .data.y = color_picker->triangle_point.y,
        .data.z = color_picker->triangle_point_valid ? 1.0f : 0.0f,
        .data.w = color_picker->hue,
    };

    vulkan_command_bind_pipeline(queue, app->pipelines[PIPELINE_COLOR_PICKER]);
    vulkan_command_push_constants(queue, sizeof(pc), &pc, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, app->pipelines[PIPELINE_COLOR_PICKER]);
    vulkan_command_draw(queue, 6);
}

static color4 get_color(f32 hue, v2 p)
{
    const f32 k = SDL_sqrtf(3.0f);
    const v2  a = v2make(0.0, 2.0 * 0.75 / k);
    const v2  b = v2make(-0.75, -0.75 / k);
    const v2  c = v2make(0.75, -0.75 / k);

    v3 colors[] = {
        v3make(hue, 0.0, 1.0),
        v3make(hue, 1.0, 0.0),
        v3make(hue, 1.0, 1.0),
    };

    v3 barycentrics = math_barycentrics(a, b, c, p);

    // NOTE: Fuck naming at this point (i am writing this at 2 am)
    v3 a0 = v3scale(colors[0], barycentrics.x);
    v3 b0 = v3scale(colors[1], barycentrics.y);
    v3 c0 = v3scale(colors[2], barycentrics.z);

    v3 hsv = v3add(a0, v3add(b0, c0));
    return color4fromhsv2(hsv);
}

static void color_picker(app *app)
{
    ui_layout_context *layout        = &app->ui_layout;
    const u32          panel_width   = 250;
    const u32          panel_height  = 290;
    const u32          topbar_height = panel_height - panel_width;

    UI({
        .layout    = LAYOUT_TO_BOTTOM,
        .pos       = ABSOLUTE(layout->color_picker_pos.x, layout->color_picker_pos.y, 1),
        .width     = FIXED(panel_width),
        .height    = FIXED(panel_height),
        .color     = layout->particle_gradient.colors[layout->color_picker_color],
        .padding   = PAD_ALL(8),
        .child_gap = 12,
        .roundness = 0.05f,
    })
    {
        // NOTE: Topbar
        UI({
            .width       = GROW(0),
            .height      = FIXED(topbar_height),
            .color       = UI_COLOR4,
            .roundness   = 0.3f,
            .padding     = PAD_ALL(6),
            .child_align = CENTER,
        })
        {
            if (HOVERED() && input_pressed(&app->input, INPUT_LMB))
            {
                set_active(app, CURRENT()->id);
            }
            if (is_active(app, CURRENT()->id))
            {
                layout->color_picker_pos = v2add(layout->color_picker_pos, app->input.mouse_delta);
            }

            UI({
                .layout      = LAYOUT_TO_RIGHT,
                .width       = FIXED(topbar_height - CURRENT()->padding.right - CURRENT()->padding.left),
                .height      = GROW(0),
                .color       = RED,
                .roundness   = 0.35f,
                // .child_align = CENTER,
            })
            {
                if (HOVERED() && input_pressed(&app->input, INPUT_LMB))
                {
                    layout->show_color_picker = false;
                }

                UI({
                    .width  = GROW(0),
                    .height = GROW(0),
                    .color = UI_COLOR8, 
                    .text   = {
                        .font_size = 30,
                        .chars     = "X",
                        .font      = &app->jet_brains,
                        .align_x = CENTER,
                        .align_y = CENTER,
                    },
                });
            }
        }

        // NOTE: Color picker container
        UI({
            .width  = GROW(0),
            .height = GROW(0),
            .color  = UI_COLOR0,
        })
        {
            v2 world_pos = WORLD_POS(CURRENT()->id);
            v2 size      = SIZE(CURRENT()->id);

            if (HOVERED() && input_pressed(&app->input, INPUT_LMB))
            {
                v2 mouse_pos = v2sub(app->input.mouse_pos, world_pos);

                f32 u = (mouse_pos.x / size.x) * 2.0f - 1.0f;
                f32 v = (mouse_pos.y / size.y) * 2.0f - 1.0f;
                v2  p = v2make(u, -v);

                v2 triangle_p = v2rotate(p, TO_RADIANS(-layout->color_picker_data.hue + 90.0f));
                if (sdf_triangle2D(triangle_p, 0.75) <= 0.0f)
                {
                    set_active(app, CURRENT()->id);
                    layout->color_picker_triangle_active = true;
                }
                else if (sdf_ring2D(p, 0.9f, 1.0) <= 0.1f)
                {
                    set_active(app, CURRENT()->id);
                    layout->color_picker_triangle_active = false;
                }
            }

            if (is_active(app, CURRENT()->id))
            {
                v2 mouse_pos = v2sub(app->input.mouse_pos, world_pos);

                f32 u = (mouse_pos.x / size.x) * 2.0f - 1.0f;
                f32 v = (mouse_pos.y / size.y) * 2.0f - 1.0f;
                v2  p = v2make(u, -v);

                const f32 k = SDL_sqrtf(3.0f);
                const v2  a = v2make(0.0, 2.0 * 0.75 / k);
                const v2  b = v2make(-0.75, -0.75 / k);
                const v2  c = v2make(0.75, -0.75 / k);

                if (layout->color_picker_triangle_active)
                {
                    v2 triangle_p                                  = v2rotate(p, TO_RADIANS(-layout->color_picker_data.hue + 90.0f));
                    layout->color_picker_data.triangle_point       = triangle_p;
                    layout->color_picker_data.triangle_point_valid = true;
                    if (sdf_triangle2D(triangle_p, 0.75) > 0.0f)
                    {
                        layout->color_picker_data.triangle_point = math_closest_point_on_triangle(triangle_p, a, b, c);
                    }

                    layout->particle_gradient.colors[layout->color_picker_color] = get_color(layout->color_picker_data.hue, layout->color_picker_data.triangle_point);
                }
                else
                {
                    layout->color_picker_data.hue = TO_DEGREES(SDL_atan2f(p.y, p.x));
                    if (layout->color_picker_data.hue < 0.0f)
                    {
                        layout->color_picker_data.hue += 360.0f;
                    }

                    layout->particle_gradient.colors[layout->color_picker_color] = get_color(layout->color_picker_data.hue, layout->color_picker_data.triangle_point);
                }
            }

            UI({
                .width  = GROW(0),
                .height = GROW(0),
                .custom = {
                    .draw_func = draw_color_picker,
                    .data      = (void *)&layout->color_picker_data,
                },
            });
        }
    }
}

static void color_gradient(app *app)
{
    ui_layout_context *layout = &app->ui_layout;

    UI({
        .layout    = LAYOUT_TO_BOTTOM,
        .width     = GROW(0),
        .height    = FIT(0),
        .color     = UI_COLOR1,
        .padding   = PAD(18, 18, 12, 12),
        .child_gap = 6,
        .roundness = STD_ROUNDNESS,
    })
    {
        UI({
            .width  = GROW(0),
            .height = FIXED(40 * layout->height_scale),
            .color  = UI_COLOR3,
        })
        {
            ui_layout_gradient *gradient = &layout->particle_gradient;

            if (gradient->positions[0] > 0.005f)
            {
                UI({
                    .pos    = RELATIVE(0.0f, 0.0f),
                    .width  = PERCENT(gradient->positions[0]),
                    .height = GROW(0),
                    .color  = gradient->colors[0],
                });
            }

            for (u32 i = 1; i < ARRAY_COUNT(gradient->colors); i++)
            {
                f32 width = gradient->positions[i] - gradient->positions[i - 1];

                UI({
                    .width    = PERCENT(width),
                    .height   = GROW(0),
                    .gradient = {
                        .type = GRADIENT_HORIZOTNAL,
                        .a    = gradient->colors[i - 1],
                        .b    = gradient->colors[i],
                    },
                });
            }

            if (gradient->positions[ARRAY_COUNT(gradient->positions) - 1] < 0.995)
            {
                UI({
                    .width  = PERCENT(1.0f - gradient->positions[ARRAY_COUNT(gradient->positions) - 1]),
                    .height = GROW(0),
                    .color  = gradient->colors[ARRAY_COUNT(gradient->positions) - 1],
                });
            }
        }

        UI({
            .width  = GROW(0),
            .height = FIXED(35 * layout->height_scale),
        })
        {
            v2                  container_size = SIZE(CURRENT()->id);
            ui_layout_gradient *gradient       = &layout->particle_gradient;

            u32 size = 25;
            for (u32 i = 0; i < ARRAY_COUNT(gradient->colors); i++)
            {
                UI({
                    .pos     = RELATIVE(gradient->positions[i] * container_size.x - size / 2.0f, 0.0f),
                    .width   = FIXED(size),
                    .height  = FIXED(size),
                    .color   = UI_COLOR3,
                    .flags   = UI_NO_ADVANCE_BIT,
                    .padding = PAD_ALL(4),
                })
                {
                    if (HOVERED() && input_pressed(&app->input, INPUT_RMB))
                    {
                        layout->show_color_picker = true;
                        layout->color_picker_pos  = WORLD_POS(PARENT()->id);
                        layout->color_picker_pos.x -= 300;
                        layout->color_picker_color = i;
                        set_active(app, CURRENT()->id);
                    }

                    if (HOVERED() && input_pressed(&app->input, INPUT_LMB))
                    {
                        set_active(app, CURRENT()->id);
                    }

                    if (is_active(app, CURRENT()->id) && !layout->show_color_picker)
                    {
                        v2 world_pos = WORLD_POS(PARENT()->id);
                        if (world_pos.x < app->input.mouse_pos.x)
                        {
                            v2  p = v2sub(app->input.mouse_pos, world_pos);
                            f32 t = p.x / container_size.x;

                            f32 lower = i == 0 ? 0.0f : gradient->positions[i - 1];
                            f32 upper = (i + 1 == ARRAY_COUNT(gradient->positions)) ? 1.0f : gradient->positions[i + 1];

                            gradient->positions[i] = CLAMP(t, lower, upper);
                        }
                    }

                    UI({
                        .width  = GROW(0),
                        .height = GROW(0),
                        .color  = gradient->colors[i],
                    });
                }
            }
        }

        if (layout->show_color_picker)
        {
            color_picker(app);
        }
    }
}

static void number_box(app *app, const char *label)
{
    ui_layout_context *layout = &app->ui_layout;
    
    UI({
        .width = GROW(0),
        .height = FIXED(48 * layout->height_scale),
        .color = UI_COLOR5,
        .roundness = STD_ROUNDNESS,
        .padding = PAD(4, 12, 4, 4),
        .child_gap = 12,
    })
    {
        UI({
            .width = PERCENT(0.5f),
            .height = GROW(0),
            .color = UI_COLOR2,
            .padding = PAD_ALL(6),
        })
        {
            UI({
                .height = GROW(0),
                .aspect_ratio = 1.0f,
                .color = UI_COLOR8,
            });
        }

        UI({
            .width = PERCENT(0.5f),
            .height = GROW(0),
            .text = {
                .chars = label,
                .font_size = 24 * layout->height_scale,
                .font = &app->jet_brains,
                .align_x = CENTER,
                .align_y = CENTER,
            },
        });
    }
}

// NOTE: Non-linear ui scaling
void layout_size(ui_layout_context *layout, u32 width, u32 height)
{
    layout->width        = (SDL_sqrtf(width) * 6.5f) + 100;
    layout->height_scale = MAX(height / 1440.0f, 0.6f);
}

void ui_layout_calculate(app *app)
{
    ui_layout_context *layout = &app->ui_layout;
    layout_size(layout, app->window.width, app->window.height);

    UI({
        .layout    = LAYOUT_TO_BOTTOM,
        .pos       = RELATIVE(app->window.width - layout->width, 0.0f),
        .width     = FIXED(layout->width),
        .height    = PERCENT(1.0f),
        .color     = UI_COLOR0,
        .padding   = PAD_ALL(16),
        .child_gap = 16,
    })
    {
        slider(app, &app->bounding_box.size.x, 10, 1000, "Size X");
        slider(app, &app->bounding_box.size.y, 10, 1000, "Size Y");
        slider(app, &app->bounding_box.size.z, 10, 1000, "Size Z");

        checkbox(app, &app->render_bounding_box, "Render bounding box");

        UI({
            .width     = GROW(0),
            .height    = FIT(0),
            .child_gap = 12,
        })
        {
            app->reset_pressed = button(app, app->simulation.initialized ? "Reset" : "Spawn");
            if (button(app, app->paused ? "Resume" : "Pause"))
            {
                app->paused = !app->paused;
            }
        }

        slider(app, &app->particle_radius, 1, 10, "Particle radius");
        slider(app, &app->simulation_speed, 0.01, 1, "Speed");

        slider(app, &app->simulation.ubo_data.target_density, 0.001, 0.05, "Target density");
        slider(app, &app->simulation.ubo_data.pressure_multiplier, 0.5, 25.0, "Pressure multiplier");
        slider(app, &app->simulation.ubo_data.viscosity_coeff, 0.5, 25.0, "Viscosity multiplier");
        slider(app, &app->simulation.ubo_data.smoothing_radius, 5, 25.0, "Smoothing radius");

        color_gradient(app);
        number_box(app, "Textbox");
    }

    // NOTE: Reset current element id
    if (input_released(&app->input, INPUT_LMB))
    {
        app->ui_layout.active_id = UI_INVALID_ID;
    }
}
