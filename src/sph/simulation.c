
#include "sph/ttf.h"
#include "vk/command.h"
#include "vk/image.h"
#include "vk/pipeline.h"
#include <sph/simulation.h>
#include <vulkan/vulkan_core.h>

typedef struct
{
    vulkan_bindless_image   image;
    vulkan_bindless_sampler sampler;
} pipeline_pc;

bool simulation_create(simulation *simulation)
{
    if (!window_create(&simulation->window, 2560, 1440))
    {
        SDL_Log("[ENGINE] Failed to create window.");
        return false;
    }

    if (!vulkan_create(simulation->window.handle, &simulation->vulkan))
    {
        SDL_Log("[ENGINE] Failed to init vulkan.");
        return false;
    }

    simulation->camera = camera_create();
    simulation->input  = input_create();

    vulkan_pipeline_desc desc = vulkan_pipeline_default(VULKAN_PIPELINE_TYPE_GRAPHICS);

    vulkan_pipeline_desc_set_shaders(&desc, "src/shaders/spv/hello.spv", "src/shaders/spv/hello.spv", NULL);
    vulkan_pipeline_desc_set_shaders_entries(&desc, "vertexMain", "fragmentMain", NULL);
    vulkan_pipeline_desc_set_push_constant(&desc, sizeof(pipeline_pc), VK_SHADER_STAGE_FRAGMENT_BIT);

    simulation->pipeline = vulkan_pipeline_create(&simulation->vulkan, &desc);
    assert(simulation->pipeline != VULKAN_INVALID_PIPELINE);

    usize jet_brains_size;
    u8 *jet_brains_data = SDL_LoadFile("assets/fonts/jet-brains.ttf", &jet_brains_size);

    ttf_create(&simulation->vulkan, jet_brains_size, jet_brains_data, &simulation->jet_brains);

    vulkan_sampler_create(&simulation->vulkan, true, &simulation->sampler);

    return true;
}

void simulation_event(simulation *simulation, SDL_Event *event)
{
    input_update(&simulation->input, event);

    if (event->type == SDL_EVENT_WINDOW_RESIZED)
    {
        window_resize(&simulation->window);
        vulkan_resize(&simulation->vulkan, simulation->window.width, simulation->window.height);
    }
}

void simulation_update(simulation *simulation)
{
    time_update(&simulation->time);
    camera_update(&simulation->camera, &simulation->window, &simulation->input, simulation->time.delta);

    window *window = &simulation->window;
    vulkan *vulkan = &simulation->vulkan;
    input  *input  = &simulation->input;

    if (input_pressed(input, INPUT_W))
    {
        SDL_Log("W pressed.");
    }

    vulkan_command_begin_rendering(vulkan);
    vulkan_command_set_viewport(vulkan, 0, window->height, window->width, -(f32)window->height);

    vulkan_command_bind_pipeline(vulkan, simulation->pipeline);

    pipeline_pc pc = {
        .image = simulation->jet_brains.atlas.descriptor,
        .sampler = simulation->sampler.descriptor, 
    };
    
    vulkan_command_push_constants(vulkan, sizeof(pipeline_pc), &pc, VK_SHADER_STAGE_FRAGMENT_BIT, simulation->pipeline);
    vulkan_command_draw(vulkan, 6);

    vulkan_command_end_rendering(vulkan);

    vulkan_draw(vulkan, simulation->window.width, simulation->window.height);
    input_update(&simulation->input, NULL);
}

void simulation_destroy(simulation *simulation)
{
    vkDeviceWaitIdle(simulation->vulkan.device);

    ttf_destroy(&simulation->vulkan, &simulation->jet_brains);
    vulkan_sampler_destroy(&simulation->vulkan, &simulation->sampler);
    
    vulkan_destroy(&simulation->vulkan);
    window_destroy(&simulation->window);
}
