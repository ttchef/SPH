
import utils;

struct PushConstant
{
	float4 *positions_read;
	float4 *positions_write;
	float4 *velocities_read;
	float4 *velocities_write;
	float *densities;
	float *padding;
	float4 box_pos;
	float4 box_size;
	float dt;
	float first_run;
	uint particle_count;
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
	float shape_func = (SMOOTHING_RADIUS - d) * (SMOOTHING_RADIUS - d) / d;

	return delta * normalization * shape_func;
}

float calc_pressure(float density)
{
	const float k = 500.0;

	float term = pow(density / TARGET_DENSITY, 7);
	float pressure = k * (term - 1);

	return pressure;
}

float4 random_unit_vector(uint seed)
{
	float a = fract(sin(float(seed) * 12.9898) * 43758.5453);
	float b = fract(sin(float(seed) * 78.2330) * 12543.548);

	float theta  = a * 2.0 * PI;
	float z = b * 2.0 - 1.0;
	float r = sqrt(1.0 - z * z);

	return float4(r * cos(theta), r * sin(theta), z, 0.0);
}

float4 calc_force(uint particle_index)
{
	float4 pressure = float4(0).xxxx;
	float4 viscosity = float4(0).xxxx;

	float4 pos_i = pc.positions_read[particle_index];
	float4 vel_i = pc.velocities_read[particle_index];
	
	float density_i = pc.densities[particle_index];
	float term1 = calc_pressure(density_i) / (density_i * density_i);

	for (uint i = 0; i < pc.particle_count; i++)
	{
		if (i == particle_index)
		{
			continue;
		}

		float4 pos_j = pc.positions_read[i];

		float d = distance(pos_i, pos_j);
		if (d <= 0.0001)
		{
			float4 dir = random_unit_vector(particle_index);
			pressure -= dir * 500.0;
			continue;
		}
		if (d > SMOOTHING_RADIUS)
		{
			continue;
		}

		float4 gradient = W_spiky_gradient(pos_i, pos_j);

		float density_j = pc.densities[i];
		float term2 = calc_pressure(density_j) / (density_j * density_j);

		pressure += gradient * (term1 + term2);

		float4 v_ij = vel_i - pc.velocities_read[i];
		float4 x_ij = pos_i - pc.positions_read[i];

		float numerator = dot(x_ij, gradient);
		float denominator = dot(x_ij, x_ij) + 0.01 * SMOOTHING_RADIUS * SMOOTHING_RADIUS;

		viscosity += v_ij * (1.0 / density_j) * (numerator / denominator) * VISCOSITY_COEFF; 
	}

	pressure = -pressure;
	viscosity *= 2;

	return pressure + viscosity;
}

[shader("compute")]
[numthreads(256, 1, 1)]
void computeMain(uint index: SV_DispatchThreadID)
{
	if (index >= pc.particle_count)
	{
		return;
	}

	float4 pos = pc.positions_read[index];
	float4 vel = pc.velocities_read[index];

	float4 acc = float4(0, -4, 0, 0);
	acc += calc_force(index);

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
	if (pos.y > pc.box_pos.y + pc.box_size.y * 0.5)
	{
		vel.y = -vel.y * 0.9;
		pos.y = pc.box_pos.y + pc.box_size.y * 0.5;
	}
	if (pos.x < pc.box_pos.x - pc.box_size.x * 0.5)
	{
		vel.x = -vel.x * 0.9;
		pos.x = pc.box_pos.x - pc.box_size.x * 0.5;
	}
	if (pos.x > pc.box_pos.x + pc.box_size.x * 0.5)
	{
		vel.x = -vel.x * 0.9;
		pos.x = pc.box_pos.x + pc.box_size.x * 0.5;
	}
	if (pos.z < pc.box_pos.z - pc.box_size.z * 0.5)
	{
		vel.z = -vel.z * 0.9;
		pos.z = pc.box_pos.z - pc.box_size.z * 0.5;
	}
	if (pos.z > pc.box_pos.z + pc.box_size.z * 0.5)
	{
		vel.z = -vel.z * 0.9;
		pos.z = pc.box_pos.z + pc.box_size.z * 0.5;
	}

	pc.positions_write[index] = pos;
	pc.velocities_write[index] = vel;
}
