
import utils;

struct PushConstant
{
	float4 *densities_read;
	float4 *densities_write;
};

[[vk::push_constant]]
PushConstant pc;

[shader("compute")]
[numthreads(256, 1, 1)]
void computeMain(uint index: SV_DispatchThreadID)
{
	float4 density = pc.densities_read[index];

	pc.densities_write[index] = density;
}

