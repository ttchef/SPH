
#include <sph/simulation.h>
#include <sph/window.h>
#include <sph/render.h>
#include <math/core.h>
#include <vk/context.h>

#include <SDL3/SDL_log.h>
#include <vulkan/vulkan_core.h>

#define PARTICLE_DISTANCE 5.5f
#define FIXED_DT (1.0f / 240.0f)
#define MAX_STEPS_PER_FRAME 2
#define RADIX_SORT_PASSES 4

// TODO: Make particle count as a specilazation constant in the shaders

//
// NOTE: Push constants
//

typedef struct radixsort_pc
{
	u32 element_count;
	u32 shift;
	u32 workgroup_count;
	u32 blocks_per_workgroup;	
} radixsort_pc;

typedef struct update_pc
{
	v4 cube_min;
	v4 cube_max;
	f32 dt;
	f32 padding[3];
} update_pc;

typedef struct render_pc
{
	m4 view;
	m4 perspective;
} render_pc;

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

	vkCmdDispatch(cmd, group_count, 1, 1);
	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &compute_barrier, 0, NULL, 0, NULL);

	// NOTE: Sorting spatial lookup
	for (u32 i = 0; i < RADIX_SORT_PASSES; i++)
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

	BIND_COMPUTE(simulation->start_indices_pipelines[sim]);
	vkCmdDispatch(cmd, group_count, 1, 1);
	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &compute_barrier, 0, NULL, 0, NULL);
		
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->handle);
	for (u32 i = 0; i < pipeline->descriptor_count; i++)
	{
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->layout, i, 1, &pipeline->descriptors[i].set, 0, NULL);
	}

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

	vkCmdDispatch(cmd, group_count, 1, 1);
	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &compute_barrier, 0, NULL, 0, NULL);

	// NOTE: Sorting spatial lookup
	for (u32 i = 0; i < RADIX_SORT_PASSES; i++)
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

	BIND_COMPUTE(simulation->start_indices_pipelines[sim]);
	vkCmdDispatch(cmd, group_count, 1, 1);
	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &compute_barrier, 0, NULL, 0, NULL);

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

