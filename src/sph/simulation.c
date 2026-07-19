
#include <sph/simulation.h>
#include <math/matrix.h>
#include <vk/context.h>

#include <SDL3/SDL_log.h>
#include <vulkan/vulkan_core.h>

#define PARTICLE_DISTANCE 10.0f
#define FIXED_DT (1.0f / 240.0f)
#define MAX_STEPS_PER_FRAME 4

// TODO: Make particle count as a specilazation constant in the shaders

//
// NOTE: Push constants
//

typedef struct spatial_lookup_pc
{
	u32 particle_count;
} spatial_lookup_pc;

typedef struct radixsort_pc
{
	u32 element_count;
	u32 shift;
	u32 workgroup_count;
	u32 blocks_per_workgroup;	
} radixsort_pc;

typedef struct start_indices_pc
{
	u32 particle_count;
} start_indices_pc;

typedef struct density_pc
{
	u32 particle_count;
} density_pc;

typedef struct update_pc
{
	f32 dt;
	u32 window_width;
	u32 window_height;
	u32 particle_count;
} update_pc;

void simulation_measure_rest_density(vulkan_context *vulkan, simulation *simulation)
{
	vulkan_pipeline *pipeline = vulkan_pipeline_get(vulkan, simulation->density_pipelines[0]);

	VkCommandBufferAllocateInfo alloc_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = vulkan->command_handler.command_pool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1,
	};
	VkCommandBuffer cmd;
	vkAllocateCommandBuffers(vulkan->device, &alloc_info, &cmd);

	VkCommandBufferBeginInfo begin_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};
	vkBeginCommandBuffer(cmd, &begin_info);

	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->handle);
	for (u32 i = 0; i < pipeline->descriptor_count; i++)
	{
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->layout, i, 1, &pipeline->descriptors[i].set, 0, NULL);
	}

	density_pc pc = { .particle_count = PARTICLE_COUNT };
	vkCmdPushConstants(cmd, pipeline->layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(pc), &pc);

	const u32 group_count = (PARTICLE_COUNT + 255) / 256;
	vkCmdDispatch(cmd, group_count, 1, 1);

	VkMemoryBarrier barrier = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
		.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
		.dstAccessMask = VK_ACCESS_HOST_READ_BIT,
	};
	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0, 1, &barrier, 0, NULL, 0, NULL);

	vkEndCommandBuffer(cmd);

	VkSubmitInfo submit = { .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO, .commandBufferCount = 1, .pCommandBuffers = &cmd };
	vkQueueSubmit(vulkan->graphics_queue.handle, 1, &submit, VK_NULL_HANDLE);
	vkQueueWaitIdle(vulkan->graphics_queue.handle);   
	vkFreeCommandBuffers(vulkan->device, vulkan->command_handler.command_pool, 1, &cmd);

	vulkan_buffer densities;
	if (vulkan_buffer_device_local_get_data(vulkan, simulation->particles[0], &densities))
	{
		particle *particles = densities.host_visible.data;
		u32 grid_side = (u32)SDL_sqrtf(PARTICLE_COUNT);
		u32 interior_index = (grid_side / 2) * grid_side + (grid_side / 2); 
		SDL_Log("[SPH] Rest density (interior particle): %.10f", particles[interior_index].density);
	}

	vulkan_buffer_destroy(vulkan, &densities);
}

void simulation_check_sorted(vulkan_context *vulkan, simulation *simulation)
{
	VkCommandBufferAllocateInfo alloc_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = vulkan->command_handler.command_pool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1,
	};
	VkCommandBuffer cmd;
	vkAllocateCommandBuffers(vulkan->device, &alloc_info, &cmd);

	VkCommandBufferBeginInfo begin_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};
	vkBeginCommandBuffer(cmd, &begin_info);

	const u32 sim = simulation->sim_buffer;
	const u32 group_size = 256;
	const u32 group_count = (PARTICLE_COUNT + group_size - 1) / group_size;

	VkMemoryBarrier compute_barrier = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
		.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
		.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT,
	};

