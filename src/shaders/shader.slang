
Texture2D<float4> textures[256];
SamplerState samplers[256];

struct PushConstant
{
	uint texture;
	uint sampler;	
};

[[vk::push_constant]]
PushConstant pc;

struct VSOutput
{
	float4 position : SV_Position;
	float2 uv;
	float3 color;	
};

[shader("vertex")]
VSOutput vertexMain(uint index : SV_VulkanVertexID)
{
	VSOutput output;

	float2 vertices[6] = {
		float2(-1.0, -1.0),	
		float2(-1.0, 1.0),	
		float2(1.0, 1.0),	

		float2(-1.0, -1.0),	
		float2(1.0, -1.0),	
		float2(1.0, 1.0),	
	};

	float2 uv[6] = {
		float2(0.0, 1.0),
		float2(0.0, 0.0),
		float2(1.0, 0.0),

		float2(0.0, 1.0),
		float2(1.0, 1.0),
		float2(1.0, 0.0),	
	};

	float3 colors[6] = {
		float3(1.0, 0.0, 0.0),	
		float3(0.0, 1.0, 0.0),	
		float3(0.0, 0.0, 1.0),	
		float3(1.0, 0.0, 0.0),	
		float3(0.0, 1.0, 0.0),	
		float3(0.0, 0.0, 1.0),	
	};

	output.position = float4(vertices[index], 0.0, 1.0);
	output.uv = uv[index];
	output.color = colors[index];

	return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
	return textures[pc.texture].Sample(samplers[pc.sampler], input.uv);
}
