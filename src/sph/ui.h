
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
    SIZE_FIXED,
    SIZE_FIT,
} ui_size_type;

typedef struct
{
    ui_size_type type;
    f32 width;
    f32 height;
} ui_size;

static inline ui_size size(ui_size_type type, f32 width, f32 height)
{
    return (ui_size){
        .type = type,
        .width = width,
        .height = height,
    };
}

typedef struct
{
    f32 left;
    f32 right;
    f32 top;
    f32 bottom;
} ui_padding;

static inline ui_padding padding(f32 left, f32 right, f32 top, f32 bottom)
{
    return (ui_padding){
        .left = left,
        .right = right,
        .top = top,
        .bottom = bottom,
    };
}

typedef enum
{
    LAYOUT_TO_RIGHT,  
    LAYOUT_TO_BOTTOM,
} ui_layout;

typedef struct ui_element
{
    ui_layout layout;
    v2 pos;
    ui_size size;
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

