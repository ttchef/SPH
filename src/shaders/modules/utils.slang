
module utils;

public struct GlobalUBO
{
    public float4x4 view;
    public float4x4 perspective;
    public float4x4 orthographic;
};

[[vk::binding(0, 0)]]
public ConstantBuffer<GlobalUBO> ubo;

[[vk::binding(1, 0)]]
public Texture2D<float4> textures[256];

[[vk::binding(2, 0)]]
public SamplerState samplers[256];

public float3 gamma_correct(float3 color)
{
	return pow(color, float3(1.0 / 2.2));	
}

