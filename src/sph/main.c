

#include <sph/simulation.h>

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

SDL_AppResult SDL_AppInit(void **appstate, i32 argc, char *argv[])
{
    UNUSED(argc);
    UNUSED(argv);

    simulation *simulation = SDL_calloc(1, sizeof(*simulation));
    assert(simulation);

    *appstate = simulation;

    SDL_SetAppMetadata("SPH Simulation", "1.0", NULL);

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("[ENGINE] Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!simulation_create(simulation))
    {
        SDL_Log("[ENGINE] Failed to initialize simulation.");
        return SDL_APP_FAILURE;
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    simulation *simulation = appstate;
    assert(simulation);

    simulation_event(simulation, event);

    if (event->type == SDL_EVENT_QUIT)
    {
        return SDL_APP_SUCCESS;
    }
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
    simulation *simulation = appstate;
    assert(simulation);

    simulation_update(simulation);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    UNUSED(result);

    simulation *simulation = appstate;
    assert(simulation);

    simulation_destroy(simulation);
}
