
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
    float4x4 model;
    uint     texture;
    uint     sampler;
    uint     rounded;
    uint     padding;
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

float sdfBox(float2 p, float2 b)
{
    float2 d = abs(p) - b;
    return length(max(d ,0.0)) + min(max(d.x, d.y), 0.0);
}

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
    if (pc.texture != 0)
    {
        return textures[pc.texture].Sample(samplers[pc.sampler], input.uv);
    }
    if (pc.rounded == 1)
    {
        return float4(input.uv, 0.0, 1.0);
    }
    return pc.color;
}
