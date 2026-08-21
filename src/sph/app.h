
#pragma once

#include <sph/camera.h>
#include <sph/input.h>
#include <sph/memory.h>
#include <sph/pipelines.h>
#include <sph/simulation.h>
#include <sph/time.h>
#include <sph/ttf.h>
#include <sph/types.h>
#include <sph/ui_layout.h>
#include <sph/window.h>
#include <vk/context.h>

struct app
{
    window            window;
    input             input;
    camera            camera;
    time              time;
    vulkan            vulkan;
    ui_layout_context ui_layout;
    simulation        simulation;

    memory_arena          frame_arena;
    vulkan_command_queue *render_queue;
    vulkan_pipeline_id    pipelines[PIPELINE_COUNT];

    vulkan_sampler linear_sampler;
    vulkan_image   skybox;
    ttf_font       jet_brains;

    bool  reset_pressed;
    bool  paused;
    bool  first_time;
};

//
// NOTE: Helpers
//
f32 measure_text(ttf_font *font, u32 font_size, const char *format, ...);

void draw_text(app *app, ttf_font *font, v2 pos, u32 font_size, const char *format, ...);

void draw_quad(app *app, v2 pos, v2 scale, f32 roundness, color4 color, vulkan_image *image, vulkan_sampler *sampler);

void draw_gradient(app *app, v2 pos, v2 scale, f32 roundness, color4 a, color4 b, gradient_type type);

void draw_cube_lines(app *app, v3 pos, v3 scale);

void draw_skybox(app *app, vulkan_image skybox);
