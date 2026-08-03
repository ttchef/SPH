
#pragma once

#include <math/types.h>
#include <sph/types.h>
#include <types.h>
#include <vk/buffer.h>
#include <vk/types.h>

#define PING_PONG_COUNT 2

typedef struct
{
    vulkan_buffer densities;
    vulkan_buffer velocities[PING_PONG_COUNT];
    vulkan_buffer positions[PING_PONG_COUNT];

    bool first_loop;
    u32 read_index;
    u32 write_index;
    f32 elapsed_time;
} simulation;

bool simulation_create(app *app, vulkan *vulkan, simulation *out_simulation);

void simulation_update(app *app, vulkan *vulkan, simulation *simulation, f32 dt);

// NOTE: app is only passed in for testing atm it will get removed later
void simulation_draw(app *app, vulkan *vulkan, simulation *simulation);

void simulation_destroy(vulkan *vulkan, simulation *simulation);
