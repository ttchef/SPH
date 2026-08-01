
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

struct VSOutput
{
	float4 position : SV_Position;	
};

[shader("vertex")]
VSOutput vertexMain(uint index: SV_VulkanVertexID)
{
	VSOutput output;

	output.position = float4(1.0, 1.0, 1.0, 1.0);

	return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
	return float4(1.0, 0.0, 0.0, 1.0);
}
