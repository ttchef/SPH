
import utils;

struct PushConstant
{
	SpatialLookupEntry *spatial_lookup;
	uint *histograms;	
};

// NOTE: 8 bit number possiblities
static const uint bin_count = 256;

[shader("compute")]
void computeMain(uint index: SV_DispatchThreadID)
{
	
}
