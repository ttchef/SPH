
import utils;

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
	float3 dir;
};

[shader("vertex")]
VSOutput vertexMain(uint index: SV_VulkanVertexID)
{
	VSOutput output;

	const float3 vertices[36] = {
	    float3(-0.5, -0.5,  0.5),
	    float3( 0.5, -0.5,  0.5),
	    float3( 0.5,  0.5,  0.5),
	    float3(-0.5, -0.5,  0.5),
	    float3( 0.5,  0.5,  0.5),
	    float3(-0.5,  0.5,  0.5),

	    float3( 0.5, -0.5, -0.5),
	    float3(-0.5, -0.5, -0.5),
	    float3(-0.5,  0.5, -0.5),
	    float3( 0.5, -0.5, -0.5),
	    float3(-0.5,  0.5, -0.5),
	    float3( 0.5,  0.5, -0.5),

	    float3(-0.5, -0.5, -0.5),
	    float3(-0.5, -0.5,  0.5),
	    float3(-0.5,  0.5,  0.5),
	    float3(-0.5, -0.5, -0.5),
	    float3(-0.5,  0.5,  0.5),
	    float3(-0.5,  0.5, -0.5),

	    float3( 0.5, -0.5,  0.5),
	    float3( 0.5, -0.5, -0.5),
	    float3( 0.5,  0.5, -0.5),
	    float3( 0.5, -0.5,  0.5),
	    float3( 0.5,  0.5, -0.5),
	    float3( 0.5,  0.5,  0.5),

	    float3(-0.5,  0.5,  0.5),
	    float3( 0.5,  0.5,  0.5),
	    float3( 0.5,  0.5, -0.5),
	    float3(-0.5,  0.5,  0.5),
	    float3( 0.5,  0.5, -0.5),
	    float3(-0.5,  0.5, -0.5),

	    float3(-0.5, -0.5, -0.5),
	    float3( 0.5, -0.5, -0.5),
	    float3( 0.5, -0.5,  0.5),
	    float3(-0.5, -0.5, -0.5),
	    float3( 0.5, -0.5,  0.5),
	    float3(-0.5, -0.5,  0.5)
	};

	// NOTE: Cancel rotation
	float4x4 view = float4x4(
	    float4(ubo.view[0].xyz, 0.0),
	    float4(ubo.view[1].xyz, 0.0),
	    float4(ubo.view[2].xyz, 0.0),
	    float4(0.0, 0.0, 0.0, 1.0)
	);

	float3 vertex = vertices[index];

	output.position = mul(ubo.perspective, mul(view, float4(vertex, 1.0))).xyww;
	output.dir = normalize(vertex);

	return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
	return cube_textures[pc.texture].Sample(samplers[pc.sampler], input.dir);
}
