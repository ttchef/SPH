
#include <sph/app.h>
#include <sph/simulation.h>
#include <vk/context.h>

#define PARTICLE_X 30
#define PARTICLE_Y 30
#define PARTICLE_Z 30

static void simulation_compute_densities(app *app, vulkan *vulkan, simulation *simulation, u32 particle_count)
{
    vulkan_command_label_begin(vulkan, "compute densities", CYAN);
    vulkan_command_bind_pipeline(vulkan, app->pipelines[PIPELINE_PARTICLE_DENSITY]);

    particle_density_pc density_pc = {
        .densities_addr      = vulkan_buffer_address_get(vulkan, simulation->densities),
        .positions_read_addr = vulkan_buffer_address_get(vulkan, simulation->positions[simulation->read_index]),
        .particle_count      = particle_count,
    };
    vulkan_command_push_constants(vulkan, sizeof(density_pc), &density_pc, VK_SHADER_STAGE_COMPUTE_BIT, app->pipelines[PIPELINE_PARTICLE_DENSITY]);
    vulkan_command_dispatch(vulkan, (particle_count + 255) / 256, 1, 1);

    vulkan_command_label_end(vulkan);
}

bool simulation_create(app *app, vulkan *vulkan, simulation *out_simulation)
{
    simulation result = {0};

    result.read_index  = 0;
    result.write_index = 1;
    result.first_loop  = true;

    v4 *particle_positions = SDL_calloc(PARTICLE_X * PARTICLE_Y * PARTICLE_Z, sizeof(v4));

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
        "particle_densites_zero",
        "particle_densities_one",
    };
    u32                   string_index   = 0;
    u32                   particle_count = PARTICLE_X * PARTICLE_Y * PARTICLE_Z;
    VkBufferUsageFlagBits usage          = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

    vulkan_buffer_device_local_create(vulkan, usage, sizeof(v4) * particle_count, particle_positions, buffer_strings[string_index++], &result.positions[0]);
    vulkan_buffer_device_local_create(vulkan, usage, sizeof(v4) * particle_count, NULL, buffer_strings[string_index++], &result.positions[1]);
    vulkan_buffer_device_local_create(vulkan, usage, sizeof(v4) * particle_count, NULL, buffer_strings[string_index++], &result.velocities[0]);
    vulkan_buffer_device_local_create(vulkan, usage, sizeof(v4) * particle_count, NULL, buffer_strings[string_index++], &result.velocities[1]);
    vulkan_buffer_device_local_create(vulkan, usage, sizeof(f32) * particle_count, NULL, buffer_strings[string_index++], &result.densities);

    SDL_free(particle_positions);

    /*
    simulation_compute_densities(app, vulkan, &result, particle_count);
    // NOTE: Currently very hacky way change that later
    vulkan_draw(vulkan, app->window.width, app->window.height);

    vulkan_buffer target_densities;
    if (!vulkan_buffer_device_local_get_data(vulkan, result.densities, "target densities", &target_densities))
    {
        return false;
    }

    u32 middle_index = (PARTICLE_X / 2) * (PARTICLE_Y / 2) * (PARTICLE_Z / 2);
    f32 density = ((f32 *)target_densities.host_visible.data)[middle_index];
    SDL_Log("Density %u: %.4f", middle_index, density);

    vulkan_object_destroy(vulkan, sizeof(target_densities), &target_densities, (vulkan_destroy_func)vulkan_buffer_destroy);
*/
    *out_simulation = result;

    return true;
}

void simulation_update(app *app, vulkan *vulkan, simulation *simulation)
{
    const f32 dt = 1.0f / 60.0f;
    simulation->elapsed_time += dt;

    const u32 particle_count = PARTICLE_X * PARTICLE_Y * PARTICLE_Z;

    simulation_compute_densities(app, vulkan, simulation, particle_count);

    vulkan_command_barrier(vulkan, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);

    vulkan_command_label_begin(vulkan, "update_particles", YELLOW);
    vulkan_command_bind_pipeline(vulkan, app->pipelines[PIPELINE_PARTICLE_UPDATE]);

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
        .particle_count        = particle_count,
    };

    vulkan_command_push_constants(vulkan, sizeof(update_pc), &update_pc, VK_SHADER_STAGE_COMPUTE_BIT, app->pipelines[PIPELINE_PARTICLE_UPDATE]);

    vulkan_command_dispatch(vulkan, (particle_count + 255) / 256, 1, 1);
    vulkan_command_label_end(vulkan);

    // TODO: Only maybe?
    // vulkan_command_barrier(vulkan, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);

    simulation->read_index  = (simulation->read_index + 1) % PING_PONG_COUNT;
    simulation->write_index = (simulation->write_index + 1) % PING_PONG_COUNT;
    simulation->first_loop = false;
}

void simulation_draw(app *app, vulkan *vulkan, simulation *simulation)
{
    vulkan_command_label_begin(vulkan, "render_particles", BLUE);
    vulkan_command_bind_pipeline(vulkan, app->pipelines[PIPELINE_PARTICLE_RENDER]);

    particle_render_pc pc = {
        .positions_addr = vulkan_buffer_address_get(vulkan, simulation->positions[simulation->read_index]),
        .velocities_addr = vulkan_buffer_address_get(vulkan, simulation->velocities[simulation->read_index]),
        .particle_count = PARTICLE_X * PARTICLE_Y * PARTICLE_Z,
        .particle_radius = app->particle_radius,
    };

    vulkan_command_push_constants(vulkan, sizeof(pc), &pc, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, app->pipelines[PIPELINE_PARTICLE_RENDER]);
    vulkan_command_draw(vulkan, PARTICLE_X * PARTICLE_Y * PARTICLE_Z * 6);
    vulkan_command_label_end(vulkan);
}

void simulation_destroy(vulkan *vulkan, simulation *simulation)
{
    vulkan_object_destroy(vulkan, sizeof(simulation->positions[0]), &simulation->positions[0], (vulkan_destroy_func)vulkan_buffer_destroy);
    vulkan_object_destroy(vulkan, sizeof(simulation->positions[1]), &simulation->positions[1], (vulkan_destroy_func)vulkan_buffer_destroy);
    vulkan_object_destroy(vulkan, sizeof(simulation->velocities[0]), &simulation->velocities[0], (vulkan_destroy_func)vulkan_buffer_destroy);
    vulkan_object_destroy(vulkan, sizeof(simulation->velocities[1]), &simulation->velocities[1], (vulkan_destroy_func)vulkan_buffer_destroy);
    vulkan_object_destroy(vulkan, sizeof(simulation->densities), &simulation->densities, (vulkan_destroy_func)vulkan_buffer_destroy);
}
