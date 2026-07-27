
#include <types.h>
#include <sph/window.h>
#include <sph/camera.h>
#include <sph/input.h>
#include <sph/time.h>
#include <sph/ttf.h>
#include <sph/pipelines.h>
#include <vk/context.h>

typedef struct
{
	window window;
	input input;
	camera camera;
	time time;
	vulkan vulkan;

	vulkan_pipeline_id pipelines[PIPELINE_COUNT];

	vulkan_sampler linear_sampler;
	vulkan_image test_texture;
	ttf_font jet_brains;
} simulation;

bool simulation_create(simulation *simulation);

void simulation_event(simulation *simulation, SDL_Event *event);

void simulation_update(simulation *simulation);

void simulation_destroy(simulation *simulation);
