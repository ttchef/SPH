
#include "vk/buffer.h"
#include <SDL3/SDL_stdinc.h>
#include <sph/simulation.h>

#include <SDL3/SDL_log.h>
#include <vulkan/vulkan_core.h>

#define PARTICLE_DISTANCE 30.0f

bool simulation_init(VulkanContext *ctx, u32 window_width, u32 window_height, Simulation *simulation)
{
	assert(ctx);
	assert(simulation);

	Particle *particles = SDL_calloc(PARTICLE_COUNT, sizeof(Particle));
	assert(particles);

	V2 start = v2(window_width / 2.0f - PARTICLE_DISTANCE * SDL_sqrtf(PARTICLE_COUNT) * 0.5f, window_height / 2.0f - PARTICLE_DISTANCE * SDL_sqrtf(PARTICLE_COUNT) * 0.5f);

	for (u32 i = 0; i < PARTICLE_COUNT; i++)
	{
		Particle *p = &particles[i];
		assert(p);

		*p = (Particle){
			.mass = 1.0f,
			.pos = v2((i % (u32)SDL_sqrtf(PARTICLE_COUNT)) * PARTICLE_DISTANCE + start.x, (i / SDL_sqrtf(PARTICLE_COUNT)) * PARTICLE_DISTANCE + start.y), 
		};
	}

	vulkan_buffer_device_local_init(ctx, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, PARTICLE_COUNT * sizeof(Particle), particles, &simulation->particles);

	return true;
}

void simulation_deinit(VulkanContext *ctx, Simulation *simulation)
{
	assert(ctx);
	assert(simulation);

	vulkan_buffer_deinit(ctx, &simulation->particles);
}
