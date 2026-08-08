
#include <sph/app.h>
#include <sph/simulation.h>
#include <vk/context.h>

#define PARTICLE_X     50
#define PARTICLE_Y     50
#define PARTICLE_Z     50
#define PARTICLE_COUNT PARTICLE_X * PARTICLE_Y * PARTICLE_Z

static void compute_densities(app *app, vulkan_command_queue *queue, simulation *simulation, u32 particle_count, u32 sorted)
{
    vulkan *vulkan = &app->vulkan;

    vulkan_command_label_begin(queue, "compute densities", CYAN);
    vulkan_command_bind_pipeline(queue, app->pipelines[PIPELINE_PARTICLE_DENSITY]);
    vulkan_command_bind_scene_ubo(queue, simulation->scene, app->pipelines[PIPELINE_PARTICLE_DENSITY]);

    particle_density_pc density_pc = {
        .densities_addr      = vulkan_buffer_address_get(vulkan, simulation->densities),
        .positions_read_addr = vulkan_buffer_address_get(vulkan, simulation->sorted_positions),
        .spatial_lookup_addr = vulkan_buffer_address_get(vulkan, simulation->spatial_lookup[sorted]),
        .start_indices_addr  = vulkan_buffer_address_get(vulkan, simulation->start_indices),
    };
    vulkan_command_push_constants(queue, sizeof(density_pc), &density_pc, VK_SHADER_STAGE_COMPUTE_BIT, app->pipelines[PIPELINE_PARTICLE_DENSITY]);
    vulkan_command_dispatch(queue, (particle_count + 255) / 256, 1, 1);

    vulkan_command_label_end(queue);
}

