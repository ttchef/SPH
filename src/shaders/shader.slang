
static float2 vertices[6] = {
	float2(-1.0, -1.0),	
	float2(-1.0, 1.0),	
	float2(1.0, 1.0),	

	float2(-1.0, -1.0),	
	float2(1.0, -1.0),	
	float2(1.0, 1.0),	
};

static float3 colors[6] = {
	float3(1.0, 0.0, 0.0),	
	float3(0.0, 1.0, 0.0),	
	float3(0.0, 0.0, 1.0),	
	float3(1.0, 0.0, 0.0),	
	float3(0.0, 1.0, 0.0),	
	float3(0.0, 0.0, 1.0),	
};

struct VSOutput
{
	float4 position : SV_Position;
	float3 color;	
};

[shader("vertex")]
VSOutput vertexMain(uint index : SV_VulkanVertexID)
{
	VSOutput output;

	output.position = float4(vertices[index], 0.0, 1.0);
	output.color = colors[index];

	return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
	return float4(input.color, 1.0);
}
