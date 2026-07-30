
#pragma once

#include <math/vector.h>
#include <sph/types.h>
#include <types.h>

//
// NOTE: Heavily inspired by clay: https://github.com/nicbarker/clay.git
//       This file doesnt have proper namespacing to make using it more pleasent
//

typedef enum
{
    UI_SIZE_FIXED,
    UI_SIZE_PERCENT,
    UI_SIZE_FIT,
    UI_SIZE_GROW,
} ui_size_type;

typedef struct
{
    ui_size_type type;
    f32          value;
    f32          min;
    f32          max;
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
    TOP,
    CENTER,
    BOTTOM,
} ui_alignement;

typedef struct ui_element
{
    ui_layout     layout;
    v2            pos;
    ui_size       width;
    ui_size       height;
    ui_padding    padding;
    color4        color;
    f32           child_gap;
    f32           roundness;
    ui_alignement align_x;
    ui_alignement align_y;

    const char *text;
    u32         font_size;
    ttf_font   *font;

    struct ui_element *parent;
    // NOTE: darray.h
    struct ui_element *children;
} ui_element;

typedef struct
{
    struct
    {
        // NOTE: Max of 32 element depth
        ui_element *elements[32];
        u32         element_count;
    } open_elements;

    // NOTE: Only have one root element for now
    ui_element *root;

    v2 mouse_pos;
} ui_context;

void ui_update(void);

ui_element *ui_open(ui_element *parent, ui_element *element);

void ui_close(ui_element *element);

void ui_draw(simulation *simulation);

ui_context *ui_context_get(void);

ui_element *ui_open_elements_pop(void);

ui_element *ui_open_elements_peek(void);

#define UI(...) \
    for (u32 _ = (ui_open(ui_open_elements_peek(), __VA_ARGS__), 0); \
        _ < 1; \
        _ = 1, ui_close(ui_open_elements_pop()))
        

// NOTE: Struct builders
static inline ui_size fixed(f32 value)
{
    return (ui_size){
        .type  = UI_SIZE_FIXED,
        .value = value,
    };
}

static inline ui_size percent(f32 value, f32 min, f32 max)
{
    return (ui_size){
        .type  = UI_SIZE_PERCENT,
        .min   = min,
        .max   = max,
        .value = value,
    };
}

static inline ui_size fit(f32 min, f32 max)
{
    return (ui_size){
        .type  = UI_SIZE_FIT,
        .min   = min,
        .max   = max,
        .value = min,
    };
}

static inline ui_size grow(f32 min, f32 max)
{
    return (ui_size){
        .type  = UI_SIZE_GROW,
        .min   = min,
        .max   = max,
        .value = min,
    };
}

static inline ui_padding padding(f32 left, f32 right, f32 top, f32 bottom)
{
    return (ui_padding){
        .left   = left,
        .right  = right,
        .top    = top,
        .bottom = bottom,
    };
}

