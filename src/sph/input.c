
#include <math/vector.h>
#include <sph/input.h>
#include <sph/window.h>
#include <types.h>

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_scancode.h>

input input_create(void)
{
    input result = {0};

    return result;
}

static u32 scancode_to_enum(u32 scancode)
{
    switch (scancode)
    {
    case SDL_SCANCODE_W:
        return INPUT_W;
    case SDL_SCANCODE_A:
        return INPUT_A;
    case SDL_SCANCODE_S:
        return INPUT_S;
    case SDL_SCANCODE_D:
        return INPUT_D;
    case SDL_SCANCODE_SPACE:
        return INPUT_SPACE;
    case SDL_SCANCODE_LSHIFT:
        return INPUT_LSHIFT;
    }

    return INPUT_UNKOWN;
}

static u32 mouse_button_to_enum(u32 button)
{
    switch (button)
    {
    case SDL_BUTTON_LEFT:
        return INPUT_LMB;
    case SDL_BUTTON_RIGHT:
        return INPUT_RMB;
    }
    return INPUT_UNKOWN;
}

void input_update(input *input, window *window)
{
    for (u32 i = 0; i < INPUT_COUNT; i++)
    {
        input->actions[i].pressed  = false;
        input->actions[i].released = false;
    }

    f32 x, y;
    SDL_GetRelativeMouseState(&x, &y);
    input->mouse_delta = v2make(x, y);

    if (input->relative_mouse)
    {
        SDL_WarpMouseInWindow(window->handle, window->width / 2.0f, window->height / 2.0f);
    }
}

void input_event(input *input, SDL_Event *event)
{
    switch (event->type)
    {
    case SDL_EVENT_KEY_DOWN:
    {
        u32 index                  = scancode_to_enum(event->key.scancode);
        input->actions[index].down = true;

        if (!event->key.repeat)
        {
            input->actions[index].pressed = true;
        }
    }
    break;
    case SDL_EVENT_KEY_UP:
    {
        u32 index                      = scancode_to_enum(event->key.scancode);
        input->actions[index].down     = false;
        input->actions[index].released = true;
    }
    break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    {
        u32 index                     = mouse_button_to_enum(event->button.button);
        input->actions[index].down    = true;
        input->actions[index].pressed = true;
    }
    break;
    case SDL_EVENT_MOUSE_BUTTON_UP:
    {
        u32 index                      = mouse_button_to_enum(event->button.button);
        input->actions[index].down     = false;
        input->actions[index].released = true;
    }
    break;
    case SDL_EVENT_MOUSE_MOTION:
    {
        v2 pos = v2make(event->motion.x, event->motion.y);
        input->mouse_pos         = pos;
    }
    default:
        break;
    }
}

bool input_down(input *input, u32 key)
{
    assert(key < INPUT_COUNT);
    return input->actions[key].down;
}

bool input_pressed(input *input, u32 key)
{
    assert(key < INPUT_COUNT);
    return input->actions[key].pressed;
}

bool input_released(input *input, u32 key)
{
    assert(key < INPUT_COUNT);
    return input->actions[key].released;
}

void input_relative_mouse(input *input, window *window, bool on)
{
    SDL_SetWindowRelativeMouseMode(window->handle, on);
    input->relative_mouse = on;
}

