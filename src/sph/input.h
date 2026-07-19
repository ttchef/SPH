
#pragma once

#include <types.h>
#include <math/types.h>

#include <SDL3/SDL.h>

typedef struct input_action
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

	INPUT_LMB,
	INPUT_RMB,

	// NOTE: Used for unkown keys
	INPUT_UNKOWN,
	INPUT_COUNT,	
};

typedef struct input
{
	input_action actions[INPUT_COUNT];

	v2 mouse_pos;
	v2 mouse_delta;
	bool mouse_initialized;
} input;

input input_create(void);

void input_update(input *input, SDL_Event *event);

bool input_down(input *input, u32 key);

bool input_pressed(input *input, u32 key);

bool input_released(input *input, u32 key);