#define BIND_COMPUTE(pipeline_id)                                                                            \
	do                                                                                                        \
	{                                                                                                         \
		vulkan_pipeline *_p = vulkan_pipeline_get(vulkan, (pipeline_id));                                     \
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _p->handle);                                   \
		for (u32 _i = 0; _i < _p->descriptor_count; _i++)                                                     \
			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _p->layout, _i, 1, &_p->descriptors[_i].set, 0, NULL); \
	} while (0)

	// NOTE: Spatial lookup
	BIND_COMPUTE(simulation->spatial_lookup_pipelines[sim]);

	spatial_lookup_pc spatial_lookup_pc = { .particle_count = PARTICLE_COUNT };
	{
		vulkan_pipeline *p = vulkan_pipeline_get(vulkan, simulation->spatial_lookup_pipelines[sim]);
		vkCmdPushConstants(cmd, p->layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(spatial_lookup_pc), &spatial_lookup_pc);
	}
	vkCmdDispatch(cmd, group_count, 1, 1);
	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &compute_barrier, 0, NULL, 0, NULL);

	// NOTE: Sorting spatial lookup
	for (u32 i = 0; i < 4; i++)
	{
		u32 pass_buf = (sim + i) % FRAMES_IN_FLIGHT;

		radixsort_pc radixsort_pc = {
			.element_count = PARTICLE_COUNT,
			.workgroup_count = group_count,
			.blocks_per_workgroup = 32,
			.shift = 8 * i,
		};

		BIND_COMPUTE(simulation->radixsort_histogram_pipelines[pass_buf]);
		{
			vulkan_pipeline *p = vulkan_pipeline_get(vulkan, simulation->radixsort_histogram_pipelines[pass_buf]);
			vkCmdPushConstants(cmd, p->layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(radixsort_pc), &radixsort_pc);
		}
		vkCmdDispatch(cmd, group_count, 1, 1);
		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &compute_barrier, 0, NULL, 0, NULL);

		BIND_COMPUTE(simulation->radixsort_pipelines[pass_buf]);
		{
			vulkan_pipeline *p = vulkan_pipeline_get(vulkan, simulation->radixsort_pipelines[pass_buf]);
			vkCmdPushConstants(cmd, p->layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(radixsort_pc), &radixsort_pc);
		}
		vkCmdDispatch(cmd, group_count, 1, 1);
		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &compute_barrier, 0, NULL, 0, NULL);
	}

	VkMemoryBarrier host_barrier = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
		.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
		.dstAccessMask = VK_ACCESS_HOST_READ_BIT,
	};
	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0, 1, &host_barrier, 0, NULL, 0, NULL);

	vkEndCommandBuffer(cmd);

	VkSubmitInfo submit = { .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO, .commandBufferCount = 1, .pCommandBuffers = &cmd };
	vkQueueSubmit(vulkan->graphics_queue.handle, 1, &submit, VK_NULL_HANDLE);
	vkQueueWaitIdle(vulkan->graphics_queue.handle);
	vkFreeCommandBuffers(vulkan->device, vulkan->command_handler.command_pool, 1, &cmd);

	vulkan_buffer spatial_lookup_buffer;
	if (!vulkan_buffer_device_local_get_data(vulkan, simulation->spatial_lookup[sim], &spatial_lookup_buffer))
	{
		SDL_Log("[VULKAN] Failed to get buffer data.");
		return;
	}

	spatial_lookup_entry *spatial_lookup = spatial_lookup_buffer.host_visible.data;
	for (u32 i = 0; i < PARTICLE_COUNT; i++)
	{
		SDL_Log("[ENGINE] Entry %u:\n\tParticle Index: %u\n\tCell Key: %u", i, spatial_lookup[i].particle_index, spatial_lookup[i].cell_key);
	}

	vulkan_buffer_destroy(vulkan, &spatial_lookup_buffer);
}

