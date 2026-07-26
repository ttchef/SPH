
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
    float2   rect_pos;
    float2   rect_size;
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
        float2(pc.rect_pos.x, pc.rect_pos.y),
        float2(pc.rect_pos.x, pc.rect_size.y),
        float2(pc.rect_size.x, pc.rect_size.y),

        float2(pc.rect_pos.x, pc.rect_pos.y),
        float2(pc.rect_size.x, pc.rect_pos.y),
        float2(pc.rect_size.x, pc.rect_size.y),
    };
    
    output.position = mul(ubo.orthographic, mul(pc.model, float4(vertices[index], 0.0, 1.0)));
    output.uv       = uv[index];

    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
    return textures[pc.texture].Sample(samplers[pc.sampler], input.uv);
}
