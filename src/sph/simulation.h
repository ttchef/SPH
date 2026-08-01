
#include <types.h>
#include <vk/buffer.h>

typedef struct 
{
	vulkan_buffer densities;
	vulkan_buffer velocities;
	vulkan_buffer positions;
} simulation;

bool simulation_create(simulation *simulation);

void simulation_update(simulation *simulation);

void simulation_destroy(simulation *simulation);