static u32 spatial_lookup_sort(app *app, vulkan_command_queue *queue, simulation *simulation)
{
    vulkan *vulkan = &app->vulkan;

    const u32 workgroup_size       = 256;
    const u32 workgroup_count      = (PARTICLE_COUNT + workgroup_size - 1) / workgroup_size;
    const u32 sort_groups          = 32;
    const u32 blocks_per_workgroup = (PARTICLE_COUNT + sort_groups * workgroup_size - 1) / (sort_groups * workgroup_size);

    vulkan_command_label_begin(queue, "spatial lookup sort", MAGENTA);
    vulkan_command_bind_pipeline(queue, app->pipelines[PIPELINE_SPATIAL_LOOKUP_WRITE]);
    vulkan_command_bind_scene_ubo(queue, simulation->scene, app->pipelines[PIPELINE_SPATIAL_LOOKUP_WRITE]);

    spatial_lookup_write_pc write_pc = {
        .spatial_lookup_addr = vulkan_buffer_address_get(vulkan, simulation->spatial_lookup[simulation->read_index]),
        .start_indices_addr  = vulkan_buffer_address_get(vulkan, simulation->start_indices),
        .positions_addr      = vulkan_buffer_address_get(vulkan, simulation->positions[simulation->read_index]),
    };

    vulkan_command_push_constants(queue, sizeof(write_pc), &write_pc, VK_SHADER_STAGE_COMPUTE_BIT, app->pipelines[PIPELINE_SPATIAL_LOOKUP_WRITE]);
    vulkan_command_dispatch(queue, workgroup_count, 1, 1);
    vulkan_command_barrier(queue, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);

    u32 src = simulation->read_index;
    u32 dst = simulation->write_index;
    for (u32 i = 0; i < 4; i++)
    {
        vulkan_command_bind_pipeline(queue, app->pipelines[PIPELINE_SPATIAL_LOOKUP_HISTOGRAMS]);
        vulkan_command_bind_scene_ubo(queue, simulation->scene, app->pipelines[PIPELINE_SPATIAL_LOOKUP_HISTOGRAMS]);

        spatial_lookup_histograms_pc histograms_pc = {
            .spatial_lookup_addr            = vulkan_buffer_address_get(vulkan, simulation->spatial_lookup[src]),
            .spatial_lookup_histograms_addr = vulkan_buffer_address_get(vulkan, simulation->spatial_lookup_histograms),
            .shift                          = i * 8,
            .workgroup_count                = workgroup_count,
            .blocks_per_workgroup           = blocks_per_workgroup,
        };

        vulkan_command_push_constants(queue, sizeof(histograms_pc), &histograms_pc, VK_SHADER_STAGE_COMPUTE_BIT, app->pipelines[PIPELINE_SPATIAL_LOOKUP_HISTOGRAMS]);
        vulkan_command_dispatch(queue, workgroup_count, 1, 1);
        vulkan_command_barrier(queue, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);

        vulkan_command_bind_pipeline(queue, app->pipelines[PIPELINE_SPATIAL_LOOKUP_SORT]);
        vulkan_command_bind_scene_ubo(queue, simulation->scene, app->pipelines[PIPELINE_SPATIAL_LOOKUP_SORT]);

        spatial_lookup_sort_pc sort_pc = {
            .spatial_lookup_read_addr       = vulkan_buffer_address_get(vulkan, simulation->spatial_lookup[src]),
            .spatial_lookup_write_addr      = vulkan_buffer_address_get(vulkan, simulation->spatial_lookup[dst]),
            .spatial_lookup_histograms_addr = vulkan_buffer_address_get(vulkan, simulation->spatial_lookup_histograms),
            .shift                          = i * 8,
            .workgroup_count                = workgroup_count,
            .blocks_per_workgroup           = blocks_per_workgroup,
        };

        vulkan_command_push_constants(queue, sizeof(sort_pc), &sort_pc, VK_SHADER_STAGE_COMPUTE_BIT, app->pipelines[PIPELINE_SPATIAL_LOOKUP_SORT]);
        vulkan_command_dispatch(queue, workgroup_count, 1, 1);
        vulkan_command_barrier(queue, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);

        // NOTE: Xor swap
        src ^= dst;
        dst ^= src;
        src ^= dst;
    }

    vulkan_command_bind_pipeline(queue, app->pipelines[PIPELINE_START_INDICES]);
    vulkan_command_bind_scene_ubo(queue, simulation->scene, app->pipelines[PIPELINE_START_INDICES]);

    start_indices_pc start_pc = {
        .spatial_lookup_addr = vulkan_buffer_address_get(vulkan, simulation->spatial_lookup[src]),
        .start_indices_addr  = vulkan_buffer_address_get(vulkan, simulation->start_indices),
    };

    vulkan_command_push_constants(queue, sizeof(start_pc), &start_pc, VK_SHADER_STAGE_COMPUTE_BIT, app->pipelines[PIPELINE_START_INDICES]);
    vulkan_command_dispatch(queue, workgroup_count, 1, 1);
    vulkan_command_label_end(queue);

    vulkan_command_barrier(queue, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);

    vulkan_command_label_begin(queue, "particle reorder", CYAN);

    vulkan_command_bind_pipeline(queue, app->pipelines[PIPELINE_PARTICLE_REORDER]);
    vulkan_command_bind_scene_ubo(queue, simulation->scene, app->pipelines[PIPELINE_PARTICLE_REORDER]);

    particle_reorder_pc reorder_pc = {
        .positions_read_addr    = vulkan_buffer_address_get(vulkan, simulation->positions[simulation->read_index]),
        .velocities_read_addr   = vulkan_buffer_address_get(vulkan, simulation->velocities[simulation->read_index]),
        .spatial_lookup_addr    = vulkan_buffer_address_get(vulkan, simulation->spatial_lookup[src]),
        .sorted_positions_addr  = vulkan_buffer_address_get(vulkan, simulation->sorted_positions),
        .sorted_velocities_addr = vulkan_buffer_address_get(vulkan, simulation->sorted_velocities),
    };
    vulkan_command_push_constants(queue, sizeof(reorder_pc), &reorder_pc, VK_SHADER_STAGE_COMPUTE_BIT, app->pipelines[PIPELINE_PARTICLE_REORDER]);
    vulkan_command_dispatch(queue, workgroup_count, 1, 1);
    vulkan_command_label_end(queue);

    return src;
}

