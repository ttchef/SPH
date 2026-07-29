
#pragma once

#include <types.h>
#include <sph/types.h>
#include <math/vector.h>

//
// NOTE: Heavily inspired by clay: https://github.com/nicbarker/clay.git
//       This file doesnt have proper namespacing to make using it more pleasent
//

typedef enum
{
    UI_SIZE_FIXED,
    UI_SIZE_FIT,
    UI_SIZE_GROW,
} ui_size_type;

typedef struct
{
    ui_size_type type;
    f32 value;
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

typedef struct ui_element
{
    ui_layout layout;
    v2 pos;
    ui_size width;
    ui_size height;
    ui_padding padding;
    color4 color;
    f32 child_gap;

    struct ui_element *parent;
    // NOTE: darray.h
    struct ui_element *children;
} ui_element;

ui_element *ui_open(ui_element *parent, ui_element *element);

void ui_close(ui_element *element);

void ui_draw(simulation *simulation, ui_element *root);

void ui_destroy(ui_element *root);

// NOTE: Struct builders
static inline ui_size fixed(f32 value)
{
    return (ui_size){
        .type = UI_SIZE_FIXED,
        .value = value,
    };
}

static inline ui_size fit(f32 value)
{
    return (ui_size){
        .type = UI_SIZE_FIT,
        .value = value,
    };
}

static inline ui_size grow(f32 value)
{
    return (ui_size){
        .type = UI_SIZE_GROW,
        .value = value,
    };
}

static inline ui_padding padding(f32 left, f32 right, f32 top, f32 bottom)
{
    return (ui_padding){
        .left = left,
        .right = right,
        .top = top,
        .bottom = bottom,
    };
}
