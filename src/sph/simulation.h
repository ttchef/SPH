
#include <types.h>
#include <sph/types.h>
#include <sph/window.h>
#include <sph/camera.h>
#include <sph/input.h>
#include <sph/time.h>
#include <sph/ttf.h>
#include <sph/ui_layout.h>
#include <sph/pipelines.h>
#include <vk/context.h>

struct simulation
{
	window window;
	input input;
	camera camera;
	time time;
	vulkan vulkan;
	ui_layout_context ui_layout;

	vulkan_pipeline_id pipelines[PIPELINE_COUNT];

	vulkan_sampler linear_sampler;
	vulkan_image test_texture;
	ttf_font jet_brains;

	cube bounding_box;
};

bool simulation_create(simulation *simulation);

void simulation_event(simulation *simulation, SDL_Event *event);

void simulation_update(simulation *simulation);

void simulation_destroy(simulation *simulation);

//
// NOTE: Helpers
//
f32 measure_text(ttf_font *font, u32 font_size, const char *format, ...);

void draw_text(simulation *simulation, ttf_font *font, v2 pos, u32 font_size, const char *format, ...);
 
void draw_quad(simulation *simulation, v2 pos, v2 scale, f32 roundness, color4 color, vulkan_image *image, vulkan_sampler *sampler);

void draw_cube_lines(simulation *simulation, v3 pos, v3 scale);