bool simulation_create(app *app, simulation *out_simulation)
{
    vulkan *vulkan = &app->vulkan;

    simulation result = {0};

    result.read_index  = 0;
    result.write_index = 1;
    result.first_loop  = true;

    v4 *particle_positions = memory_arena_alloc(&app->frame_arena, PARTICLE_X * PARTICLE_Y * PARTICLE_Z * sizeof(v4));

    u32 particle_index = 0;
    for (u32 z = 0; z < PARTICLE_Z; z++)
    {
        for (u32 y = 0; y < PARTICLE_Y; y++)
        {
            for (u32 x = 0; x < PARTICLE_X; x++)
            {
                v4 pos;
                pos.x = (x * 5) - (PARTICLE_X / 2.0f * 5);
                pos.y = (y * 5) - (PARTICLE_Y / 2.0f * 5);
                pos.z = (z * 5) - (PARTICLE_Z / 2.0f * 5);
                pos.w = 1.0f;

                particle_positions[particle_index++] = pos;
            }
        }
    }
    const u32 workgroup_size  = 256;
    const u32 workgroup_count = (PARTICLE_COUNT + workgroup_size - 1) / workgroup_size;

    VkBufferUsageFlagBits usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

    vulkan_buffer_device_local_create(vulkan, &app->frame_arena, usage, sizeof(v4) * PARTICLE_COUNT, particle_positions, "particle_positions_zero", &result.positions[0]);
    vulkan_buffer_device_local_create(vulkan, &app->frame_arena, usage, sizeof(v4) * PARTICLE_COUNT, NULL, "particle_positions_one", &result.positions[1]);
    vulkan_buffer_device_local_create(vulkan, &app->frame_arena, usage, sizeof(v4) * PARTICLE_COUNT, NULL, "particle_velocities_zero", &result.velocities[0]);
    vulkan_buffer_device_local_create(vulkan, &app->frame_arena, usage, sizeof(v4) * PARTICLE_COUNT, NULL, "particle_velocities_one", &result.velocities[1]);
    vulkan_buffer_device_local_create(vulkan, &app->frame_arena, usage, sizeof(f32) * PARTICLE_COUNT, NULL, "particle_densities", &result.densities);
    vulkan_buffer_device_local_create(vulkan, &app->frame_arena, usage, sizeof(simulation_spatial_lookup_entry) * PARTICLE_COUNT, NULL, "particle_spatial_lookup_zero", &result.spatial_lookup[0]);
    vulkan_buffer_device_local_create(vulkan, &app->frame_arena, usage, sizeof(simulation_spatial_lookup_entry) * PARTICLE_COUNT, NULL, "particle_spatial_lookup_one", &result.spatial_lookup[1]);
    vulkan_buffer_device_local_create(vulkan, &app->frame_arena, usage, sizeof(u32) * workgroup_count * workgroup_size, NULL, "particle_spatial_lookup_histograms", &result.spatial_lookup_histograms);
    vulkan_buffer_device_local_create(vulkan, &app->frame_arena, usage, sizeof(u32) * PARTICLE_COUNT, NULL, "particle_start_indices", &result.start_indices);
    vulkan_buffer_device_local_create(vulkan, &app->frame_arena, usage, sizeof(v4) * PARTICLE_COUNT, NULL, "particle_sorted_positions", &result.sorted_positions);
    vulkan_buffer_device_local_create(vulkan, &app->frame_arena, usage, sizeof(v4) * PARTICLE_COUNT, NULL, "particle_sorted_velocities", &result.sorted_velocities);

    result.scene    = vulkan_bindless_scene_ubo_aquire(vulkan);
    result.ubo_data = (simulation_ubo){
        .particle_count      = PARTICLE_COUNT,
        .target_density      = 0.0065,
        .pressure_multiplier = 2.5f,
        .smoothing_radius    = 9.0f,
        .viscosity_coeff     = 15.0f,
    };
    SDL_memcpy(vulkan_bindless_scene_ubo_get(vulkan, result.scene), &result.ubo_data, sizeof(simulation_ubo));

    /*
    vulkan_command_queue *spatial_lookup_queue = vulkan_command_begin(&app->frame_arena);

    u32 sorted = spatial_lookup_sort(app, spatial_lookup_queue, &result);
    if (!vulkan_command_end(spatial_lookup_queue, vulkan, true))
    {
        return false;
    }

    vulkan_buffer spatial_lookup;
    if (!vulkan_buffer_device_local_get_data(vulkan, &app->frame_arena, result.spatial_lookup[sorted], "spatal_lookup_sorted", &spatial_lookup))
    {
        return false;
    }

    simulation_spatial_lookup_entry *entries = spatial_lookup.host_visible.data;
    for (u32 i = 0; i < PARTICLE_COUNT; i++)
    {
        simulation_spatial_lookup_entry *entry = &entries[i];

        SDL_Log("Entry %u:\n\tIndex: %u\n\tKey: %u", i, entry->particle_index, entry->cell_key);
    }

    vulkan_object_destroy(vulkan, sizeof(spatial_lookup), &spatial_lookup, (vulkan_destroy_func)vulkan_buffer_destroy);

    vulkan_buffer start_indices;
    if (!vulkan_buffer_device_local_get_data(vulkan, &app->frame_arena, result.start_indices, "start_indices_test", &start_indices))
    {
        return false;
    }

    u32 *start_indices_data = start_indices.host_visible.data;
    for (u32 i = 0; i < PARTICLE_COUNT; i++)
    {
        if (start_indices_data[i] == UINT32_MAX) continue;
        SDL_Log("Indice %u: %u", i, start_indices_data[i]);
    }
    vulkan_object_destroy(vulkan, sizeof(start_indices), &start_indices, (vulkan_destroy_func)vulkan_buffer_destroy);

    vulkan_command_queue *density_queue = vulkan_command_begin(&app->frame_arena);

    compute_densities(app, density_queue, &result, PARTICLE_COUNT, sorted);
    if (!vulkan_command_end(density_queue, vulkan, true))
    {
        return false;
    }

    vulkan_buffer target_densities;
    if (!vulkan_buffer_device_local_get_data(vulkan, &app->frame_arena, result.densities, "target densities", &target_densities))
    {
        return false;
    }

    u32 middle_index = PARTICLE_COUNT / 2;
    f32 density      = ((f32 *)target_densities.host_visible.data)[middle_index];
    SDL_Log("Density %u: %.4f", middle_index, density);

    vulkan_object_destroy(vulkan, sizeof(target_densities), &target_densities, (vulkan_destroy_func)vulkan_buffer_destroy);
    */
    result.initialized = true;

    *out_simulation = result;

    return true;
}

