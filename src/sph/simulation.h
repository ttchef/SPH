
#include <types.h>

#include <vk/buffer.h>
#include <vk/pipeline.h>
#include <vk/command.h>
#include <vk/types.h>

#include <math/types.h>

#define PARTICLE_COUNT 306

// NOTE: IMPORTANT!!! Needs to match with GPU implementation
typedef struct particle
{
	v2 pos;
	v2 vel;
	f32 mass;
	f32 density;
} particle;

typedef struct simulation
{
	vulkan_buffer particles;
	vulkan_pipeline_id update_pipeline;
	vulkan_pipeline_id render_pipeline;
} simulation;

bool simulation_create(vulkan_context *vulkan, u32 window_width, u32 window_height, simulation *simulation);

void simulation_update(vulkan_context *vulkan, u32 window_width, u32 window_height, simulation *simulation);

void simulation_destroy(vulkan_context *vulkan, simulation *simulation);
