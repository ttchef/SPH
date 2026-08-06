
import utils;

struct PushConstant
{
	SpatialLookupEntry *spatial_lookup;
	uint *start_indices;
	float4 *positions;
}

[[vk::push_constant]]
PushConstant pc;

[shader("compute")]
[numthreads(256, 1, 1)]
void computeMain(uint index: SV_DispatchThreadID)
{
	if (index >= scene_ubo.particle_count)
	{
		return;
	}

	int3 cell = position_to_cell(pc.positions[index]);
	uint cell_key = cell_hash(cell);

	pc.spatial_lookup[index].particle_index = index;	
	pc.spatial_lookup[index].cell_key = cell_key;

	pc.start_indices[index] = U32_MAX;
}
