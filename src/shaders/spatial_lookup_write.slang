
struct SimulationUbo
{
	uint particle_count;
	float target_density;
	float visocity_coeff;
	float smoothing_radius;
	float pressure_multiplier;	
};

[[vk::binding(0, 1)]]
ConstantBuffer<SimulationUbo> scene_ubo;

struct SpatialLookupEntry
{
	uint particle_index;
	uint cell_key;	
};

struct PushConstant
{
	SpatialLookupEntry *spatial_lookup;
	uint *start_indices;
	float4 *positions;
}

[[vk::push_constant]]
PushConstant pc;

static const uint U32_MAX = (~0u);

int3 position_to_cell(float4 pos)
{
	return int3(int(pos.x / scene_ubo.smoothing_radius), int(pos.y / scene_ubo.smoothing_radius), int(pos.z / scene_ubo.smoothing_radius));
}

uint cell_hash(int3 cell)
{
	uint a = uint(cell.x) * 15824;
    uint b = uint(cell.y) * 9737333;
    uint c = uint(cell.z) * 74771;
    return (a + b + c) % scene_ubo.particle_count;   
}

[shader("compute")]
[numthreads(64, 1, 1)]
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