bool simulation_create(vulkan_context *vulkan, u32 window_width, u32 window_height, simulation *simulation)
{
	assert(vulkan);
	assert(simulation);

	simulation->accumulator = 0.0;
	simulation->sim_buffer = 0;

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
		VkBufferUsageFlags usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		vulkan_buffer_device_local_create(vulkan, usage, PARTICLE_COUNT * sizeof(particle), particles, &simulation->particles[i]);

		usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		vulkan_buffer_device_local_create(vulkan, usage, PARTICLE_COUNT * sizeof(spatial_lookup_entry), NULL, &simulation->spatial_lookup[i]);
		vulkan_buffer_device_local_create(vulkan, usage, PARTICLE_COUNT * sizeof(u32), NULL, &simulation->start_indices[i]);
		vulkan_buffer_device_local_create(vulkan, usage, PARTICLE_COUNT * sizeof(u32), NULL, &simulation->histograms[i]);
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
	vulkan_pipeline_desc_set_push_constant(&render_description, sizeof(m4), VK_SHADER_STAGE_VERTEX_BIT);
	vulkan_pipeline_desc_set_shaders(&render_description, "src/shaders/spv/shader.vert.spv", "src/shaders/spv/shader.frag.spv", NULL);

	simulation->render_pipeline = vulkan_pipeline_create(vulkan, &render_description);
	assert(simulation->render_pipeline != INVALID_PIPELINE);

	for (u32 i = 0; i < FRAMES_IN_FLIGHT; i++)
	{
		u32 read_buffer = i;
		u32 write_buffer = (i + 1) % FRAMES_IN_FLIGHT;

		vulkan_pipeline_desc spatial_lookup_description = vulkan_pipeline_default(VULKAN_PIPELINE_TYPE_COMPUTE);

		vulkan_pipeline_desc_add_storage_buffer(&spatial_lookup_description, vulkan, simulation->particles[read_buffer], 0, VK_SHADER_STAGE_COMPUTE_BIT);
		vulkan_pipeline_desc_add_storage_buffer(&spatial_lookup_description, vulkan, simulation->spatial_lookup[read_buffer], 0, VK_SHADER_STAGE_COMPUTE_BIT);
		vulkan_pipeline_desc_add_storage_buffer(&spatial_lookup_description, vulkan, simulation->start_indices[read_buffer], 0, VK_SHADER_STAGE_COMPUTE_BIT);

		vulkan_pipeline_desc_set_shaders(&spatial_lookup_description, NULL, NULL, "src/shaders/spv/spatial_lookup.comp.spv");
		vulkan_pipeline_desc_set_push_constant(&spatial_lookup_description, sizeof(spatial_lookup_pc), VK_SHADER_STAGE_COMPUTE_BIT);

		simulation->spatial_lookup_pipelines[i] = vulkan_pipeline_create(vulkan, &spatial_lookup_description);
		assert(simulation->spatial_lookup_pipelines[i] != INVALID_PIPELINE);

		vulkan_pipeline_desc radixsort_histograms_description = vulkan_pipeline_default(VULKAN_PIPELINE_TYPE_COMPUTE);

		vulkan_pipeline_desc_add_storage_buffer(&radixsort_histograms_description, vulkan, simulation->spatial_lookup[i], 0, VK_SHADER_STAGE_COMPUTE_BIT);
		vulkan_pipeline_desc_add_storage_buffer(&radixsort_histograms_description, vulkan, simulation->histograms[i], 0, VK_SHADER_STAGE_COMPUTE_BIT);

		vulkan_pipeline_desc_set_shaders(&radixsort_histograms_description, NULL, NULL, "src/shaders/spv/radixsort_histograms.comp.spv");
		vulkan_pipeline_desc_set_push_constant(&radixsort_histograms_description, sizeof(radixsort_pc), VK_SHADER_STAGE_COMPUTE_BIT);

		simulation->radixsort_histogram_pipelines[i] = vulkan_pipeline_create(vulkan, &radixsort_histograms_description);
		assert(simulation->radixsort_histogram_pipelines[i] != INVALID_PIPELINE);

		vulkan_pipeline_desc radixsort_description = vulkan_pipeline_default(VULKAN_PIPELINE_TYPE_COMPUTE);

		vulkan_pipeline_desc_add_storage_buffer(&radixsort_description, vulkan, simulation->spatial_lookup[read_buffer], 0, VK_SHADER_STAGE_COMPUTE_BIT);
		vulkan_pipeline_desc_add_storage_buffer(&radixsort_description, vulkan, simulation->spatial_lookup[write_buffer], 0, VK_SHADER_STAGE_COMPUTE_BIT);
		vulkan_pipeline_desc_add_storage_buffer(&radixsort_description, vulkan, simulation->histograms[i], 0, VK_SHADER_STAGE_COMPUTE_BIT);

		vulkan_pipeline_desc_set_shaders(&radixsort_description, NULL, NULL, "src/shaders/spv/radixsort.comp.spv");
		vulkan_pipeline_desc_set_push_constant(&radixsort_description, sizeof(radixsort_pc), VK_SHADER_STAGE_COMPUTE_BIT);

		simulation->radixsort_pipelines[i] = vulkan_pipeline_create(vulkan, &radixsort_description);
		assert(simulation->radixsort_pipelines[i] != INVALID_PIPELINE);

		vulkan_pipeline_desc start_indices_description = vulkan_pipeline_default(VULKAN_PIPELINE_TYPE_COMPUTE);

		vulkan_pipeline_desc_add_storage_buffer(&start_indices_description, vulkan, simulation->spatial_lookup[read_buffer], 0, VK_SHADER_STAGE_COMPUTE_BIT);
		vulkan_pipeline_desc_add_storage_buffer(&start_indices_description, vulkan, simulation->start_indices[read_buffer], 0, VK_SHADER_STAGE_COMPUTE_BIT);

		vulkan_pipeline_desc_set_shaders(&start_indices_description, NULL, NULL, "src/shaders/spv/start_indices.comp.spv");
		vulkan_pipeline_desc_set_push_constant(&start_indices_description, sizeof(start_indices_pc), VK_SHADER_STAGE_COMPUTE_BIT);
		
		simulation->start_indices_pipelines[i] = vulkan_pipeline_create(vulkan, &start_indices_description);
		assert(simulation->start_indices_pipelines[i] != INVALID_PIPELINE);

		vulkan_pipeline_desc density_description = vulkan_pipeline_default(VULKAN_PIPELINE_TYPE_COMPUTE);

		vulkan_pipeline_desc_add_storage_buffer(&density_description, vulkan, simulation->particles[read_buffer], 0, VK_SHADER_STAGE_COMPUTE_BIT);
		vulkan_pipeline_desc_add_storage_buffer(&density_description, vulkan, simulation->particles[read_buffer], 0, VK_SHADER_STAGE_COMPUTE_BIT);

		vulkan_pipeline_desc_set_shaders(&density_description, NULL, NULL, "src/shaders/spv/density.comp.spv");
		vulkan_pipeline_desc_set_push_constant(&density_description, sizeof(density_pc), VK_SHADER_STAGE_COMPUTE_BIT);
		
		simulation->density_pipelines[i] = vulkan_pipeline_create(vulkan, &density_description);
		assert(simulation->density_pipelines[i] != INVALID_PIPELINE);

		
		vulkan_pipeline_desc update_description = vulkan_pipeline_default(VULKAN_PIPELINE_TYPE_COMPUTE);

		vulkan_pipeline_desc_add_storage_buffer(&update_description, vulkan, simulation->particles[read_buffer], 0, VK_SHADER_STAGE_COMPUTE_BIT);
		vulkan_pipeline_desc_add_storage_buffer(&update_description, vulkan, simulation->particles[write_buffer], 0, VK_SHADER_STAGE_COMPUTE_BIT);
		vulkan_pipeline_desc_add_storage_buffer(&update_description, vulkan, simulation->spatial_lookup[read_buffer], 0, VK_SHADER_STAGE_COMPUTE_BIT);
		vulkan_pipeline_desc_add_storage_buffer(&update_description, vulkan, simulation->start_indices[read_buffer], 0, VK_SHADER_STAGE_COMPUTE_BIT);
		
		vulkan_pipeline_desc_set_shaders(&update_description, NULL, NULL, "src/shaders/spv/update.comp.spv");
		vulkan_pipeline_desc_set_push_constant(&update_description, sizeof(update_pc), VK_SHADER_STAGE_COMPUTE_BIT);

		simulation->update_pipelines[i] = vulkan_pipeline_create(vulkan, &update_description);
		assert(simulation->update_pipelines[i] != INVALID_PIPELINE);	
	}

	simulation_measure_rest_density(vulkan, simulation);
	// simulation_check_sorted(vulkan, simulation);

	return true;
}

