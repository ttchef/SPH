

#include <math/core.h>
#include <sph/app.h>
#include <sph/png.h>
#include <sph/utils.h>

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

typedef struct
{
    m4 view;
    m4 perspective;
    m4 orthographic;
} global_ubo;

f32 measure_text(ttf_font *font, u32 font_size, const char *format, ...)
{
    va_list args;
    va_start(args, format);

    static char buffer[4096];
    i32         written = SDL_vsnprintf(buffer, sizeof(buffer), format, args);
    if (written < 0 || written >= (i32)sizeof(buffer))
    {
        SDL_Log("[ENGINE] Draw string format error.");
        va_end(args);
        return 0.0f;
    }

    f32 max_width = 0.0f;

    const f32 scale       = (f32)font_size / (f32)font->size_px;
    const f32 line_height = font->ascent - font->descent + font->line_gap;
    v2        write       = v2make(0.0f, SDL_roundf(0.0f + font->ascent * scale - font->size_px * scale));

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
            max_width = MAX(max_width, write.x);
            write.x   = 0.0f;
            write.y   = SDL_roundf(write.y + line_height * scale);
            continue;
        }
        if (c < '!' || c > '~')
        {
            continue;
        }

        ttf_glyph *glyph = &font->glyphs[c - '!'];
        write.x += glyph->advance * scale;
    }

    va_end(args);

    max_width = MAX(max_width, write.x);
    return max_width;
}

void draw_text(app *app, ttf_font *font, v2 pos, u32 font_size, const char *format, ...)
{
    vulkan_command_queue *queue = app->render_queue;

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

    vulkan_command_bind_pipeline(queue, app->pipelines[PIPELINE_TEXTURED_QUAD]);

    textured_quad_pc pc = {
        .image   = font->atlas.descriptor,
        .sampler = app->linear_sampler.descriptor,
        .color   = v4fromcolor4(WHITE),
    };

    const f32 scale       = (f32)font_size / (f32)font->size_px;
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

        vulkan_command_push_constants(queue, sizeof(textured_quad_pc), &pc, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, app->pipelines[PIPELINE_TEXTURED_QUAD]);
        vulkan_command_draw(queue, 6);

        write.x += glyph->advance * scale;
    }

    va_end(args);
}

void draw_quad(app *app, v2 pos, v2 scale, f32 roundness, color4 color, vulkan_image *image, vulkan_sampler *sampler)
{
    vulkan_command_queue *queue = app->render_queue;
    vulkan_command_bind_pipeline(queue, app->pipelines[PIPELINE_TEXTURED_QUAD]);

    v2 center = v2add(pos, v2scale(scale, 0.5f));

    m4 scale_m   = m4scale(scale.x, scale.y, 1.0);
    m4 translate = m4translate(center.x, center.y, 0.0f);
    m4 model     = m4mul(translate, scale_m);

    textured_quad_pc pc = {
        .model        = model,
        .uv_min       = v2make(0.0f, 0.0f),
        .uv_max       = v2make(1.0f, 1.0f),
        .image        = image ? image->descriptor : VULKAN_INVALID_BINDING,
        .sampler      = sampler ? sampler->descriptor : VULKAN_INVALID_BINDING,
        .color        = v4fromcolor4(color),
        .roundness    = roundness,
        .aspect_ratio = scale.x / scale.y,
    };

    vulkan_command_push_constants(queue, sizeof(textured_quad_pc), &pc, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, app->pipelines[PIPELINE_TEXTURED_QUAD]);
    vulkan_command_draw(queue, 6);
}

void draw_cube_lines(app *app, v3 pos, v3 scale)
{
    vulkan_command_queue *queue     = app->render_queue;
    m4                    scale_m   = m4scale(scale.x, scale.y, scale.z);
    m4                    translate = m4translate(pos.x, pos.y, pos.z);
    m4                    model     = m4mul(translate, scale_m);

    cube_lines_pc pc = {
        .model = model,
    };

    vulkan_command_bind_pipeline(queue, app->pipelines[PIPELINE_CUBE_LINES]);
    vulkan_command_push_constants(queue, sizeof(pc), &pc, VK_SHADER_STAGE_VERTEX_BIT, app->pipelines[PIPELINE_CUBE_LINES]);
    vulkan_command_draw(queue, 24);
}

static bool resources_create(app *app)
{
    vulkan *vulkan = &app->vulkan;

    memory_arena resource_arena = memory_arena_create(MEGABYTES(20));

    usize jet_brains_size;
    u8   *jet_brains_data = SDL_LoadFile(path_abs("assets/fonts/jet-brains.ttf"), &jet_brains_size);
    if (!ttf_create(vulkan, jet_brains_size, jet_brains_data, "jet-brains", &app->jet_brains))
    {
        return false;
    }

    vulkan_sampler_create(vulkan, true, "linear_sampler", &app->linear_sampler);

    usize test_size;
    u8   *test_data = SDL_LoadFile(path_abs("assets/textures/watermelon.png"), &test_size);

    image_raw test_image;
    if (!png_create(&resource_arena, test_size, test_data, &test_image))
    {
        return false;
    }

    vulkan_image_create(vulkan, v2umake(test_image.width, test_image.height), VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_ASPECT_COLOR_BIT, "watermelon", &app->test_texture);
    vulkan_image_data_upload(vulkan, &app->test_texture, test_image.width * test_image.height * 4, test_image.data, v2umake(test_image.width, test_image.height), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, true);

    SDL_free(test_data);
    SDL_free(jet_brains_data);

    memory_arena_destroy(&resource_arena);

    return true;
}

