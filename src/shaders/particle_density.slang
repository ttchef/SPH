
import utils;

struct PushConstant
{
	float4 *densities;
	float4 *positions_read;
	float4 *padding;
	uint particle_count;
};

[[vk::push_constant]]
PushConstant pc;

float W_poly6(float4 p_i, float4 p_j)
{
	float4 delta = p_i - p_j;
	float d = dot(delta, delta);

	if (d > SMOOTHING_RADIUS * SMOOTHING_RADIUS)
	{
		return 0.0;
	}
	d = sqrt(d);

	const float normalization = 315.0 / (64.0 * PI * pow(SMOOTHING_RADIUS, 9));
	float shape_func = pow((SMOOTHING_RADIUS * SMOOTHING_RADIUS) - (d * d), 3);

	return normalization * shape_func;
}

[shader("compute")]
[numthreads(256, 1, 1)]
void computeMain(uint index: SV_DispatchThreadID)
{
	float density = 0.0;

	float4 p_i = pc.positions_read[index];
	for (uint i = 0; i < pc.particle_count; i++)
	{
		float4 p_j = pc.positions_read[i];
		density += W_poly6(p_i, p_j);
	}

	pc.densities[index] = density;
}

