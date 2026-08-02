
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