SDL_AppResult SDL_AppInit(void **appstate, i32 argc, char *argv[])
{
    UNUSED(argc);
    UNUSED(argv);

    app *app = SDL_calloc(1, sizeof(*app));
    assert(app);

    *appstate = app;

    SDL_SetAppMetadata("SPH Simulation", "1.0", NULL);

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("[ENGINE] Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!window_create(&app->window, 2560, 1440))
    {
        SDL_Log("[ENGINE] Failed to create window.");
        return false;
    }

    if (!vulkan_create(app->window.handle, &app->vulkan, sizeof(global_ubo)))
    {
        SDL_Log("[ENGINE] Failed to init vulkan.");
        return false;
    }

    app->time      = time_create();
    app->camera    = camera_create();
    app->input     = input_create();
    app->ui_layout = ui_layout_create();

    if (!pipelines_create(&app->vulkan, app->pipelines, PIPELINE_COUNT))
    {
        return false;
    }

    if (!resources_create(app))
    {
        return false;
    }

    app->frame_arena = memory_arena_create(MEGABYTES(12));

    app->render_bounding_box = true;
    app->bounding_box        = cubemake(v3zero(), v3make(200, 200, 200));
    app->particle_radius     = 1.0f;
    app->simulation_speed    = 1.0f;

    if (!simulation_create(app, &app->simulation))
    {
        SDL_Log("[ENGINE] Failed to initialize simulation.");
        return SDL_APP_FAILURE;
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    app *app = appstate;
    assert(app);

    input_update(&app->input, event);

    if (event->type == SDL_EVENT_WINDOW_RESIZED)
    {
        window_resize(&app->window);
        vulkan_resize(&app->vulkan, app->window.width, app->window.height);
    }

    if (event->type == SDL_EVENT_QUIT)
    {
        return SDL_APP_SUCCESS;
    }
    return SDL_APP_CONTINUE;
}

static void global_ubo_update(app *app)
{
    global_ubo *ubo = vulkan_bindless_ubo_get(&app->vulkan.bindless);

    ubo->view = camera_view(&app->camera);

    f32 aspect_ratio = (f32)(MAX(app->window.width, app->ui_layout.width + 1) - app->ui_layout.width) / (f32)app->window.height;
    ubo->perspective = m4perspective(TO_RADIANS(87.0f), aspect_ratio, 0.1f, 1500.0f);

    ubo->orthographic = m4orthographic(0, app->window.width, app->window.height, 0, -1.0, 1.0f);
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
    app *app = appstate;
    assert(app);

    memory_arena_reset(&app->frame_arena);
    time_update(&app->time);
    camera_update(&app->camera, &app->window, &app->input, app->time.delta);
    global_ubo_update(app);

    window *window = &app->window;
    vulkan *vulkan = &app->vulkan;
    input  *input  = &app->input;

    app->render_queue           = vulkan_command_begin(&app->frame_arena);
    vulkan_command_queue *queue = app->render_queue;
    vulkan_command_set_present(queue);

    if (!app->paused)
    {
        const f32 simulation_dt = 1.0f / 60.0f;
        f32       delta         = app->time.delta;

        u32 iteration_count = 0;
        while (delta > 0.0f)
        {
            simulation_update(app, &app->simulation, simulation_dt * app->simulation_speed);
            delta -= simulation_dt;

            vulkan_command_barrier(queue, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_WRITE_BIT);

            // NOTE: 1 only for developing
            if (iteration_count++ > 1)
            {
                break;
            }
        }
    }

    vulkan_command_begin_rendering(queue);
    vulkan_command_set_viewport(queue, 0, 0, MAX(window->width, app->ui_layout.width + 1) - app->ui_layout.width, window->height);

    vulkan_command_label_begin(queue, "render cube", RED);
    if (app->render_bounding_box)
    {
        draw_cube_lines(app, app->bounding_box.pos, app->bounding_box.size);
    }
    vulkan_command_label_end(queue);

    simulation_draw(app, &app->simulation);

    vulkan_command_set_viewport(queue, 0, 0, window->width, window->height);
    ui_update(input);

    vulkan_command_label_begin(queue, "render_text", GREEN);
    draw_text(app, &app->jet_brains, v2make(0, 0), 24, "Frametime: %.4fms", app->time.smooth_delta * 1000.0f);
    vulkan_command_label_end(queue);

    ui_layout_calculate(app);

    if (app->paused_pressed)
    {
        app->paused = !app->paused;
    }
    if (app->reset_pressed)
    {
        simulation_destroy(app, &app->simulation);
        simulation_create(app, &app->simulation);
    }

    vulkan_command_label_begin(queue, "ui", RED);
    ui_draw(app);
    vulkan_command_label_end(queue);

    vulkan_command_end_rendering(queue);

    vulkan_draw(vulkan, app->window.width, app->window.height, queue);
    input_update(&app->input, NULL);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    UNUSED(result);

    app *app = appstate;
    assert(app);

    vulkan *vulkan = &app->vulkan;
    vkDeviceWaitIdle(vulkan->device);

    memory_arena_destroy(&app->frame_arena);
    simulation_destroy(app, &app->simulation);
    ttf_destroy(vulkan, &app->jet_brains);

    vulkan_object_destroy(vulkan, sizeof(app->test_texture), &app->test_texture, (vulkan_destroy_func)vulkan_image_destroy);
    vulkan_object_destroy(vulkan, sizeof(app->linear_sampler), &app->linear_sampler, (vulkan_destroy_func)vulkan_sampler_destroy);

    vulkan_destroy(vulkan);
    window_destroy(&app->window);

    SDL_free(app);
}
