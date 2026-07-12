

#include <sph/time.h>
#include <vk/context.h>

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

typedef struct AppState
{
    Time time;
    SDL_Window *window;
    VulkanContext vulkan;
} AppState;

SDL_AppResult SDL_AppInit(void **appstate, i32 argc, char *argv[])
{
    UNUSED(argc);
    UNUSED(argv);
        
    AppState *state = SDL_calloc(1, sizeof(AppState));
    assert(state);

    *appstate = state;
    
    SDL_SetAppMetadata("SPH Simulation", "1.0", NULL);

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("[ENGINE] Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    state->window = SDL_CreateWindow("SPH", 800, 600, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
    assert(state->window);

    if (!vulkan_init(state->window, &state->vulkan))
    {
        SDL_Log("[ENGINE] Failed to init vulkan.");
        return SDL_APP_FAILURE;
    }

    return SDL_APP_CONTINUE; 
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    AppState *state = (AppState *)appstate;
    assert(state);

    UNUSED(state);
    
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }
    return SDL_APP_CONTINUE;  
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
    AppState *state = (AppState *)appstate;
    assert(state);

    time_update(&state->time);    


    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    UNUSED(result);
    
    AppState *state = (AppState *)appstate;
    assert(state);

    vulkan_deinit(&state->vulkan);
}

