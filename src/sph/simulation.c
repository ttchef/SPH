
#include "sph/window.h"
#include <sph/simulation.h>

bool simulation_create(simulation *simulation)
{
	if (!window_create(&simulation->window, 2560, 1440))
	{
		SDL_Log("[ENGINE] Failed to create window.");
		return false;
	}

	if (!vulkan_init(simulation->window.handle, &simulation->vulkan))
	{
		SDL_Log("[ENGINE] Failed to init vulkan.");
		return false;
	}

	simulation->camera = camera_create();
	simulation->input = input_create();
	
	return true;
}

void simulation_event(simulation *simulation, SDL_Event *event)
{
	input_update(&simulation->input, event);

	if (event->type == SDL_EVENT_WINDOW_RESIZED)
    {
		window_resize(&simulation->window);		
        vulkan_resize(&simulation->vulkan, simulation->window.width, simulation->window.height);
    }
}

void simulation_update(simulation *simulation)
{
	time_update(&simulation->time);
	camera_update(&simulation->camera, &simulation->window, &simulation->input, simulation->time.delta);

	input *input = &simulation->input;

	if (input_pressed(input, INPUT_W))
	{
		SDL_Log("W pressed.");
	}

	vulkan_draw(&simulation->vulkan, simulation->window.width, simulation->window.height);
	input_update(&simulation->input, NULL);
}

void simulation_destroy(simulation *simulation)
{
	vulkan_deinit(&simulation->vulkan);
	window_destroy(&simulation->window);
}
