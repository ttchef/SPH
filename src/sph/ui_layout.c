
#include <sph/simulation.h>
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

ui_layout_context ui_layout_create(void)
{
    ui_layout_context result = {0};

    result.active_id = UI_INVALID_ID;
    result.width     = 400;

    return result;
}

static bool is_active(simulation *simulation, ui_id id)
{
    return (simulation->ui_layout.active_id == id || simulation->ui_layout.active_id == UI_INVALID_ID);
}

static void set_active(simulation *simulation, ui_id id)
{
    simulation->ui_layout.active_id = id;
}

static void slider(simulation *simulation, f32 *value, f32 min, f32 max, const char *label)
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
        .roundness   = 0.3f,
    })
    {
        v2 world_pos = WORLD_POS(CURRENT()->id);
        world_pos.x += CURRENT()->padding.left + bubble_size * 0.5f;

        v2 size = SIZE(CURRENT()->id);
        size.x -= CURRENT()->padding.right + CURRENT()->padding.left + bubble_size;

        if (size.x > FLT_EPSILON)
        {
            if (HOVERED() && input_down(&simulation->input, INPUT_LMB) && is_active(simulation, CURRENT()->id))
            {
                *value                          = (simulation->input.mouse_pos.x - world_pos.x) / size.x;
                set_active(simulation, CURRENT()->id);
            }
        }

        UI({
            .width  = FIT(0),
            .height = FIT(0),
            .text   = {
                .chars     = label,
                .font_size = 20,
                .font      = &simulation->jet_brains,
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

void ui_layout_calculate(simulation *simulation)
{
    UI({
        .layout    = LAYOUT_TO_BOTTOM,
        .pos       = v2make(simulation->window.width - 400, 0.0f),
        .width     = FIXED(400),
        .height    = PERCENT(1.0f),
        .color     = UI_COLOR0,
        .padding   = PAD_ALL(16),
        .child_gap = 16,
    })
    {
        slider(simulation, &simulation->bounding_box.size.x, 10, 1000, "Size X");
        slider(simulation, &simulation->bounding_box.size.y, 10, 1000, "Size Y");
        slider(simulation, &simulation->bounding_box.size.z, 10, 1000, "Size Z");
    }

    // NOTE: Reset current element id
    if (input_released(&simulation->input, INPUT_LMB))
    {
        simulation->ui_layout.active_id = UI_INVALID_ID;
    }
}
