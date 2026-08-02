
import utils;

struct PushConstant
{
	float4x4 model;	
};

[[vk::push_constant]]
PushConstant pc;

struct VSOutput
{
	float4 position : SV_Position;	
};

[[shader("vertex")]]
VSOutput vertexMain(uint index : SV_VulkanVertexID)
{
	VSOutput output;

	float3 cube_line_vertices[24] = {
	    float3(-0.5, -0.5, 0.5), float3(-0.5, 0.5, 0.5),
	    float3(-0.5, 0.5, 0.5), float3(0.5, 0.5, 0.5),
	    float3(0.5, 0.5, 0.5), float3(0.5, -0.5, 0.5),
	    float3(0.5, -0.5, 0.5), float3(-0.5, -0.5, 0.5),

	    float3(-0.5, -0.5, -0.5), float3(-0.5, 0.5, -0.5),
	    float3(-0.5, 0.5, -0.5), float3(0.5, 0.5, -0.5),
	    float3(0.5, 0.5, -0.5), float3(0.5, -0.5, -0.5),
	    float3(0.5, -0.5, -0.5), float3(-0.5, -0.5, -0.5),

	    float3(-0.5, -0.5, 0.5), float3(-0.5, -0.5, -0.5),
	    float3(-0.5, 0.5, 0.5), float3(-0.5, 0.5, -0.5),
	    float3(0.5, 0.5, 0.5), float3(0.5, 0.5, -0.5),
	    float3(0.5, -0.5, 0.5), float3(0.5, -0.5, -0.5) 
	};

	float4x4 view_proj = mul(ubo.perspective, ubo.view);
	output.position = mul(view_proj, mul(pc.model, float4(cube_line_vertices[index], 1.0)));

	return output;
}

[[shader("fragment")]]
float4 fragmentMain(VSOutput input)
{
	return float4(gamma_correct(float3(1.0, 0.0, 0.0)), 1.0);
}
