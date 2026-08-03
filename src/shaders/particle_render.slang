
import utils;

struct PushConstant
{
	float4 *positions;
	float4 *velocities;
	uint particle_count;
	float particle_radius;
};

[[vk::push_constant]]
PushConstant pc;

struct VSOutput
{
	float4 position : SV_Position;
	float2 uv;
	float4 view_pos;
	uint index;	
};

[shader("vertex")]
VSOutput vertexMain(uint index: SV_VulkanVertexID)
{
	VSOutput output;

	uint particle_index = index / 6;
	float4 pos = pc.positions[particle_index];

	const float2 offsets[6] = {
		float2(-1.0, -1.0), float2(1.0, -1.0),
		float2(-1.0, 1.0), float2(-1.0, 1.0),
		float2(1.0, -1.0), float2(1.0, 1.0),
	};

	float2 offset = offsets[index % 6];

	float4 view_pos = mul(ubo.view, pos);
	view_pos.xy += offset * pc.particle_radius;

	output.position = mul(ubo.perspective, float4(view_pos));
	output.uv = offset;
	output.view_pos = view_pos;
	output.index = particle_index;

	return output;
}

struct FSOutput
{
	float4 color : SV_Target;
	float depth : SV_Depth;	
};

[shader("fragment")]
FSOutput fragmentMain(VSOutput input)
{
	FSOutput output;

	float dist_sq = dot(input.uv, input.uv);
	if (dist_sq > 1.0)
	{
		discard;
	}

	float z = sqrt(1.0 - dist_sq);
	float3 normal = normalize(float3(input.uv, z));

	const float3 light_dir = normalize(float3(0.5, 0.5, 1.0));
	float diffuse = max(dot(normal, light_dir), 0.2);

	const float max_velo = 50.0;
	float speed = length(pc.velocities[input.index].xyz);
	float t = min(speed / max_velo, 1.0);
	float3 color = float3(t, 0.0, 1.0 - t);

	output.color = float4(gamma_correct(color * diffuse), 1.0);

	float4 pos = input.view_pos;
	pos.z += z * pc.particle_radius;

	float4 clip_pos = mul(ubo.perspective, pos);
	output.depth = clip_pos.z / clip_pos.w;

	return output;
}
