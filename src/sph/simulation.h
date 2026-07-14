
#include <types.h>

#include <vk/buffer.h>
#include <vk/types.h>

#include <math/types.h>

#define PARTICLE_COUNT 306

// NOTE: IMPORTANT!!! Needs to match with GPU implementation
typedef struct Particle
{
	v2 pos;
	v2 vel;
	f32 mass;
	f32 density;
} Particle;

typedef struct Simulation
{
	vulkan_buffer particles;
} Simulation;

bool simulation_init(vulkan_context *ctx, u32 window_width, u32 window_height, Simulation *simulation);

void simulation_deinit(vulkan_context *ctx, Simulation *simulation);
