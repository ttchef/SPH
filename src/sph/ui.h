
#pragma once

#include <math/vector.h>
#include <sph/darray.h>
#include <sph/input.h>
#include <sph/ttf.h>
#include <sph/types.h>
#include <types.h>

//
// NOTE: Heavily inspired by clay: https://github.com/nicbarker/clay.git
//       This file defines the ui api which you can use to build the ui.
//       Because of this doesnt have proper namespacing to make using it more pleasent
//

typedef u32 ui_id;

#define UI_INVALID_ID UINT32_MAX

typedef enum
{
    // NOTE: Default is FIT
    UI_SIZE_FIT,
    UI_SIZE_FIXED,
    UI_SIZE_PERCENT,
    UI_SIZE_GROW,
} ui_size_type;

typedef struct
{
    f32 min;
    f32 max;
} ui_min_max;

typedef struct
{
    ui_size_type type;
    union
    {
        f32        percent;
        ui_min_max min_max;
    };
} ui_size;

typedef struct
{
    f32 left;
    f32 right;
    f32 top;
    f32 bottom;
} ui_padding;

typedef enum
{
    LAYOUT_TO_RIGHT,
    LAYOUT_TO_BOTTOM,
} ui_layout;

typedef enum
{
    LEFT,
    RIGHT,
    TOP,
    BOTTOM,
    CENTER,
} ui_alignement;

typedef struct
{
    color4        a;
    color4        b;
    gradient_type type;
} ui_gradient;

typedef enum
{
    POSITION_RELATIVE,
    POSITION_ABSOLUTE,
} ui_position_type;

typedef struct
{
    ui_position_type type;
    u32              z_order;

    union
    {
        v2 relative;
        v2 absolute;
    };
} ui_position;

typedef void (*ui_draw_fn)(app *app, vulkan_command_queue *queue, v2 pos, v2 size, void *data);

enum
{
    UI_NO_ADVANCE_BIT = (1 << 0),
};

typedef struct ui_element
{
    ui_id       id;
    ui_layout   layout;
    ui_position pos;
    ui_size     width;
    ui_size     height;
    color4      color;
    f32         roundness;

    ui_gradient gradient;

    ui_padding padding;

    f32           child_gap;
    ui_alignement child_align;

    u16 flags;

    struct
    {
        ui_draw_fn draw_func;
        // NOTE: Must be statically allocated right now
        void *data;
    } custom;

    struct
    {
        // NOTE: Must be statically allocated right now
        const char *chars;
        u32         font_size;
        // NOTE: Must be statically allocated right now
        ttf_font *font;
    } text;

    struct ui_element *parent;
    // NOTE: darray.h
    struct ui_element *children;
} ui_element;

typedef struct
{
    v2    pos;
    v2    size;
    ui_id id;
} ui_previous_data;

typedef struct
{
    struct
    {
        // NOTE: Max of 32 element depth
        ui_element *elements[32];
        u32         element_count;
    } open;

    struct
    {
        ui_element *elements[64];
        u32         element_count;
    } floating;

    // NOTE: Only have one root element for now
    ui_element *root;

    u32 element_count;

    // NOTE: darray.h
    ui_id            *pointer_over_ids;
    ui_previous_data *previous_data;
} ui_context;

void ui_update(input *input);

ui_element *ui_open(ui_element *parent, ui_element element);

void ui_close(ui_element *element);

void ui_draw(app *app, vulkan_command_queue *queue);

ui_context *ui_context_get(void);

ui_element *ui_open_elements_pop(void);

ui_element *ui_open_elements_peek(i32 offset);

bool ui_hovered(ui_id id);

v2 ui_world_data(ui_id id, bool size);

static inline ui_size FIXED(f32 size)
{
    return (ui_size){
        .type        = UI_SIZE_FIXED,
        .min_max.min = size,
        .min_max.max = size,
    };
}

static inline ui_size PERCENT(f32 percent)
{
    return (ui_size){
        .type    = UI_SIZE_PERCENT,
        .percent = percent,
    };
}

#define FIT(...)             \
    (ui_size)                \
    {                        \
        .type = UI_SIZE_FIT, \
        .min_max =           \
        {                    \
            __VA_ARGS__      \
        }                    \
    }

#define GROW(...)             \
    (ui_size)                 \
    {                         \
        .type = UI_SIZE_GROW, \
        .min_max =            \
        {                     \
            __VA_ARGS__       \
        }                     \
    }

static inline ui_padding PAD(f32 left, f32 right, f32 top, f32 bottom)
{
    return (ui_padding){
        .left   = left,
        .right  = right,
        .top    = top,
        .bottom = bottom,
    };
}

static inline ui_padding PAD_ALL(f32 padding)
{
    return (ui_padding){
        .left   = padding,
        .right  = padding,
        .top    = padding,
        .bottom = padding,
    };
}

static inline ui_position RELATIVE(f32 x, f32 y)
{
    return (ui_position){
        .type     = POSITION_RELATIVE,
        .relative = v2make(x, y),
        .z_order  = 0,
    };
}

static inline ui_position ABSOLUTE(f32 x, f32 y, u32 z)
{
    return (ui_position){
        .type     = POSITION_ABSOLUTE,
        .absolute = v2make(x, y),
        .z_order  = z,
    };
}

static inline ui_element *PARENT(void)
{
    return ui_open_elements_peek(-1);
}

static inline ui_element *CURRENT(void)
{
    return ui_open_elements_peek(0);
}

static inline bool HOVERED(void)
{
    return ui_hovered(CURRENT()->id);
}

static inline v2 WORLD_POS(ui_id id)
{
    return ui_world_data(id, false);
}

static inline v2 SIZE(ui_id id)
{
    return ui_world_data(id, true);
}

static inline v2 GET_POS(ui_position pos)
{
    switch (pos.type)
    {
    case POSITION_ABSOLUTE:
        return pos.absolute;
    case POSITION_RELATIVE:
        return pos.relative;
    default:
        return v2zero();
    }
}

static inline bool MOUSE_OVER_UI(void)
{
    ui_context *ctx = ui_context_get();
    if (ctx && ctx->pointer_over_ids)
    {
        return darray_len(ctx->pointer_over_ids) > 0;
    }
    return false;
}

static inline v2 *POS(ui_element *element)
{
    switch (element->pos.type)
    {
    case POSITION_RELATIVE:
        return &element->pos.relative;
    case POSITION_ABSOLUTE:
        return &element->pos.absolute;
    default:
        return NULL;
    }
}

#define UI(...)                                                    \
    for (u32 _ = (ui_open(CURRENT(), (ui_element)__VA_ARGS__), 0); \
         _ < 1;                                                    \
         _ = 1, ui_close(ui_open_elements_pop()))
