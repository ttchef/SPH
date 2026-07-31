
#pragma once

#include <math/vector.h>
#include <sph/input.h>
#include <sph/types.h>
#include <types.h>

//
// NOTE: Heavily inspired by clay: https://github.com/nicbarker/clay.git
//       This file doesnt have proper namespacing to make using it more pleasent
//

typedef u32 ui_id;

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

typedef struct ui_element
{
    ui_id     id;
    ui_layout layout;
    v2        pos;
    ui_size   width;
    ui_size   height;
    color4    color;
    f32       roundness;

    ui_padding padding;

    f32           child_gap;
    ui_alignement child_align;

    struct
    {
        const char *chars;
        u32         font_size;
        ttf_font   *font;
    } text;

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
        u32         count;
    } open_elements;

    // NOTE: Only have one root element for now
    ui_element *root;

    u32    element_count;
    ui_id *pointer_over_ids;
} ui_context;

void ui_update(input *input);

ui_element *ui_open(ui_element *parent, ui_element element);

void ui_close(ui_element *element);

void ui_draw(simulation *simulation);

ui_context *ui_context_get(void);

ui_element *ui_open_elements_pop(void);

ui_element *ui_open_elements_peek(void);

bool ui_hovered(ui_id id);

#define UI(...)                                                                  \
    for (u32 _ = (ui_open(ui_open_elements_peek(), (ui_element)__VA_ARGS__), 0); \
         _ < 1;                                                                  \
         _ = 1, ui_close(ui_open_elements_pop()))

#define FIXED(size)                   \
    (ui_size)                         \
    {                                 \
        .type        = UI_SIZE_FIXED, \
        .min_max.min = size,          \
        .min_max.max = size,          \
    }

#define PERCENT(size)               \
    (ui_size)                       \
    {                               \
        .type    = UI_SIZE_PERCENT, \
        .percent = size,            \
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

#define PAD(left_value, right_value, top_value, bottom_value) \
    (ui_padding)                                              \
    {                                                         \
        .left   = left_value,                                 \
        .right  = right_value,                                \
        .top    = top_value,                                  \
        .bottom = bottom_value,                               \
    }

#define PAD_ALL(value)   \
    (ui_padding)         \
    {                    \
        .left   = value, \
        .right  = value, \
        .top    = value, \
        .bottom = value, \
    }

#define ALIGN(x_type, y_type) \
    (ui_alignement)           \
    {                         \
        .x = x_type,          \
        .y = y_type,          \
    }

#define CURRENT(...) \
    ui_open_elements_peek()

#define HOVERED(...) \
    ui_hovered(ui_open_elements_peek()->id)

