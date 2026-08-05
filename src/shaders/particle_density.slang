
import utils;

struct PushConstant
{
	float *densities;
	float4 *positions_read;
	float4 *padding;
};

[[vk::push_constant]]
PushConstant pc;

float W_poly6(float4 p_i, float4 p_j)
{
	float4 delta = p_i - p_j;
	float d = dot(delta, delta);

	if (d > scene_ubo.smoothing_radius * scene_ubo.smoothing_radius)
	{
		return 0.0;
	}
	d = sqrt(d);

	const float normalization = 315.0 / (64.0 * PI * pow(scene_ubo.smoothing_radius, 9));
	float shape_func = pow((scene_ubo.smoothing_radius * scene_ubo.smoothing_radius) - (d * d), 3);

	return normalization * shape_func;
}

[shader("compute")]
[numthreads(256, 1, 1)]
void computeMain(uint index: SV_DispatchThreadID)
{
	if (index >= scene_ubo.particle_count)
	{
		return;
	}

	float density = 0.0;

	float4 p_i = pc.positions_read[index];
	for (uint i = 0; i < scene_ubo.particle_count; i++)
	{
		float4 p_j = pc.positions_read[i];
		density += W_poly6(p_i, p_j);
	}

	pc.densities[index] = density;
}

