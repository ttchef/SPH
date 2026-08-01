
#pragma once

#include <types.h>
#include <sph/types.h>
#include <vk/types.h>
#include <vk/buffer.h>
#include <math/types.h>

typedef struct 
{
	vulkan_buffer densities;
	vulkan_buffer velocities;
	vulkan_buffer positions;

	f32 elapsed_time;
} simulation;

bool simulation_create(vulkan *vulkan, simulation *out_simulation);

void simulation_update(vulkan *vulkan, simulation *simulation);

// NOTE: app is only passed in for testing atm it will get removed later
void simulation_draw(app *app, vulkan *vulkan, simulation *simulation);

void simulation_destroy(vulkan *vulkan, simulation *simulation);

