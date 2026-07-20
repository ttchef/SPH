
#include <types.h>

#include <vk/buffer.h>
#include <vk/pipeline.h>
#include <vk/command.h>
#include <vk/types.h>
#include <sph/time.h>
#include <sph/render.h>
#include <sph/ui.h>
#include <sph/camera.h>
#include <math/types.h>

static const u32 PARTICLE_COUNT = 100024;

// NOTE: IMPORTANT!!! Needs to match with GPU implementation
typedef struct particle
{
	v4 pos;
	v4 vel;
	f32 mass;
	f32 density;

	f32 padding[2];
} particle;

// NOTE: IMPORTANT!!! Needs to match with GPU implementation
typedef struct spatial_lookup_entry
{
	u32 particle_index;
	u32 cell_key;
} spatial_lookup_entry;

typedef struct simulation
{
	vulkan_buffer particles[FRAMES_IN_FLIGHT];
	vulkan_buffer spatial_lookup[FRAMES_IN_FLIGHT];
	vulkan_buffer start_indices[FRAMES_IN_FLIGHT];
	vulkan_buffer histograms[FRAMES_IN_FLIGHT];

	vulkan_pipeline_id spatial_lookup_pipelines[FRAMES_IN_FLIGHT];
	vulkan_pipeline_id radixsort_histogram_pipelines[FRAMES_IN_FLIGHT];
	vulkan_pipeline_id radixsort_pipelines[FRAMES_IN_FLIGHT];
	vulkan_pipeline_id start_indices_pipelines[FRAMES_IN_FLIGHT];
	vulkan_pipeline_id density_pipelines[FRAMES_IN_FLIGHT];
	vulkan_pipeline_id update_pipelines[FRAMES_IN_FLIGHT];
	vulkan_pipeline_id render_pipeline[FRAMES_IN_FLIGHT];

	u32 sim_buffer;
	f64 accumulator;

	cube boundary_cube;
} simulation;

bool simulation_create(vulkan_context *vulkan, simulation *simulation);

void simulation_update(vulkan_context *vulkan, u32 window_width, u32 window_height, time time, camera camera, render *render, ui *ui, simulation *simulation);

void simulation_destroy(vulkan_context *vulkan, simulation *simulation);
