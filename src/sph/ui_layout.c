
#include <sph/app.h>
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

    result.active_id = UI_INVALID_ID;
    result.width     = 400;

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
    const f32 bubble_size = 20;

    *value -= min;
    *value /= (max - min);

    UI({
        .layout      = LAYOUT_TO_BOTTOM,
        .width       = GROW(0),
        .height      = FIT(0),
        .child_align = LEFT,
        .color       = UI_COLOR1,
        .padding     = PAD_ALL(16),
        .child_gap   = 16,
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
                .font_size = 20,
                .font      = &app->jet_brains,
            },
        });

        *value = CLAMP(*value, 0.0f, 1.0f);

        UI({
            .width     = GROW(0),
            .height    = FIXED(10),
            .color     = UI_COLOR3,
            .roundness = 0.9f,
        })
        {
            UI({
                .pos       = v2make(0, -5),
                .width     = FIXED(bubble_size),
                .height    = FIXED(bubble_size),
                .roundness = 1.0f,
                .color     = HOVERED() ? UI_COLOR9 : UI_COLOR6,
            })
            {
                CURRENT()->pos.x = *value * size.x;
            }
        }
    }

    *value *= (max - min);
    *value += min;
}

static void checkbox(app *app, bool *value, const char *label)
{
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
            .height    = FIXED(32),
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
                .font_size = 20,
                .font      = &app->jet_brains,
            },
        });
    }
}

// NOTE: Returns true when pressed
static bool button(app *app, const char *label)
{
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
            .height      = FIXED(32),
            .roundness   = 0.4f,
            .child_align = CENTER,
        })
        {
            CURRENT()->color = HOVERED() ? UI_COLOR4 : UI_COLOR3;

            if (HOVERED() && input_pressed(&app->input, INPUT_LMB))
            {
                CURRENT()->color = UI_COLOR6;
                result = true;
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
                    .font_size = 20,
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

void ui_layout_calculate(app *app)
{
    UI({
        .layout    = LAYOUT_TO_BOTTOM,
        .pos       = v2make(app->window.width - 400, 0.0f),
        .width     = FIXED(400),
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
            .width  = GROW(0),
            .height = FIT(0),
            .child_gap = 12,
        })
        {
            app->paused_pressed = button(app, "Reset");
            app->paused_pressed = button(app, app->paused ? "Resume" : "Pause");
        }
    }

    // NOTE: Reset current element id
    if (input_released(&app->input, INPUT_LMB))
    {
        app->ui_layout.active_id = UI_INVALID_ID;
    }
}
