
#pragma once

#include <math/types.h>
#include <sph/types.h>
#include <types.h>
#include <vk/buffer.h>
#include <vk/types.h>

#define PING_PONG_COUNT 2

typedef struct
{
    u32 particle_count;
    f32 target_density;
    f32 viscosity_coeff;
    f32 smoothing_radius;
    f32 pressure_multiplier;
} simulation_ubo;

typedef struct
{
    vulkan_bindless_scene_ubo scene;
    simulation_ubo ubo_data;
    
    vulkan_buffer densities;
    vulkan_buffer velocities[PING_PONG_COUNT];
    vulkan_buffer positions[PING_PONG_COUNT];

    bool first_loop;
    u32 read_index;
    u32 write_index;
    f32 elapsed_time;

    bool initialized;
} simulation;

bool simulation_create(app *app, simulation *out_simulation);

void simulation_ubo_update(app *app, simulation *simulation);

void simulation_update(app *app, simulation *simulation, f32 dt);

void simulation_draw(app *app, simulation *simulation);

void simulation_destroy(app *app, simulation *simulation);
