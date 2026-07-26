
#include "vk/command.h"
#include "vk/pipeline.h"
#include <sph/simulation.h>

bool simulation_create(simulation *simulation)
{
	if (!window_create(&simulation->window, 2560, 1440))
	{
		SDL_Log("[ENGINE] Failed to create window.");
		return false;
	}

	if (!vulkan_create(simulation->window.handle, &simulation->vulkan))
	{
		SDL_Log("[ENGINE] Failed to init vulkan.");
		return false;
	}

	simulation->camera = camera_create();
	simulation->input = input_create();

	vulkan_pipeline_desc desc = vulkan_pipeline_default(VULKAN_PIPELINE_TYPE_GRAPHICS);

	vulkan_pipeline_desc_set_shaders(&desc, "hello.vert.spv", "hello.frag.spv", NULL);
	// vulkan_pipeline_desc_set_shaders_entries(&desc, "vertexMain", "fragmentMain", NULL);
	
	simulation->pipeline = vulkan_pipeline_create(&simulation->vulkan, &desc);
	assert(simulation->pipeline != INVALID_PIPELINE);
	
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

	window *window = &simulation->window;
	vulkan *vulkan = &simulation->vulkan;
	input *input = &simulation->input;

	if (input_pressed(input, INPUT_W))
	{
		SDL_Log("W pressed.");
	}

	vulkan_command_begin_rendering(vulkan);
	vulkan_command_set_viewport(vulkan, 0, 0, window->width, window->height);
	vulkan_command_bind_pipeline(vulkan, simulation->pipeline);
	vulkan_command_draw(vulkan, 6);
	vulkan_command_end_rendering(vulkan);

	vulkan_draw(vulkan, simulation->window.width, simulation->window.height);
	input_update(&simulation->input, NULL);
}

void simulation_destroy(simulation *simulation)
{
	vulkan_destroy(&simulation->vulkan);
	window_destroy(&simulation->window);
}
