
#include <sph/app.h>
#include <sph/simulation.h>
#include <vk/context.h>

#define PARTICLE_X 20
#define PARTICLE_Y 20
#define PARTICLE_Z 20

bool simulation_create(vulkan *vulkan, simulation *out_simulation)
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
    VkBufferUsageFlagBits usage          = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

    vulkan_buffer_device_local_create(vulkan, usage, sizeof(v4) * particle_count, particle_positions, buffer_strings[string_index++], &result.positions[0]);
    vulkan_buffer_device_local_create(vulkan, usage, sizeof(v4) * particle_count, NULL, buffer_strings[string_index++], &result.positions[1]);
    vulkan_buffer_device_local_create(vulkan, usage, sizeof(v4) * particle_count, NULL, buffer_strings[string_index++], &result.velocities[0]);
    vulkan_buffer_device_local_create(vulkan, usage, sizeof(v4) * particle_count, NULL, buffer_strings[string_index++], &result.velocities[1]);

    SDL_free(particle_positions);

    *out_simulation = result;

    return true;
}

void simulation_update(app *app, vulkan *vulkan, simulation *simulation)
{
    const f32 dt = 1.0f / 360.0f;
    simulation->elapsed_time += dt;

    vulkan_command_label_begin(vulkan, "update_particles", YELLOW);
    vulkan_command_bind_pipeline(vulkan, app->pipelines[PIPELINE_PARTICLE_UPDATE]);

    particle_update_pc pc = {
        .positions_read_addr   = vulkan_buffer_address_get(vulkan, simulation->positions[simulation->read_index]),
        .positions_write_addr  = vulkan_buffer_address_get(vulkan, simulation->positions[simulation->write_index]),
        .velocities_read_addr  = vulkan_buffer_address_get(vulkan, simulation->velocities[simulation->read_index]),
        .velocities_write_addr = vulkan_buffer_address_get(vulkan, simulation->velocities[simulation->write_index]),
        .box_pos               = v4fromv3(app->bounding_box.pos, 1.0f),
        .box_size              = v4fromv3(app->bounding_box.size, 0.0f),
        .dt                    = dt,
        .first_run             = simulation->first_loop ? 1.0f : 0.0f,
    };

    vulkan_command_push_constants(vulkan, sizeof(pc), &pc, VK_SHADER_STAGE_COMPUTE_BIT, app->pipelines[PIPELINE_PARTICLE_UPDATE]);

    const u32 particle_count = PARTICLE_X * PARTICLE_Y * PARTICLE_Z;
    vulkan_command_dispatch(vulkan, (particle_count + 255) / 256, 1, 1);
    vulkan_command_label_end(vulkan);

    simulation->read_index  = (simulation->read_index + 1) % PING_PONG_COUNT;
    simulation->write_index = (simulation->write_index + 1) % PING_PONG_COUNT;
}

void simulation_draw(app *app, vulkan *vulkan, simulation *simulation)
{
    vulkan_command_label_begin(vulkan, "render_particles", BLUE);
    // draw_quad(app, v2make(window->width * 0.5 - 200 * 0.5 + SDL_sinf(simulation->elapsed_time * 2) * 400, window->height * 0.5 - 200 * 0.5 + SDL_cosf(simulation->elapsed_time * 2) * 400), v2make(200, 200), 0.0f, WHITE, &app->test_texture, &app->linear_sampler);
    vulkan_command_bind_pipeline(vulkan, app->pipelines[PIPELINE_PARTICLE_RENDER]);

    particle_render_pc pc = {
        .positions_addr = vulkan_buffer_address_get(vulkan, simulation->positions[simulation->write_index]),
        .particle_count = PARTICLE_X * PARTICLE_Y * PARTICLE_Z,
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
}
