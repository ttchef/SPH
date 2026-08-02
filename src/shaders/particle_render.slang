
import utils;

struct PushConstant
{
	float4 *positions;
	uint particle_count;
};

[[vk::push_constant]]
PushConstant pc;

struct VSOutput
{
	float4 position : SV_Position;
	uint index;
};

[shader("vertex")]
VSOutput vertexMain(uint index: SV_VulkanVertexID)
{
	VSOutput output;

	float4 pos = pc.positions[index / 6];

	const float2 offsets[6] = {
		float2(-1.0, -1.0), float2(1.0, -1.0),
		float2(-1.0, 1.0), float2(-1.0, 1.0),
		float2(1.0, -1.0), float2(1.0, 1.0),
	};

	float2 offset = offsets[index % 6];

	float4 view_pos = mul(ubo.view, pos);
	view_pos.xy += offset;

	output.position = mul(ubo.perspective, float4(view_pos));
	output.index = index;

	return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
	float r = float(input.index % 20) / 20.0;
	float g = float((input.index / 20) % 20) / 20.0;
	float b = float(input.index / (20 * 20)) / 20.0;
	return float4(gamma_correct(float3(r, g, b)), 1.0);
}
