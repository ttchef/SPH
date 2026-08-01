
#include <sph/app.h>
#include <sph/simulation.h>
#include <vk/context.h>

bool simulation_create(vulkan *vulkan, simulation *out_simulation)
{
    simulation result = {0};

    *out_simulation = result;

    return true;
}

void simulation_update(vulkan *vulkan, simulation *simulation)
{
    simulation->elapsed_time += 1.0f / 360.0f;
}

void simulation_draw(app *app, vulkan *vulkan, simulation *simulation)
{
    window *window = &app->window;

    vulkan_command_label_begin(vulkan, "render_quads", BLUE);
    draw_quad(app, v2make(window->width * 0.5 - 200 * 0.5 + SDL_sinf(simulation->elapsed_time * 2) * 400, window->height * 0.5 - 200 * 0.5 + SDL_cosf(simulation->elapsed_time * 2) * 400), v2make(200, 200), 0.0f, WHITE, &app->test_texture, &app->linear_sampler);
    vulkan_command_label_end(vulkan);
}

void simulation_destroy(vulkan *vulkan, simulation *simulation)
{
    vkDeviceWaitIdle(vulkan->device);
}
