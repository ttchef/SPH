
import utils;

struct PushConstant
{
	SpatialLookupEntry *spatial_lookup;
	uint *histograms;
	uint shift;
    uint workgroup_count;
    uint blocks_per_workgroup;
};

[[vk::push_constant]]
PushConstant pc;

// NOTE: 8 bit number possiblities
static const uint bin_count = 256;
groupshared Atomic<uint> histogram[bin_count];

[shader("compute")]
[numthreads(256, 1, 1)]
void computeMain(uint index: SV_DispatchThreadID, uint local_index : SV_GroupThreadID, uint group_id : SV_GroupID)
{
	if (local_index < bin_count)
	{
		histogram[local_index].store(0);
	}
	GroupMemoryBarrierWithGroupSync();

	for (uint i = 0; i < pc.blocks_per_workgroup; i++)
	{
		const uint element_id = group_id * pc.blocks_per_workgroup * WorkgroupSize().x + i * WorkgroupSize().x + local_index;
		if (element_id < scene_ubo.particle_count)
		{
			const uint bin = (pc.spatial_lookup[element_id].cell_key >> pc.shift) & (bin_count - 1);
			histogram[bin].add(1);
		}
	}
	GroupMemoryBarrierWithGroupSync();

	if (local_index < bin_count)
	{
		pc.histograms[bin_count * group_id + local_index] = histogram[local_index].load();
	}
}
