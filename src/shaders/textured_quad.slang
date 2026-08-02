
import utils;

struct PushConstant
{
    float4x4 model;
    uint     texture;
    uint     sampler;
    float    roundness;
    float    aspect_ratio;
    float2   uv_min;
    float2   uv_max;
    float4   color;
};

[[vk::push_constant]]
PushConstant pc;

struct VSOutput
{
    float4 position : SV_Position;
    float2 uv;
};

[shader("vertex")]
VSOutput vertexMain(uint index: SV_VulkanVertexID)
{
    VSOutput output;

    float2 vertices[6] = {
        float2(-0.5, -0.5),
        float2(-0.5, 0.5),
        float2(0.5, 0.5),

        float2(-0.5, -0.5),
        float2(0.5, -0.5),
        float2(0.5, 0.5),
    };

    // NOTE: Y inverted uv's for textures
    float2 uv[6] = {
        float2(pc.uv_min.x, pc.uv_min.y),
        float2(pc.uv_min.x, pc.uv_max.y),
        float2(pc.uv_max.x, pc.uv_max.y),

        float2(pc.uv_min.x, pc.uv_min.y),
        float2(pc.uv_max.x, pc.uv_min.y),
        float2(pc.uv_max.x, pc.uv_max.y),
    };

    output.position = mul(ubo.orthographic, mul(pc.model, float4(vertices[index], 0.0, 1.0)));
    output.uv       = uv[index];

    return output;
}

float sdfRoundedBox(float2 p, float2 b, float r)
{
    float2 q = abs(p) - b + r;
    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - r;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
    float4 color = pc.color;
    
    if (pc.texture != 0)
    {
        color = textures[pc.texture].Sample(samplers[pc.sampler], input.uv);
    }
    else if (pc.roundness != 0.0)
    {
        float2 remapped_uv = (input.uv * 2) - 1;
        remapped_uv.x *= pc.aspect_ratio;
        float sdf = sdfRoundedBox(remapped_uv, float2(1.0 * pc.aspect_ratio, 1.0), pc.roundness);

        float pd = fwidth(sdf);
        float mask = 1.0 - saturate(sdf / pd);
        
        color = mask * pc.color;
    }

    return float4(gamma_correct(color.xyz), color.w);
}