bool simulation_create(vulkan_context *vulkan, simulation *simulation)
{
	assert(vulkan);
	assert(simulation);

	simulation->accumulator = 0.0;
	simulation->sim_buffer = 0;

	particle *particles = SDL_calloc(PARTICLE_COUNT, sizeof(particle));
	assert(particles);

	u32 grid_size = (u32)SDL_roundf(SDL_powf((f32)PARTICLE_COUNT, 1.0f / 3.0f));
	f32 half_grid_width = (grid_size - 1) * PARTICLE_DISTANCE * 0.5f;

	for (u32 i = 0; i < PARTICLE_COUNT; i++)
	{
		particle *p = &particles[i];
		assert(p);

		u32 x_index = i % grid_size;
		u32 y_index = (i / grid_size) % grid_size;
		u32 z_index = i / (grid_size * grid_size);

		f32 x = x_index * PARTICLE_DISTANCE - half_grid_width;
		f32 y = y_index * PARTICLE_DISTANCE - half_grid_width;
		f32 z = z_index * PARTICLE_DISTANCE - half_grid_width;

		*p = (particle){
			.mass = 1.0f,
			.pos = v4make(x, y, z, 1.0f),
		};
	}

	const u32 group_size = 256;
	const u32 group_count = (PARTICLE_COUNT + group_size - 1) / group_size;
	for (u32 i = 0; i < FRAMES_IN_FLIGHT; i++)
	{
		VkBufferUsageFlags usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		vulkan_buffer_device_local_create(vulkan, usage, PARTICLE_COUNT * sizeof(particle), particles, &simulation->particles[i]);

		usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		vulkan_buffer_device_local_create(vulkan, usage, PARTICLE_COUNT * sizeof(spatial_lookup_entry), NULL, &simulation->spatial_lookup[i]);
		vulkan_buffer_device_local_create(vulkan, usage, PARTICLE_COUNT * sizeof(u32), NULL, &simulation->start_indices[i]);
		vulkan_buffer_device_local_create(vulkan, usage, 256 * group_count * sizeof(u32), NULL, &simulation->histograms[i]);
	}

	for (u32 i = 0; i < FRAMES_IN_FLIGHT; i++)
	{
		u32 read_buffer = i;
		u32 write_buffer = (i + 1) % FRAMES_IN_FLIGHT;
		u32 sorted_buffer = (i + RADIX_SORT_PASSES) % FRAMES_IN_FLIGHT;

		vulkan_pipeline_desc spatial_lookup_description = vulkan_pipeline_default(VULKAN_PIPELINE_TYPE_COMPUTE);

		vulkan_pipeline_desc_add_storage_buffer(&spatial_lookup_description, vulkan, simulation->particles[read_buffer], 0, VK_SHADER_STAGE_COMPUTE_BIT);
		vulkan_pipeline_desc_add_storage_buffer(&spatial_lookup_description, vulkan, simulation->spatial_lookup[read_buffer], 0, VK_SHADER_STAGE_COMPUTE_BIT);
		vulkan_pipeline_desc_add_storage_buffer(&spatial_lookup_description, vulkan, simulation->start_indices[read_buffer], 0, VK_SHADER_STAGE_COMPUTE_BIT);

		vulkan_pipeline_desc_set_shaders(&spatial_lookup_description, NULL, NULL, "src/shaders/spv/spatial_lookup.comp.spv");
		vulkan_pipeline_desc_set_specialization_constant(&spatial_lookup_description, sizeof(u32), (void *)&PARTICLE_COUNT, VK_SHADER_STAGE_COMPUTE_BIT);

		simulation->spatial_lookup_pipelines[i] = vulkan_pipeline_create(vulkan, &spatial_lookup_description);
		assert(simulation->spatial_lookup_pipelines[i] != INVALID_PIPELINE);

		vulkan_pipeline_desc radixsort_histograms_description = vulkan_pipeline_default(VULKAN_PIPELINE_TYPE_COMPUTE);

		vulkan_pipeline_desc_add_storage_buffer(&radixsort_histograms_description, vulkan, simulation->spatial_lookup[read_buffer], 0, VK_SHADER_STAGE_COMPUTE_BIT);
		vulkan_pipeline_desc_add_storage_buffer(&radixsort_histograms_description, vulkan, simulation->histograms[read_buffer], 0, VK_SHADER_STAGE_COMPUTE_BIT);

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

		vulkan_pipeline_desc_add_storage_buffer(&start_indices_description, vulkan, simulation->spatial_lookup[sorted_buffer], 0, VK_SHADER_STAGE_COMPUTE_BIT);
		vulkan_pipeline_desc_add_storage_buffer(&start_indices_description, vulkan, simulation->start_indices[sorted_buffer], 0, VK_SHADER_STAGE_COMPUTE_BIT);

		vulkan_pipeline_desc_set_shaders(&start_indices_description, NULL, NULL, "src/shaders/spv/start_indices.comp.spv");
		vulkan_pipeline_desc_set_specialization_constant(&start_indices_description, sizeof(u32), (void *)&PARTICLE_COUNT, VK_SHADER_STAGE_COMPUTE_BIT);
		
		simulation->start_indices_pipelines[i] = vulkan_pipeline_create(vulkan, &start_indices_description);
		assert(simulation->start_indices_pipelines[i] != INVALID_PIPELINE);

		vulkan_pipeline_desc density_description = vulkan_pipeline_default(VULKAN_PIPELINE_TYPE_COMPUTE);

		vulkan_pipeline_desc_add_storage_buffer(&density_description, vulkan, simulation->particles[read_buffer], 0, VK_SHADER_STAGE_COMPUTE_BIT);
		vulkan_pipeline_desc_add_storage_buffer(&density_description, vulkan, simulation->particles[read_buffer], 0, VK_SHADER_STAGE_COMPUTE_BIT);
		vulkan_pipeline_desc_add_storage_buffer(&density_description, vulkan, simulation->spatial_lookup[sorted_buffer], 0, VK_SHADER_STAGE_COMPUTE_BIT);
		vulkan_pipeline_desc_add_storage_buffer(&density_description, vulkan, simulation->start_indices[sorted_buffer], 0, VK_SHADER_STAGE_COMPUTE_BIT);

		vulkan_pipeline_desc_set_shaders(&density_description, NULL, NULL, "src/shaders/spv/density.comp.spv");
		vulkan_pipeline_desc_set_specialization_constant(&density_description, sizeof(u32), (void *)&PARTICLE_COUNT, VK_SHADER_STAGE_COMPUTE_BIT);
		
		simulation->density_pipelines[i] = vulkan_pipeline_create(vulkan, &density_description);
		assert(simulation->density_pipelines[i] != INVALID_PIPELINE);

		
		vulkan_pipeline_desc update_description = vulkan_pipeline_default(VULKAN_PIPELINE_TYPE_COMPUTE);
		vulkan_pipeline_desc_set_specialization_constant(&update_description, sizeof(u32), (void *)&PARTICLE_COUNT, VK_SHADER_STAGE_COMPUTE_BIT);

		vulkan_pipeline_desc_add_storage_buffer(&update_description, vulkan, simulation->particles[read_buffer], 0, VK_SHADER_STAGE_COMPUTE_BIT);
		vulkan_pipeline_desc_add_storage_buffer(&update_description, vulkan, simulation->particles[write_buffer], 0, VK_SHADER_STAGE_COMPUTE_BIT);
		vulkan_pipeline_desc_add_storage_buffer(&update_description, vulkan, simulation->spatial_lookup[sorted_buffer], 0, VK_SHADER_STAGE_COMPUTE_BIT);
		vulkan_pipeline_desc_add_storage_buffer(&update_description, vulkan, simulation->start_indices[sorted_buffer], 0, VK_SHADER_STAGE_COMPUTE_BIT);
		
		vulkan_pipeline_desc_set_shaders(&update_description, NULL, NULL, "src/shaders/spv/update.comp.spv");
		vulkan_pipeline_desc_set_push_constant(&update_description, sizeof(update_pc), VK_SHADER_STAGE_COMPUTE_BIT);

		simulation->update_pipelines[i] = vulkan_pipeline_create(vulkan, &update_description);
		assert(simulation->update_pipelines[i] != INVALID_PIPELINE);

		vulkan_pipeline_desc render_description = vulkan_pipeline_default(VULKAN_PIPELINE_TYPE_GRAPHICS);

		vulkan_pipeline_desc_set_push_constant(&render_description, sizeof(render_pc), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
		vulkan_pipeline_desc_set_shaders(&render_description, "src/shaders/spv/particle.vert.spv", "src/shaders/spv/particle.frag.spv", NULL);

		vulkan_pipeline_desc_add_storage_buffer(&render_description, vulkan, simulation->particles[write_buffer], 0, VK_SHADER_STAGE_VERTEX_BIT);

		simulation->render_pipeline[i] = vulkan_pipeline_create(vulkan, &render_description);
		assert(simulation->render_pipeline[i] != INVALID_PIPELINE);
	}

	simulation_measure_rest_density(vulkan, simulation);
	// simulation_check_sorted(vulkan, simulation);

	simulation->boundary_cube = cubemake(v3zero(), v3make(500, 900, 500));

	return true;
}

void simulation_update(vulkan_context *vulkan, u32 window_width, u32 window_height, time time, camera camera, render *render, ui *ui, simulation *simulation)
{
	assert(vulkan);
	assert(simulation);

	simulation->accumulator += time.delta;
	simulation->accumulator = MIN(simulation->accumulator, FIXED_DT * MAX_STEPS_PER_FRAME);

	u32 sim = simulation->sim_buffer;
	// /*
	while (simulation->accumulator >= FIXED_DT)
	{
		sim = simulation->sim_buffer;
		const u32 group_size = 256;
		const u32 group_count = (PARTICLE_COUNT + group_size - 1) / group_size;

		// NOTE: Spatial lookup
		vulkan_command_bind_pipeline(vulkan, simulation->spatial_lookup_pipelines[sim]);
		vulkan_command_dispatch(vulkan, group_count, 1, 1);

		vulkan_command_barrier(vulkan, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT);	

		// NOTE: Sorting spatial lookup
		for (u32 i = 0; i < 4; i++)
		{
			u32 pass_buf = (sim + i) % FRAMES_IN_FLIGHT;

			vulkan_command_bind_pipeline(vulkan, simulation->radixsort_histogram_pipelines[pass_buf]);

			const u32 sort_groups = 32;
			const u32 blocks_per_workgroup = (PARTICLE_COUNT + sort_groups * group_size - 1) / (sort_groups * group_size);
			radixsort_pc radixsort_pc = {
				.element_count = PARTICLE_COUNT,
				.workgroup_count = group_count,
				.blocks_per_workgroup = blocks_per_workgroup,
				.shift = 8 * i,
			};

			vulkan_command_push_constants(vulkan, sizeof(radixsort_pc), &radixsort_pc, VK_SHADER_STAGE_COMPUTE_BIT, simulation->radixsort_histogram_pipelines[pass_buf]);
			vulkan_command_dispatch(vulkan, group_count, 1, 1);

			vulkan_command_barrier(vulkan, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT);

			vulkan_command_bind_pipeline(vulkan, simulation->radixsort_pipelines[pass_buf]);

			vulkan_command_push_constants(vulkan, sizeof(radixsort_pc), &radixsort_pc, VK_SHADER_STAGE_COMPUTE_BIT, simulation->radixsort_pipelines[pass_buf]);
			vulkan_command_dispatch(vulkan, group_count, 1, 1);

			vulkan_command_barrier(vulkan, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT);
		}

		// NOTE: Updating start indices
		vulkan_command_bind_pipeline(vulkan, simulation->start_indices_pipelines[sim]);
		vulkan_command_dispatch(vulkan, group_count, 1, 1);

		vulkan_command_barrier(vulkan, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT);

		// NOTE: Calculating densities
		vulkan_command_bind_pipeline(vulkan, simulation->density_pipelines[sim]);
		vulkan_command_dispatch(vulkan, group_count, 1, 1);
		
		vulkan_command_barrier(vulkan, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT);	

		// NOTE: Update particles
		vulkan_command_bind_pipeline(vulkan, simulation->update_pipelines[sim]);

		v3 cube_min = v3sub(simulation->boundary_cube.pos, v3scale(simulation->boundary_cube.size, 0.5f));
		v3 cube_max = v3add(simulation->boundary_cube.pos, v3scale(simulation->boundary_cube.size, 0.5f));

		update_pc update_pc = {
			.dt = FIXED_DT,
			.cube_min = v4fromv3(cube_min, 1.0),
			.cube_max = v4fromv3(cube_max, 1.0),
		};
		vulkan_command_push_constants(vulkan, sizeof(update_pc), &update_pc, VK_SHADER_STAGE_COMPUTE_BIT, simulation->update_pipelines[sim]);
		vulkan_command_dispatch(vulkan, group_count, 1, 1);

		vulkan_command_barrier(vulkan, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT);

		simulation->sim_buffer = (simulation->sim_buffer + 1) % FRAMES_IN_FLIGHT;
		simulation->accumulator -= FIXED_DT;
	}
	// */
	vulkan_command_barrier(vulkan, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT);

	// NOTE: Render
	vulkan_command_begin_rendering(vulkan);

	const u32 viewport_width = window_width - ui->settings_width;
	const u32 viewport_height = window_height;
	vulkan_command_set_viewport(vulkan, 0, 0, viewport_width, viewport_height);

	m4 view = camera_view(&camera);

	f32 aspect_ratio = (f32)viewport_width / (f32)viewport_height;
	m4 perspective = m4perspective(TO_RADIANS(60.0f), aspect_ratio, 0.1f, 3000.0f);

	render_pc render_pc = {
		.view = view,
		.perspective = perspective,	
	};
	
	vulkan_command_bind_pipeline(vulkan, simulation->render_pipeline[sim]);
	vulkan_command_push_constants(vulkan, sizeof(render_pc), &render_pc, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, simulation->render_pipeline[sim]);
	
	vulkan_command_draw(vulkan, PARTICLE_COUNT * 6);

	m4 view_proj = m4mul(perspective, view);
	render_cube_lines(vulkan, render, simulation->boundary_cube.pos, simulation->boundary_cube.size, RED, view_proj);

	ui_draw(vulkan, render, ui);

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
