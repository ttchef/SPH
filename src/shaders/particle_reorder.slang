
import utils;

struct PushConstant
{
	float4 *positions;
	float4 *velocities;
	SpatialLookupEntry *spatial_lookup;
	float4 *sorted_positions;
	float4 *sorted_velocities;	
};

[[vk::push_constant]]
PushConstant pc;

[shader("compute")]
[numthreads(256, 1, 1)]
void computeMain(uint index : SV_DispatchThreadID)
{
	if (index >= scene_ubo.particle_count)
	{
		return;
	}

	uint particle_index = pc.spatial_lookup[index].particle_index;
	pc.sorted_positions[index] = pc.positions[particle_index];
	pc.sorted_velocities[index] = pc.velocities[particle_index];
}
