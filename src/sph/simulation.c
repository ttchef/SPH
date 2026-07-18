
#include "vk/types.h"
#include <sph/simulation.h>
#include <math/matrix.h>
#include <vk/context.h>

#include <SDL3/SDL_log.h>

#define PARTICLE_DISTANCE 40.0f

bool simulation_create(vulkan_context *vulkan, u32 window_width, u32 window_height, simulation *simulation)
{
	assert(vulkan);
	assert(simulation);

	particle *particles = SDL_calloc(PARTICLE_COUNT, sizeof(particle));
	assert(particles);

	v2 start = v2make(window_width / 2.0f - PARTICLE_DISTANCE * SDL_sqrtf(PARTICLE_COUNT) * 0.5f, window_height / 2.0f - PARTICLE_DISTANCE * SDL_sqrtf(PARTICLE_COUNT) * 0.5f);

	for (u32 i = 0; i < PARTICLE_COUNT; i++)
	{
		particle *p = &particles[i];
		assert(p);

		*p = (particle){
			.mass = 1.0f,
			.pos = v2make((i % (u32)SDL_sqrtf(PARTICLE_COUNT)) * PARTICLE_DISTANCE + start.x, ((u32)i / (u32)SDL_sqrtf(PARTICLE_COUNT)) * PARTICLE_DISTANCE + start.y), 
		};
	}

	for (u32 i = 0; i < FRAMES_IN_FLIGHT; i++)
	{
		VkBufferUsageFlags usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		vulkan_buffer_device_local_create(vulkan, usage, PARTICLE_COUNT * sizeof(particle), particles, &simulation->particles[i]);
	}

	vulkan_pipeline_desc render_description = vulkan_pipeline_default(VULKAN_PIPELINE_TYPE_GRAPHICS);

	VkVertexInputAttributeDescription vertex_attributes[] = {
		// NOTE: Position
		{
			.binding = 0,
			.location = 0,
			.format = VK_FORMAT_R32G32_SFLOAT,
			.offset = 0,
		},
		// NOTE: Velocity
		{
			.binding = 0,
			.location = 1,
			.format = VK_FORMAT_R32G32_SFLOAT,
			.offset = sizeof(f32) * 2,
		},
		// NOTE: Mass
		{
			.binding = 0,
			.location = 2,
			.format = VK_FORMAT_R32_SFLOAT,
			.offset = sizeof(f32) * 4,
		},
		// NOTE: Density
		{
			.binding = 0,
			.location = 3,
			.format = VK_FORMAT_R32_SFLOAT,
			.offset = sizeof(f32) * 5,
		},
	};

	vulkan_pipeline_desc_set_vertex_input(&render_description, sizeof(particle), vertex_attributes, ARRAY_COUNT(vertex_attributes));

	simulation->render_pipeline = vulkan_pipeline_create(vulkan, &render_description);
	assert(simulation->render_pipeline != INVALID_PIPELINE);

	for (u32 i = 0; i < FRAMES_IN_FLIGHT; i++)
	{
		vulkan_pipeline_desc update_description = vulkan_pipeline_default(VULKAN_PIPELINE_TYPE_COMPUTE);

		u32 read_buffer = i;
		vulkan_pipeline_desc_add_storage_buffer(&update_description, vulkan, simulation->particles[read_buffer], 0, VK_SHADER_STAGE_COMPUTE_BIT);

		u32 write_buffer = (i + 1) % FRAMES_IN_FLIGHT;
		vulkan_pipeline_desc_add_storage_buffer(&update_description, vulkan, simulation->particles[write_buffer], 0, VK_SHADER_STAGE_COMPUTE_BIT);

		simulation->update_pipeline[i] = vulkan_pipeline_create(vulkan, &update_description);
		assert(simulation->update_pipeline[i] != INVALID_PIPELINE);	
	}

	return true;
}

void simulation_update(vulkan_context *vulkan, u32 window_width, u32 window_height, simulation *simulation)
{
	assert(vulkan);
	assert(simulation);

	vulkan_command_bind_pipeline(vulkan, simulation->update_pipeline[vulkan_frame_index(vulkan)]);
	vulkan_command_dispatch(vulkan, 1, 1, 1);

	vulkan_command_begin_rendering(vulkan);

	m4 orthographic = m4orthographic(0, window_width, 0, window_height, -1.0f, 1.0f);

	vulkan_command_bind_pipeline(vulkan, simulation->render_pipeline);
	vulkan_command_push_constants(vulkan, sizeof(orthographic), &orthographic, VK_SHADER_STAGE_VERTEX_BIT, simulation->render_pipeline);

	u32 write_buffer = (vulkan_frame_index(vulkan) + 1) % FRAMES_IN_FLIGHT;
	vulkan_command_bind_vertex_buffer(vulkan, simulation->particles[write_buffer], simulation->render_pipeline);

	vulkan_command_draw(vulkan, PARTICLE_COUNT);

	vulkan_command_end_rendering(vulkan);
}

void simulation_destroy(vulkan_context *vulkan, simulation *simulation)
{
	assert(vulkan);
	assert(simulation);

	for (u32 i = 0; i < FRAMES_IN_FLIGHT; i++)
	{
		vulkan_buffer_destroy(vulkan, &simulation->particles[i]);
	}
}
