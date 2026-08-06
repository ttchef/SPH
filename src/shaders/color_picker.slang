

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

    float2 uv[6] = {
        float2(0.0f, 1.0f),
        float2(0.0f, 0.0f),
        float2(1.0f, 0.0f),

        float2(0.0f, 1.0f),
        float2(1.0f, 1.0f),
        float2(1.0f, 0.0f),
    };

    output.position = mul(ubo.orthographic, mul(pc.model, float4(vertices[index], 0.0, 1.0)));
    output.uv       = uv[index];

    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
    float4 color = float4(input.uv, 0.0, 1.0);
    return float4(gamma_correct(color.xyz), color.w);
}
