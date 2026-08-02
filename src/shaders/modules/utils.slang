
module utils;

public static const float PI = 3.1415926538;

// NOTE: Smoothing radius
public static const float SMOOTHING_RADIUS = 15.0;
public static const float TARGET_DENSITY = 0.008;
public static const float VISCOSITY_COEFF = 5000.0;

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

