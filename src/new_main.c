

#include <types.h>

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>


typedef struct AppState
{
    SDL_Window *window;
    SDL_Renderer *renderer;
} AppState;

SDL_AppResult SDL_AppInit(void **appstate, i32 argc, char *argv[])
{
    UNUSED(argc);
    UNUSED(argv);
        
    AppState *state = SDL_calloc(1, sizeof(AppState));
    assert(state);
    
    SDL_SetAppMetadata("SPH Simulation", "1.0", NULL);

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!SDL_CreateWindowAndRenderer("Sph", 640, 480, SDL_WINDOW_RESIZABLE, &state->window, &state->renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_SetRenderLogicalPresentation(state->renderer, 640, 480, SDL_LOGICAL_PRESENTATION_LETTERBOX);

    *appstate = state;

    return SDL_APP_CONTINUE; 
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    AppState *state = (AppState *)appstate;
    assert(state);
    
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }
    return SDL_APP_CONTINUE;  
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
    AppState *state = (AppState *)appstate;
    assert(state);
    
    const f64 now = ((f64)SDL_GetTicks()) / 1000.0;
    const f32 red = (f32) (0.5 + 0.5 * SDL_sin(now));
    const f32 green = (f32) (0.5 + 0.5 * SDL_sin(now + SDL_PI_D * 2 / 3));
    const f32 blue = (f32) (0.5 + 0.5 * SDL_sin(now + SDL_PI_D * 4 / 3));
    SDL_SetRenderDrawColorFloat(state->renderer, red, green, blue, SDL_ALPHA_OPAQUE_FLOAT);

    SDL_RenderClear(state->renderer);

    SDL_RenderPresent(state->renderer);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    UNUSED(appstate);
    UNUSED(result);
}