void simulation_update(vulkan_context *vulkan, u32 window_width, u32 window_height, time time, simulation *simulation)
{
	assert(vulkan);
	assert(simulation);

	simulation->accumulator += time.delta;
	simulation->accumulator = MIN(simulation->accumulator, FIXED_DT * MAX_STEPS_PER_FRAME);

	while (simulation->accumulator >= FIXED_DT)
	{
		const u32 sim = simulation->sim_buffer;
		const u32 group_size = 256;
		const u32 group_count = (PARTICLE_COUNT + group_size - 1) / group_size;

		// NOTE: Spatial lookup
		vulkan_command_bind_pipeline(vulkan, simulation->spatial_lookup_pipelines[sim]);

		spatial_lookup_pc spatial_lookup_pc = {
			.particle_count = PARTICLE_COUNT,	
		};

		vulkan_command_push_constants(vulkan, sizeof(spatial_lookup_pc), &spatial_lookup_pc, VK_SHADER_STAGE_COMPUTE_BIT, simulation->spatial_lookup_pipelines[sim]);
		vulkan_command_dispatch(vulkan, group_count, 1, 1);

		vulkan_command_barrier(vulkan, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT);	

		// NOTE: Sorting spatial lookup
		for (u32 i = 0; i < 4; i++)
		{
			u32 pass_buf = (sim + i) % FRAMES_IN_FLIGHT;

			vulkan_command_bind_pipeline(vulkan, simulation->radixsort_histogram_pipelines[pass_buf]);

			radixsort_pc radixsort_pc = {
				.element_count = PARTICLE_COUNT,
				.workgroup_count = group_count,
				.blocks_per_workgroup = 32,
				.shift = 8 * i,
			};

			vulkan_command_push_constants(vulkan, sizeof(radixsort_pc), &radixsort_pc, VK_SHADER_STAGE_COMPUTE_BIT, simulation->radixsort_histogram_pipelines[pass_buf]);
			vulkan_command_dispatch(vulkan, group_count, 1, 1);

			vulkan_command_barrier(vulkan, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT);

			vulkan_command_bind_pipeline(vulkan, simulation->radixsort_pipelines[pass_buf]);

			vulkan_command_push_constants(vulkan, sizeof(radixsort_pc), &radixsort_pc, VK_SHADER_STAGE_COMPUTE_BIT, simulation->radixsort_pipelines[pass_buf]);
			vulkan_command_dispatch(vulkan, group_count, 1, 1);
		}
		vulkan_command_barrier(vulkan, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT);

		// NOTE: Updating start indices
		vulkan_command_bind_pipeline(vulkan, simulation->start_indices_pipelines[sim]);

		start_indices_pc start_indices_pc = {
			.particle_count = PARTICLE_COUNT,	
		};

		vulkan_command_push_constants(vulkan, sizeof(start_indices_pc), &start_indices_pc, VK_SHADER_STAGE_COMPUTE_BIT, simulation->start_indices_pipelines[sim]);
		vulkan_command_dispatch(vulkan, group_count, 1, 1);

		// NOTE: Calculating densities
		vulkan_command_bind_pipeline(vulkan, simulation->density_pipelines[sim]);

		density_pc density_pc = {
			.particle_count = PARTICLE_COUNT,	
		};

		vulkan_command_push_constants(vulkan, sizeof(density_pc), &density_pc, VK_SHADER_STAGE_COMPUTE_BIT, simulation->density_pipelines[sim]);
		vulkan_command_dispatch(vulkan, group_count, 1, 1);

		vulkan_command_barrier(vulkan, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT);	

		// NOTE: Update particles
		vulkan_command_bind_pipeline(vulkan, simulation->update_pipelines[sim]);

		update_pc update_pc = {
			.dt = FIXED_DT,
			.window_width = window_width,
			.window_height = window_height,
			.particle_count = PARTICLE_COUNT,	
		};
		vulkan_command_push_constants(vulkan, sizeof(update_pc), &update_pc, VK_SHADER_STAGE_COMPUTE_BIT, simulation->update_pipelines[sim]);
		vulkan_command_dispatch(vulkan, group_count, 1, 1);

		vulkan_command_barrier(vulkan, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT);

		simulation->sim_buffer = (simulation->sim_buffer + 1) % FRAMES_IN_FLIGHT;
		simulation->accumulator -= FIXED_DT;
	}
	vulkan_command_barrier(vulkan, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT);

	// NOTE: Render
	vulkan_command_begin_rendering(vulkan);

	m4 orthographic = m4orthographic(0, window_width, 0, window_height, -1.0f, 1.0f);
	
	vulkan_command_bind_pipeline(vulkan, simulation->render_pipeline);
	vulkan_command_push_constants(vulkan, sizeof(orthographic), &orthographic, VK_SHADER_STAGE_VERTEX_BIT, simulation->render_pipeline);

	vulkan_command_bind_vertex_buffer(vulkan, simulation->particles[simulation->sim_buffer], simulation->render_pipeline);

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
		vulkan_buffer_destroy(vulkan, &simulation->spatial_lookup[i]);
		vulkan_buffer_destroy(vulkan, &simulation->start_indices[i]);
		vulkan_buffer_destroy(vulkan, &simulation->histograms[i]);
	}
}
