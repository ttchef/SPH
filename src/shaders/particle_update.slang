
struct GlobalUBO
{
    float4x4 view;
    float4x4 perspective;
    float4x4 orthographic;
};

[[vk::binding(0, 0)]]
ConstantBuffer<GlobalUBO> ubo;

[[vk::binding(1, 0)]]
Texture2D<float4> textures[256];

[[vk::binding(2, 0)]]
SamplerState samplers[256];

struct PushConstant
{
	float4 *positions_read;
	float4 *positions_write;
	float4 *velocities_read;
	float4 *velocities_write;
	float4 box_pos;
	float4 box_size;
};

[[vk::push_constant]]
PushConstant pc;

[shader("compute")]
[numthreads(256, 1, 1)]
void computeMain(uint index: SV_DispatchThreadID)
{
	float4 pos = pc.positions_read[index];
	float4 new_pos = pos;
	float4 vel = pc.velocities_read[index];
	float4 new_vel = vel;

	new_vel.y -= 0.0001;
	new_pos += new_vel;	

	if (new_pos.y < pc.box_pos.y - pc.box_size.y * 0.5)
	{
		new_vel.y = -new_vel.y * 0.9;
		new_pos.y = pc.box_pos.y - pc.box_size.y * 0.5;
	}

	pc.positions_write[index] = new_pos;
	pc.velocities_write[index] = new_vel;
}
