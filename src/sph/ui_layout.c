
#include <sph/app.h>
#include <sph/ui_layout.h>
#include <math/core.h>

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

    return result;
}

static bool is_active(app *app, ui_id id)
{
    return (app->ui_layout.active_id == id || app->ui_layout.active_id == UI_INVALID_ID);
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
        v2 world_pos = WORLD_POS(CURRENT()->id);
        world_pos.x += CURRENT()->padding.left + bubble_size * 0.5f;

        v2 size = SIZE(CURRENT()->id);
        size.x -= CURRENT()->padding.right + CURRENT()->padding.left + bubble_size;

        if (size.x > FLT_EPSILON)
        {
            if (HOVERED() && input_down(&app->input, INPUT_LMB) && is_active(app, CURRENT()->id))
            {
                *value = (app->input.mouse_pos.x - world_pos.x) / size.x;
                set_active(app, CURRENT()->id);
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
            .width     = FIXED(32),
            .height    = FIXED(32 * layout->height_scale),
            .padding   = PAD_ALL(5),
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
        })
        {
            CURRENT()->color = HOVERED() ? UI_COLOR4 : UI_COLOR3;

            if (HOVERED() && input_pressed(&app->input, INPUT_LMB))
            {
                CURRENT()->color = UI_COLOR6;
                result           = true;
            }

            UI({
                .width  = GROW(0),
                .height = GROW(0),
            });

            UI({
                .width  = FIT(0),
                .height = FIT(0),
                .text   = {
                    .chars     = label,
                    .font_size = 20 * layout->height_scale,
                    .font      = &app->jet_brains,
                },
            });

            UI({
                .width  = GROW(0),
                .height = GROW(0),
            });
        }
    }

    return result;
}

static void draw_color_picker(app *app, vulkan_command_queue *queue, v2 pos, v2 size)
{
    v2 center = v2add(pos, v2scale(size, 0.5f));

    m4 scale_m   = m4scale(size.x, size.y, 1.0);
    m4 translate = m4translate(center.x, center.y, 0.0f);
    m4 model     = m4mul(translate, scale_m);

    color_picker_pc pc = {
          .model = model,  
    };

    vulkan_command_bind_pipeline(queue, app->pipelines[PIPELINE_COLOR_PICKER]);
    vulkan_command_push_constants(queue, sizeof(pc), &pc, VK_SHADER_STAGE_VERTEX_BIT, app->pipelines[PIPELINE_COLOR_PICKER]);
    vulkan_command_draw(queue, 6);
}

static void color_picker(void)
{    
    UI({
        .pos = ABSOLUTE(0, 0),
        .width = FIXED(200),
        .height = FIXED(200),
        .color = UI_COLOR2,
        .padding = PAD_ALL(16),
    })
    {
        UI({
            .width = GROW(0),
            .height = GROW(0),
            .custom_draw = draw_color_picker,
        });
    }
}

static void color_gradient(app *app)
{
    ui_layout_context *layout = &app->ui_layout;

    UI({
        .width = GROW(0),
        .height = FIXED(48 * layout->height_scale),
        .color = UI_COLOR1,
        .padding = PAD_ALL(6),
    })
    {
        if (HOVERED() && input_down(&app->input, INPUT_LMB))
        {
            color_picker();         
        }
    
        UI({
            .width = GROW(0),
            .height = GROW(0),
            .gradient = {
                .a = RED,
                .b = BLUE,
                .type = GRADIENT_HORIZOTNAL,
            },
        });
    }
}

// NOTE: Non-linear ui scaling
void layout_size(ui_layout_context *layout, u32 width, u32 height)
{
    layout->width = (SDL_sqrtf(width) * 6.5f) + 100;
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
    }

    // NOTE: Reset current element id
    if (input_released(&app->input, INPUT_LMB))
    {
        app->ui_layout.active_id = UI_INVALID_ID;
    }
}
