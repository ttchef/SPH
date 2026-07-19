
#include <types.h>

#include <vk/buffer.h>
#include <vk/pipeline.h>
#include <vk/command.h>
#include <vk/types.h>
#include <sph/time.h>
#include <math/types.h>

#define PARTICLE_COUNT 10024

// NOTE: IMPORTANT!!! Needs to match with GPU implementation
typedef struct particle
{
	v2 pos;
	v2 vel;
	f32 mass;
	f32 density;
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

	vulkan_pipeline_id spatial_lookup_pipelines[FRAMES_IN_FLIGHT];
	vulkan_pipeline_id density_pipelines[FRAMES_IN_FLIGHT];
	vulkan_pipeline_id update_pipelines[FRAMES_IN_FLIGHT];

	vulkan_pipeline_id render_pipeline;

	u32 sim_buffer;
	f64 accumulator;
} simulation;

bool simulation_create(vulkan_context *vulkan, u32 window_width, u32 window_height, simulation *simulation);

void simulation_update(vulkan_context *vulkan, u32 window_width, u32 window_height, time time, simulation *simulation);

void simulation_destroy(vulkan_context *vulkan, simulation *simulation);
