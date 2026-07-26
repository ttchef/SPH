
#include <sph/simulation.h>

#include <math/matrix.h>

typedef struct
{
    m4 view;
    m4 perspective;
    m4 orthographic;
} global_ubo;

typedef struct
{
    m4                      model;
    vulkan_bindless_image   image;
    vulkan_bindless_sampler sampler;
    v2                      uv_min;
    v2                      uv_max;
} text_pc;

static void draw_text(simulation *simulation, const char *text, v2 pos, f32 scale)
{
    vulkan *vulkan = &simulation->vulkan;

    vulkan_command_bind_pipeline(vulkan, simulation->text_pipeline);

    ttf_font *font = &simulation->jet_brains;

    text_pc pc = {
        .image   = font->atlas.descriptor,
        .sampler = simulation->linear_sampler.descriptor,
    };

    for (u32 i = 0; i < SDL_strlen(text); i++)
    {
        char c = text[i];

        if (c == ' ')
        {
            pos.x += font->glyphs[0].advance * scale;
            continue;
        }
        if (c < '!' || c > '~')
        {
            continue;
        }

        // TODO: Get rid of the funky index
        ttf_glyph *glyph = &font->glyphs[c - '!'];

        const f32 margin = 0.001f;
        pc.uv_min = v2make(glyph->uv_min.x + margin, glyph->uv_min.y + margin);
        pc.uv_max = v2make(glyph->uv_max.x - margin, glyph->uv_max.y - margin);

        f32 baseline_y = pos.y + font->ascent * scale;

        f32 w = (f32)glyph->size_px.x * scale;
        f32 h = (f32)glyph->size_px.y * scale;

        v2 top_left = v2make(pos.x, baseline_y - glyph->bearing_y * scale);
        v2 center   = v2make(top_left.x + w * 0.5f, top_left.y + h * 0.5f);

        m4 scale_m   = m4scale(w, h, 1.0f);
        m4 translate = m4translate(center.x, center.y, 0.0f);

        pc.model = m4mul(translate, scale_m);

        vulkan_command_push_constants(vulkan, sizeof(text_pc), &pc, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, simulation->text_pipeline);
        vulkan_command_draw(vulkan, 6);

        pos.x += glyph->advance * scale;
    }
}

bool simulation_create(simulation *simulation)
{
    if (!window_create(&simulation->window, 2560, 1440))
    {
        SDL_Log("[ENGINE] Failed to create window.");
        return false;
    }

    if (!vulkan_create(simulation->window.handle, &simulation->vulkan, sizeof(global_ubo)))
    {
        SDL_Log("[ENGINE] Failed to init vulkan.");
        return false;
    }

    simulation->camera = camera_create();
    simulation->input  = input_create();

    vulkan_pipeline_desc desc = vulkan_pipeline_default(VULKAN_PIPELINE_TYPE_GRAPHICS);

    vulkan_pipeline_desc_set_shaders(&desc, "src/shaders/spv/hello.spv", "src/shaders/spv/hello.spv", NULL);
    vulkan_pipeline_desc_set_shaders_entries(&desc, "vertexMain", "fragmentMain", NULL);
    vulkan_pipeline_desc_set_push_constant(&desc, sizeof(text_pc), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

    simulation->text_pipeline = vulkan_pipeline_create(&simulation->vulkan, &desc);
    assert(simulation->text_pipeline != VULKAN_INVALID_PIPELINE);

    usize jet_brains_size;
    u8   *jet_brains_data = SDL_LoadFile("assets/fonts/jet-brains.ttf", &jet_brains_size);

    ttf_create(&simulation->vulkan, jet_brains_size, jet_brains_data, &simulation->jet_brains);

    vulkan_sampler_create(&simulation->vulkan, true, &simulation->linear_sampler);

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

static void global_ubo_update(simulation *simulation)
{
    global_ubo *ubo = vulkan_bindless_ubo_get(&simulation->vulkan.bindless);

    ubo->view = camera_view(&simulation->camera);

    f32 aspect_ratio = (f32)simulation->window.width / (f32)simulation->window.height;
    ubo->perspective = m4perspective(TO_RADIANS(87.0f), aspect_ratio, 0.1f, 1500.0f);

    ubo->orthographic = m4orthographic(0, simulation->window.width, simulation->window.height, 0, -1.0, 1.0f);
}

void simulation_update(simulation *simulation)
{
    time_update(&simulation->time);
    camera_update(&simulation->camera, &simulation->window, &simulation->input, simulation->time.delta);
    global_ubo_update(simulation);

    window *window = &simulation->window;
    vulkan *vulkan = &simulation->vulkan;

    vulkan_command_begin_rendering(vulkan);
    vulkan_command_set_viewport(vulkan, 0, 0, window->width, window->height);

    draw_text(simulation, "Hello World", v2make(0, 0), 1.0f);

    vulkan_command_end_rendering(vulkan);

    vulkan_draw(vulkan, simulation->window.width, simulation->window.height);
    input_update(&simulation->input, NULL);
}

void simulation_destroy(simulation *simulation)
{
    vkDeviceWaitIdle(simulation->vulkan.device);

    ttf_destroy(&simulation->vulkan, &simulation->jet_brains);
    vulkan_sampler_destroy(&simulation->vulkan, &simulation->linear_sampler);

    vulkan_destroy(&simulation->vulkan);
    window_destroy(&simulation->window);
}
