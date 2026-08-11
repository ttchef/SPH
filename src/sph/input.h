
#pragma once

#include <math/types.h>
#include <sph/types.h>
#include <types.h>

#include <SDL3/SDL.h>

typedef struct
{
    bool released;
    bool pressed;
    bool down;
} input_action;

enum
{
    INPUT_W,
    INPUT_A,
    INPUT_S,
    INPUT_D,

    INPUT_SPACE,
    INPUT_LSHIFT,
    INPUT_LCTRL,
    INPUT_BACKSPACE,
    INPUT_ENTER,

    INPUT_LMB,
    INPUT_RMB,

    // NOTE: Used for unkown keys
    INPUT_UNKOWN,
    INPUT_COUNT,
};

typedef struct
{
    input_action actions[INPUT_COUNT];

    v2 mouse_pos;
    v2 mouse_delta;
    // NOTE: Position of the mouse when the last press occured
    v2 mouse_press_pos;

    bool relative_mouse;

    char text[256];
    u32 text_len;
} input;

input input_create(void);

void input_update(input *input, window *window);

void input_event(input *input, SDL_Event *event);

bool input_down(input *input, u32 key);

bool input_pressed(input *input, u32 key);

bool input_released(input *input, u32 key);

void input_relative_mouse(input *input, window *window, bool on);

const char *input_text(input *input);

u32 input_text_len(input *input);