void simulation_ubo_update(app *app, simulation *simulation)
{
    if (!simulation->initialized)
    {
        return;
    }

    ui_layout_context *layout = &app->ui_layout;
    simulation_ubo    *ubo    = &simulation->ubo_data;

    ubo->poly6_normalization          = 315.0f / (64.0f * SDL_PI_F * SDL_powf(ubo->smoothing_radius, 9));
    ubo->spiky_gradient_normalization = -45.0f / (SDL_PI_F * SDL_powf(ubo->smoothing_radius, 6));
    ubo->particle_color0              = v4fromcolor4(layout->particle_gradient.colors[0]);
    ubo->particle_color1              = v4fromcolor4(layout->particle_gradient.colors[1]);
    ubo->particle_color2              = v4fromcolor4(layout->particle_gradient.colors[2]);
    ubo->particle_color3              = v4fromcolor4(layout->particle_gradient.colors[3]);
    ubo->particle_color_positions     = v4make2(layout->particle_gradient.positions);

    SDL_memcpy(vulkan_bindless_scene_ubo_get(&app->vulkan, simulation->scene), ubo, sizeof(simulation_ubo));
}

void simulation_update(app *app, simulation *simulation, f32 dt)
{
    if (!simulation->initialized)
    {
        return;
    }

    vulkan               *vulkan = &app->vulkan;
    vulkan_command_queue *queue  = app->render_queue;

    simulation->elapsed_time += dt;

    const u32 particle_count = PARTICLE_X * PARTICLE_Y * PARTICLE_Z;

    u32 sorted = spatial_lookup_sort(app, queue, simulation);

    vulkan_command_barrier(queue, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);
    compute_densities(app, queue, simulation, particle_count, sorted);

    vulkan_command_barrier(queue, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);

    vulkan_command_label_begin(queue, "update particles", YELLOW);
    vulkan_command_bind_pipeline(queue, app->pipelines[PIPELINE_PARTICLE_UPDATE]);
    vulkan_command_bind_scene_ubo(queue, simulation->scene, app->pipelines[PIPELINE_PARTICLE_UPDATE]);

    particle_update_pc update_pc = {
        .positions_read_addr   = vulkan_buffer_address_get(vulkan, simulation->sorted_positions),
        .positions_write_addr  = vulkan_buffer_address_get(vulkan, simulation->positions[simulation->write_index]),
        .velocities_read_addr  = vulkan_buffer_address_get(vulkan, simulation->sorted_velocities),
        .velocities_write_addr = vulkan_buffer_address_get(vulkan, simulation->velocities[simulation->write_index]),
        .densities_addr        = vulkan_buffer_address_get(vulkan, simulation->densities),
        .spatial_lookup_addr   = vulkan_buffer_address_get(vulkan, simulation->spatial_lookup[sorted]),
        .start_indices_addr    = vulkan_buffer_address_get(vulkan, simulation->start_indices),
        .box_pos               = v4fromv3(app->bounding_box.pos, 1.0f),
        .box_size              = v4fromv3(app->bounding_box.size, 0.0f),
        .dt                    = dt,
        .first_run             = simulation->first_loop ? 1.0f : 0.0f,
    };

    vulkan_command_push_constants(queue, sizeof(update_pc), &update_pc, VK_SHADER_STAGE_COMPUTE_BIT, app->pipelines[PIPELINE_PARTICLE_UPDATE]);

    vulkan_command_dispatch(queue, (particle_count + 255) / 256, 1, 1);
    vulkan_command_label_end(queue);

    simulation->read_index  = (simulation->read_index + 1) % PING_PONG_COUNT;
    simulation->write_index = (simulation->write_index + 1) % PING_PONG_COUNT;

    simulation->first_loop = false;
}

