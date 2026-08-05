
import utils;

struct PushConstant
{
	SpatialLookupEntry *spatial_lookup;
	uint *histograms;	
};

// NOTE: 8 bit number possiblities
static const uint bin_count = 256;

[shader("compute")]
[numthreads(256, 1, 1)]
void computeMain(uint index: SV_DispatchThreadID, uint local_index : SV_GroupThreadID)
{
	if (local_index < bin_count)
	{
		
	}	
}
