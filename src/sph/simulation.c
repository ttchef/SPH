
#include <math/core.h>
#include <sph/png.h>
#include <sph/simulation.h>
#include <sph/utils.h>

typedef struct
{
    m4 view;
    m4 perspective;
    m4 orthographic;
} global_ubo;

static void draw_text(simulation *simulation, v2 pos, f32 scale, const char *format, ...)
{
    va_list args;
    va_start(args, format);

    static char buffer[4096];
    i32         written = SDL_vsnprintf(buffer, sizeof(buffer), format, args);
    if (written < 0 || written >= (i32)sizeof(buffer))
    {
        SDL_Log("[ENGINE] Draw string format error.");
        va_end(args);
        return;
    }

    vulkan *vulkan = &simulation->vulkan;

    vulkan_command_bind_pipeline(vulkan, simulation->pipelines[PIPELINE_TEXTURED_QUAD]);

    ttf_font *font = &simulation->jet_brains;

    textured_quad_pc pc = {
        .image   = font->atlas.descriptor,
        .sampler = simulation->linear_sampler.descriptor,
    };

    const f32 line_height = font->ascent - font->descent + font->line_gap;
    v2        write       = v2make(pos.x, SDL_roundf(pos.y + font->ascent * scale - font->size_px * scale));

    for (i32 i = 0; i < written; i++)
    {
        char c = buffer[i];

        if (c == ' ')
        {
            write.x += font->glyphs[0].advance * scale;
            continue;
        }
        if (c == '\n')
        {
            write.x = pos.x;
            write.y = SDL_roundf(write.y + line_height * scale);
            continue;
        }
        if (c < '!' || c > '~')
        {
            continue;
        }

        ttf_glyph *glyph = &font->glyphs[c - '!'];

        pc.uv_min = glyph->uv_min;
        pc.uv_max = glyph->uv_max;

        f32 w = (f32)glyph->size_px.x * scale;
        f32 h = (f32)glyph->size_px.y * scale;

        v2 top_left = v2make(write.x + glyph->bearing.x * scale, write.y + font->size_px * scale - glyph->bearing.y * scale);
        v2 center   = v2make(top_left.x + w * 0.5f, top_left.y + h * 0.5f);

        m4 scale_m   = m4scale(w, h, 1.0f);
        m4 translate = m4translate(center.x, center.y, 0.0f);

        pc.model = m4mul(translate, scale_m);

        vulkan_command_push_constants(vulkan, sizeof(textured_quad_pc), &pc, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, simulation->pipelines[PIPELINE_TEXTURED_QUAD]);
        vulkan_command_draw(vulkan, 6);

        write.x += glyph->advance * scale;
    }

    va_end(args);
}

static void draw_screen_quad(simulation *simulation, v2 pos, v2 scale, vulkan_image *image, vulkan_sampler *sampler)
{
    vulkan *vulkan = &simulation->vulkan;

    vulkan_command_bind_pipeline(vulkan, simulation->pipelines[PIPELINE_TEXTURED_QUAD]);

    v2 top_left = v2add(pos, v2scale(scale, 0.5f));

    m4 scale_m   = m4scale(scale.x, scale.y, 1.0);
    m4 translate = m4translate(top_left.x, top_left.y, 0.0f);
    m4 model     = m4mul(translate, scale_m);

    textured_quad_pc pc = {
        .model   = model,
        .uv_min  = v2make(0.0f, 0.0f),
        .uv_max  = v2make(1.0f, 1.0f),
        .image   = image->descriptor,
        .sampler = sampler->descriptor,
    };

    vulkan_command_push_constants(vulkan, sizeof(textured_quad_pc), &pc, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, simulation->pipelines[PIPELINE_TEXTURED_QUAD]);
    vulkan_command_draw(vulkan, 6);
}