void simulation_draw(app *app, simulation *simulation)
{
    if (!simulation->initialized)
    {
        return;
    }

    vulkan               *vulkan = &app->vulkan;
    vulkan_command_queue *queue  = app->render_queue;

    vulkan_command_label_begin(queue, "render_particles", BLUE);
    vulkan_command_bind_pipeline(queue, app->pipelines[PIPELINE_PARTICLE_RENDER]);
    vulkan_command_bind_scene_ubo(queue, simulation->scene, app->pipelines[PIPELINE_PARTICLE_RENDER]);

    particle_render_pc pc = {
        .positions_addr  = vulkan_buffer_address_get(vulkan, simulation->positions[simulation->read_index]),
        .velocities_addr = vulkan_buffer_address_get(vulkan, simulation->velocities[simulation->read_index]),
        .particle_count  = PARTICLE_X * PARTICLE_Y * PARTICLE_Z,
        .particle_radius = app->particle_radius,
    };

    vulkan_command_push_constants(queue, sizeof(pc), &pc, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, app->pipelines[PIPELINE_PARTICLE_RENDER]);
    vulkan_command_draw(queue, PARTICLE_X * PARTICLE_Y * PARTICLE_Z * 6);
    vulkan_command_label_end(queue);
}

void simulation_destroy(app *app, simulation *simulation)
{
    if (!simulation->initialized)
    {
        return;
    }

    vulkan *vulkan = &app->vulkan;

    vulkan_bindless_scene_ubo_release(vulkan, simulation->scene);

    vulkan_object_destroy(vulkan, sizeof(simulation->positions[0]), &simulation->positions[0], (vulkan_destroy_func)vulkan_buffer_destroy);
    vulkan_object_destroy(vulkan, sizeof(simulation->positions[1]), &simulation->positions[1], (vulkan_destroy_func)vulkan_buffer_destroy);
    vulkan_object_destroy(vulkan, sizeof(simulation->velocities[0]), &simulation->velocities[0], (vulkan_destroy_func)vulkan_buffer_destroy);
    vulkan_object_destroy(vulkan, sizeof(simulation->velocities[1]), &simulation->velocities[1], (vulkan_destroy_func)vulkan_buffer_destroy);
    vulkan_object_destroy(vulkan, sizeof(simulation->densities), &simulation->densities, (vulkan_destroy_func)vulkan_buffer_destroy);
    vulkan_object_destroy(vulkan, sizeof(simulation->spatial_lookup[0]), &simulation->spatial_lookup[0], (vulkan_destroy_func)vulkan_buffer_destroy);
    vulkan_object_destroy(vulkan, sizeof(simulation->spatial_lookup[1]), &simulation->spatial_lookup[1], (vulkan_destroy_func)vulkan_buffer_destroy);
    vulkan_object_destroy(vulkan, sizeof(simulation->spatial_lookup_histograms), &simulation->spatial_lookup_histograms, (vulkan_destroy_func)vulkan_buffer_destroy);
    vulkan_object_destroy(vulkan, sizeof(simulation->start_indices), &simulation->start_indices, (vulkan_destroy_func)vulkan_buffer_destroy);
    vulkan_object_destroy(vulkan, sizeof(simulation->sorted_positions), &simulation->sorted_positions, (vulkan_destroy_func)vulkan_buffer_destroy);
    vulkan_object_destroy(vulkan, sizeof(simulation->sorted_velocities), &simulation->sorted_velocities, (vulkan_destroy_func)vulkan_buffer_destroy);

    simulation->initialized = false;
}
