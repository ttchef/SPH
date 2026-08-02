
import utils;

struct PushConstant
{
	float4 *positions_read;
	float4 *positions_write;
	float4 *velocities_read;
	float4 *velocities_write;
	float4 box_pos;
	float4 box_size;
	float dt;
	float first_run;
};

[[vk::push_constant]]
PushConstant pc;

float4 W_spiky_gradient(float4 p_i, float4 p_j)
{
	float4 delta = p_i - p_j;
	float d = dot(delta, delta);

	if (d <= 0.0001 || d > SMOOTHING_RADIUS * SMOOTHING_RADIUS)
	{
		return float4(0).xxxx; 
	}
	d = sqrt(d);

	const float normalization = -45.0 / (PI * pow(SMOOTHING_RADIUS, 6));
	float shape_func = (SMOOTHING_RADIUS - d) / d;

	return delta * normalization * shape_func;
}

float pressure(float density)
{
	const float k = 500.0;

	float term = pow(density / TARGET_DENSITY, 7);
	float pressure = k * (term - 1);

	return pressure;
}

[shader("compute")]
[numthreads(256, 1, 1)]
void computeMain(uint index: SV_DispatchThreadID)
{
	float4 pos = pc.positions_read[index];
	float4 vel = pc.velocities_read[index];

	float4 acc = float4(0, -3, 0, 0);

	if (pc.first_run == 1.0)
	{
		vel += acc * (pc.dt / 2.0);
	}
	else
	{
		vel += acc * pc.dt;
	}

	pos += vel * pc.dt;	

	if (pos.y < pc.box_pos.y - pc.box_size.y * 0.5)
	{
		vel.y = -vel.y * 0.9;
		pos.y = pc.box_pos.y - pc.box_size.y * 0.5;
	}

	pc.positions_write[index] = pos;
	pc.velocities_write[index] = vel;
}
