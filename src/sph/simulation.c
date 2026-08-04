
#include <sph/app.h>
#include <sph/simulation.h>
#include <vk/context.h>

#define PARTICLE_X 30
#define PARTICLE_Y 30
#define PARTICLE_Z 20

static void simulation_compute_densities(app *app, vulkan_command_queue *queue, simulation *simulation, u32 particle_count)
{
    vulkan *vulkan = &app->vulkan;

    vulkan_command_label_begin(queue, "compute densities", CYAN);
    vulkan_command_bind_pipeline(queue, app->pipelines[PIPELINE_PARTICLE_DENSITY]);
    vulkan_command_bind_scene_ubo(queue, simulation->scene, app->pipelines[PIPELINE_PARTICLE_DENSITY]);

    particle_density_pc density_pc = {
        .densities_addr      = vulkan_buffer_address_get(vulkan, simulation->densities),
        .positions_read_addr = vulkan_buffer_address_get(vulkan, simulation->positions[simulation->read_index]),
    };
    vulkan_command_push_constants(queue, sizeof(density_pc), &density_pc, VK_SHADER_STAGE_COMPUTE_BIT, app->pipelines[PIPELINE_PARTICLE_DENSITY]);
    vulkan_command_dispatch(queue, (particle_count + 255) / 256, 1, 1);

    vulkan_command_label_end(queue);
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
    for (i32 z = -PARTICLE_Z / 2; z < PARTICLE_Z / 2; z++)
    {
        for (i32 y = -PARTICLE_Y / 2; y < PARTICLE_Y / 2; y++)
        {
            for (i32 x = -PARTICLE_X / 2; x < PARTICLE_X / 2; x++)
            {
                v4 pos                               = v4make(x * 5, y * 5, z * 5, 1.0f);
                particle_positions[particle_index++] = pos;
            }
        }
    }

    const char *buffer_strings[] = {
        "particle_positions_zero",
        "particle_positions_one",
        "particle_velocities_zero",
        "particle_velocities_one",
        "particle_densites",
    };
    u32                   string_index   = 0;
    u32                   particle_count = PARTICLE_X * PARTICLE_Y * PARTICLE_Z;
    VkBufferUsageFlagBits usage          = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

    vulkan_buffer_device_local_create(vulkan, &app->frame_arena, usage, sizeof(v4) * particle_count, particle_positions, buffer_strings[string_index++], &result.positions[0]);
    vulkan_buffer_device_local_create(vulkan, &app->frame_arena, usage, sizeof(v4) * particle_count, NULL, buffer_strings[string_index++], &result.positions[1]);
    vulkan_buffer_device_local_create(vulkan, &app->frame_arena, usage, sizeof(v4) * particle_count, NULL, buffer_strings[string_index++], &result.velocities[0]);
    vulkan_buffer_device_local_create(vulkan, &app->frame_arena, usage, sizeof(v4) * particle_count, NULL, buffer_strings[string_index++], &result.velocities[1]);
    vulkan_buffer_device_local_create(vulkan, &app->frame_arena, usage, sizeof(f32) * particle_count, NULL, buffer_strings[string_index++], &result.densities);

    result.scene    = vulkan_bindless_scene_ubo_aquire(&vulkan->bindless);
    result.ubo_data = (simulation_ubo){
        .particle_count      = particle_count,
        .target_density      = 0.0065,
        .pressure_multiplier = 2.5f,
        .smoothing_radius    = 15.0f,
        .viscosity_coeff     = 15.0f,
    };
    SDL_memcpy(vulkan_bindless_scene_ubo_get(&vulkan->bindless, result.scene), &result.ubo_data, sizeof(simulation_ubo));

    vulkan_command_queue *density_queue = vulkan_command_begin(&app->frame_arena);

    simulation_compute_densities(app, density_queue, &result, particle_count);
    if (!vulkan_command_end(density_queue, vulkan, true))
    {
        return false;
    }

    vulkan_buffer target_densities;
    if (!vulkan_buffer_device_local_get_data(vulkan, result.densities, "target densities", &target_densities))
    {
        return false;
    }

    u32 middle_index = particle_count / 2;
    f32 density      = ((f32 *)target_densities.host_visible.data)[middle_index];
    SDL_Log("Density %u: %.4f", middle_index, density);

    vulkan_object_destroy(vulkan, sizeof(target_densities), &target_densities, (vulkan_destroy_func)vulkan_buffer_destroy);

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

    SDL_memcpy(vulkan_bindless_scene_ubo_get(&app->vulkan.bindless, simulation->scene), &simulation->ubo_data, sizeof(simulation_ubo));
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

    simulation_compute_densities(app, queue, simulation, particle_count);

    vulkan_command_barrier(queue, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);

    vulkan_command_label_begin(queue, "update particles", YELLOW);
    vulkan_command_bind_pipeline(queue, app->pipelines[PIPELINE_PARTICLE_UPDATE]);
    vulkan_command_bind_scene_ubo(queue, simulation->scene, app->pipelines[PIPELINE_PARTICLE_UPDATE]);

    particle_update_pc update_pc = {
        .positions_read_addr   = vulkan_buffer_address_get(vulkan, simulation->positions[simulation->read_index]),
        .positions_write_addr  = vulkan_buffer_address_get(vulkan, simulation->positions[simulation->write_index]),
        .velocities_read_addr  = vulkan_buffer_address_get(vulkan, simulation->velocities[simulation->read_index]),
        .velocities_write_addr = vulkan_buffer_address_get(vulkan, simulation->velocities[simulation->write_index]),
        .densities_addr        = vulkan_buffer_address_get(vulkan, simulation->densities),
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
    simulation->first_loop  = false;
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

    vulkan_object_destroy(vulkan, sizeof(simulation->positions[0]), &simulation->positions[0], (vulkan_destroy_func)vulkan_buffer_destroy);
    vulkan_object_destroy(vulkan, sizeof(simulation->positions[1]), &simulation->positions[1], (vulkan_destroy_func)vulkan_buffer_destroy);
    vulkan_object_destroy(vulkan, sizeof(simulation->velocities[0]), &simulation->velocities[0], (vulkan_destroy_func)vulkan_buffer_destroy);
    vulkan_object_destroy(vulkan, sizeof(simulation->velocities[1]), &simulation->velocities[1], (vulkan_destroy_func)vulkan_buffer_destroy);
    vulkan_object_destroy(vulkan, sizeof(simulation->densities), &simulation->densities, (vulkan_destroy_func)vulkan_buffer_destroy);

    simulation->initialized = false;
}