static void draw_cube_lines(simulation *simulation, v3 pos, v3 scale)
{
    vulkan *vulkan = &simulation->vulkan;

    m4 scale_m   = m4scale(scale.x, scale.y, scale.z);
    m4 translate = m4translate(pos.x, pos.y, pos.z);
    m4 model     = m4mul(translate, scale_m);

    cube_lines_pc pc = {
        .model = model,
    };

    vulkan_command_bind_pipeline(vulkan, simulation->pipelines[PIPELINE_CUBE_LINES]);
    vulkan_command_push_constants(vulkan, sizeof(pc), &pc, VK_SHADER_STAGE_VERTEX_BIT, simulation->pipelines[PIPELINE_CUBE_LINES]);
    vulkan_command_draw(vulkan, 24);
}

static bool resources_create(simulation *simulation)
{
    vulkan *vulkan = &simulation->vulkan;

    usize jet_brains_size;
    u8   *jet_brains_data = SDL_LoadFile(path_abs("assets/fonts/jet-brains.ttf"), &jet_brains_size);
    if (!ttf_create(vulkan, jet_brains_size, jet_brains_data, "jet-brains", &simulation->jet_brains))
    {
        return false;
    }

    vulkan_sampler_create(vulkan, true, "linear_sampler", &simulation->linear_sampler);

    usize test_size;
    u8   *test_data = SDL_LoadFile(path_abs("assets/textures/watermelon.png"), &test_size);

    image_raw test_image;
    if (!png_create(test_size, test_data, &test_image))
    {
        return false;
    }

    vulkan_image_create(vulkan, v2umake(test_image.width, test_image.height), VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_ASPECT_COLOR_BIT, false, "watermelon", &simulation->test_texture);
    vulkan_image_data_upload(vulkan, &simulation->test_texture, test_image.width * test_image.height * 4, test_image.data, v2umake(test_image.width, test_image.height), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, true);

    SDL_free(test_image.data);
    SDL_free(test_data);
    SDL_free(jet_brains_data);

    return true;
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

    if (!pipelines_create(&simulation->vulkan, simulation->pipelines, PIPELINE_COUNT))
    {
        return false;
    }

    if (!resources_create(simulation))
    {
        return false;
    }

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

    vulkan_command_label_begin(vulkan, "render_cube", RED);
    draw_cube_lines(simulation, v3make(0, 0, 0), v3make(300, 300, 300));
    vulkan_command_label_end(vulkan);

    vulkan_command_label_begin(vulkan, "render_quads", BLUE);
    draw_screen_quad(simulation, v2make(window->width * 0.5 - 200 * 0.5 + SDL_sinf(simulation->time.accumulated * 2) * 400, window->height * 0.5 - 200 * 0.5), v2make(200, 200), &simulation->test_texture, &simulation->linear_sampler);
    draw_screen_quad(simulation, v2make(window->width * 0.5 - 200 * 0.5, window->height * 0.5 - 200 * 0.5 + SDL_cosf(simulation->time.accumulated * 2) * 400), v2make(200, 200), &simulation->test_texture, &simulation->linear_sampler);
    draw_screen_quad(simulation, v2make(window->width * 0.5 - 200 * 0.5 + SDL_sinf(simulation->time.accumulated * 2) * 400, window->height * 0.5 - 200 * 0.5 + SDL_cosf(simulation->time.accumulated * 2) * 400), v2make(200, 200), &simulation->test_texture, &simulation->linear_sampler);
    vulkan_command_label_end(vulkan);

    vulkan_command_label_begin(vulkan, "render_text", GREEN);
    draw_text(simulation, v2make(0, 0), 0.5f, "Frametime: %.4fms\nHello World", simulation->time.smooth_delta * 1000.0f);
    vulkan_command_label_end(vulkan);

    vulkan_command_end_rendering(vulkan);

    vulkan_draw(vulkan, simulation->window.width, simulation->window.height);
    input_update(&simulation->input, NULL);
}

void simulation_destroy(simulation *simulation)
{
    vulkan *vulkan = &simulation->vulkan;
    vkDeviceWaitIdle(vulkan->device);

    ttf_destroy(vulkan, &simulation->jet_brains);
    vulkan_sampler_destroy(vulkan, &simulation->linear_sampler);
    vulkan_image_destroy(vulkan, simulation->test_texture);

    vulkan_destroy(vulkan);
    window_destroy(&simulation->window);
}
