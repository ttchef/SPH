
import utils;

struct PushConstant
{
	SpatialLookupEntry *spatial_lookup_read;
	SpatialLookupEntry *spatial_lookup_write;
	uint *histograms;
	uint shift;
    uint workgroup_count;
    uint blocks_per_workgroup;
};

[[vk::push_constant]]
PushConstant pc;

// NOTE: 8 bit number possiblities
static const uint bin_count = 256;
// TODO: Check if works on amd because of diffrent wave size
static const uint subgroup_size = 32;
static const uint workgroup_size = 256;

struct BinFlags
{
	Atomic<uint> flags[256 / subgroup_size];	
};

groupshared uint sums[bin_count / subgroup_size];
groupshared Atomic<uint> global_offsets[bin_count];
groupshared BinFlags bins[bin_count];

[shader("compute")]
[numthreads(workgroup_size, 1, 1)]
void computeMain(uint index: SV_DispatchThreadID, uint local_index : SV_GroupThreadID, uint group_id : SV_GroupID)
{
	uint local_histogram = 0;
	uint histogram_count = 0;
	
	if (local_index < bin_count)
	{
		for (uint i = 0; i < pc.workgroup_count; i++)
		{
			const uint t = pc.histograms[bin_count * i + local_index];
			local_histogram = (i == group_id) ? histogram_count : local_histogram;
			histogram_count += t;		
		}
	}

	const uint sum = WaveActiveSum(histogram_count);
	const uint prefix_sum = WavePrefixSum(histogram_count);

	if (WaveIsFirstLane() && WaveGetWaveIndex() < (bin_count / subgroup_size))
	{
		sums[WaveGetWaveIndex()] = sum;	
	}
	GroupMemoryBarrierWithGroupSync();

	if (local_index < bin_count)
	{
		uint sums_prefix_sum = 0;
		for (uint i = 0; i < WaveGetWaveIndex(); i++)
		{
			sums_prefix_sum += sums[i];
		}
		global_offsets[local_index].store(sums_prefix_sum + prefix_sum + local_histogram);
	}

	const uint flags_bin = local_index / subgroup_size;
	const uint flags_bit = 1 << (local_index % subgroup_size);

	for (uint i = 0; i < pc.blocks_per_workgroup; i++)
	{
		uint element_id = group_id * pc.blocks_per_workgroup * workgroup_size + i * workgroup_size + local_index;

		if (local_index < bin_count)
		{
			for (uint j = 0; j < workgroup_size / subgroup_size; j++)
			{
				bins[local_index].flags[j].store(0);
			}
		}
		GroupMemoryBarrierWithGroupSync();

		uint element_in = 0;
		uint bin_id = 0;
		uint bin_offset = 0;

		if (element_id < scene_ubo.particle_count)
		{
			element_in = pc.spatial_lookup_read[element_id].cell_key;
			bin_id = (element_in >> pc.shift) & (bin_count - 1);
			bin_offset = global_offsets[bin_id].load();

			bins[bin_id].flags[flags_bin].add(flags_bit);
		}
		GroupMemoryBarrierWithGroupSync();

		if (element_id < scene_ubo.particle_count)
		{
			uint prefix = 0;
			uint count = 0;

			for (uint j = 0; j < workgroup_size / subgroup_size; j++)
			{
				const uint bits = bins[bin_id].flags[j].load();
				const uint full_count = countbits(bits);
				const uint partial_count = countbits(bits & (flags_bit - 1));

				prefix += (j < flags_bin) ? full_count : 0u;
				prefix += (j == flags_bin) ? partial_count : 0u;

				count += full_count;
			}

			pc.spatial_lookup_write[bin_offset + prefix] = pc.spatial_lookup_read[element_id];
			if (prefix == count - 1)
			{
				global_offsets[bin_id].add(count);
			}
		}
		GroupMemoryBarrierWithGroupSync();
	}
}
