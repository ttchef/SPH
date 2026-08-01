
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
	float4 *positions;
	float4 *velocities;
	float4 box_pos;
	float4 box_size;
};

[[vk::push_constant]]
PushConstant pc;

[shader("compute")]
[numthreads(256, 1, 1)]
void computeMain(uint index: SV_DispatchThreadID)
{
	pc.positions[index].y -= 0.01;

	// pos.y -= 100;

	// if (pos.y < pc.box_pos.y - pc.box_size.y * 0.5)
	// {
		// pos.y = pc.box_pos.y - pc.box_size.y * 0.5;
	// }	

}
