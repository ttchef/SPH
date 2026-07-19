

#include <SDL3/SDL_timer.h>
#include <sph/time.h>
#include <sph/simulation.h>
#include <vk/context.h>

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#define START_WINDOW_WIDTH 2560
#define START_WINDOW_HEIGHT 1440

typedef struct window
{
    SDL_Window *handle;

    // NOTE: Automatically gets updated
    u32 width;
    u32 height;
} window;

typedef struct app_state
{
    time time;
    simulation simulation;
    window window;
    vulkan_context vulkan;
} app_state;

SDL_AppResult SDL_AppInit(void **appstate, i32 argc, char *argv[])
{
    UNUSED(argc);
    UNUSED(argv);
        
    app_state *state = SDL_calloc(1, sizeof(app_state));
    assert(state);

    *appstate = state;
    
    SDL_SetAppMetadata("SPH Simulation", "1.0", NULL);

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("[ENGINE] Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    state->window.width = START_WINDOW_WIDTH;
    state->window.height = START_WINDOW_HEIGHT;
    state->window.handle = SDL_CreateWindow("SPH", START_WINDOW_WIDTH, START_WINDOW_HEIGHT, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
    assert(state->window.handle);

    if (!vulkan_init(state->window.handle, &state->vulkan))
    {
        SDL_Log("[ENGINE] Failed to initialize vulkan.");
        return SDL_APP_FAILURE;
    }

    if (!simulation_create(&state->vulkan, START_WINDOW_WIDTH, START_WINDOW_HEIGHT, &state->simulation))
    {
        SDL_Log("[ENGINE] Failed to initialize simulation.");
        return SDL_APP_FAILURE;
    }

    return SDL_APP_CONTINUE; 
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    app_state *state = (app_state *)appstate;
    assert(state);

    UNUSED(state);
    
    if (event->type == SDL_EVENT_QUIT)
    {
        return SDL_APP_SUCCESS;
    }
    if (event->type == SDL_EVENT_WINDOW_RESIZED)
    {
		i32 w, h;
		SDL_GetWindowSize(state->window.handle, &w, &h);
        state->window.width = (u32)w;
        state->window.height = (u32)h;
		
        vulkan_resize(&state->vulkan, (u32)w, (u32)h);
    }
    return SDL_APP_CONTINUE;  
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
    app_state *state = (app_state *)appstate;
    assert(state);

    vulkan_context *vulkan = &state->vulkan;
    assert(vulkan);

    time_update(&state->time);

    static f32 counter;
    counter += state->time.delta;
    if (counter > 1.0f)
    {
        SDL_Log("[ENGINE] Fps: %.4f\n", 1.0f / state->time.delta);
        counter = 0.0f;
    }

    simulation_update(vulkan, state->window.width, state->window.height, state->time, &state->simulation);
    vulkan_draw(vulkan, state->window.width, state->window.height);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    UNUSED(result);
    
    app_state *state = (app_state *)appstate;
    assert(state);

    simulation_destroy(&state->vulkan, &state->simulation);
    vulkan_deinit(&state->vulkan);
}

