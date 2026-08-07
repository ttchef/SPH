
module utils;

public static const float PI = 3.1415926538;
public static const uint U32_MAX = (~0u);

public struct GlobalUBO
{
    public float4x4 view;
    public float4x4 perspective;
    public float4x4 orthographic;
};

[[vk::binding(0, 0)]]
public ConstantBuffer<GlobalUBO> ubo;

public struct SimulationUbo
{
	public uint particle_count;
	public float target_density;
	public float visocity_coeff;
	public float smoothing_radius;
	public float pressure_multiplier;	
};

[[vk::binding(0, 1)]]
public ConstantBuffer<SimulationUbo> scene_ubo;

[[vk::binding(1, 0)]]
public Texture2D<float4> textures[256];

[[vk::binding(2, 0)]]
public SamplerState samplers[256];

public static const int3 neighbour_offsets[] = {
    int3(-1,-1, 0), int3(0,-1, 0), int3(1,-1, 0),
    int3(-1, 0, 0), int3(0, 0, 0), int3(1, 0, 0),
    int3(-1, 1, 0), int3(0, 1, 0), int3(1, 1, 0),
    
    int3(-1,-1, 1), int3(0,-1, 1), int3(1,-1, 1),
    int3(-1, 0, 1), int3(0, 0, 1), int3(1, 0, 1),
    int3(-1, 1, 1), int3(0, 1, 1), int3(1, 1, 1),
    
    int3(-1,-1, -1), int3(0,-1, -1), int3(1,-1, -1),
    int3(-1, 0, -1), int3(0, 0, -1), int3(1, 0, -1),
    int3(-1, 1, -1), int3(0, 1, -1), int3(1, 1, -1)
};

public struct SpatialLookupEntry
{
	public uint particle_index;
	public uint cell_key;	
};

public float3 gamma_correct(float3 color)
{
	return pow(color, float3(1.0 / 2.2));	
}

public int3 position_to_cell(float4 pos)
{
	return int3(int(pos.x / scene_ubo.smoothing_radius), int(pos.y / scene_ubo.smoothing_radius), int(pos.z / scene_ubo.smoothing_radius));
}

public uint cell_hash(int3 cell)
{
	uint a = uint(cell.x) * 15824;
    uint b = uint(cell.y) * 9737333;
    uint c = uint(cell.z) * 74771;
    return (a + b + c) % scene_ubo.particle_count;   
}


public float3 hsvToRgb(float hue, float saturation, float value)
{
    float chroma = value * saturation;
    float h1 = hue / 60.0;
    float X = chroma * (1.0 - abs(h1 % 2 - 1));

    float3 color;

    if (h1 < 1.0)
    {
        color = float3(chroma, X, 0.0);
    }
    else if (h1 < 2.0)
    {
        color = float3(X, chroma, 0.0);
    }
    else if (h1 < 3.0)
    {
        color = float3(0.0, chroma, X);
    }
    else if (h1 < 4.0)
    {
        color = float3(0.0, X, chroma); 
    }
    else if (h1 < 5.0)
    {
        color = float3(X, 0.0, chroma);
    }
    else if (h1 < 6.0)
    {
        color = float3(chroma, 0.0, X);
    }
    else
    {
        // NOTE: Easy to debug
        color = float3(1.0, 1.0, 1.0);
    }

    float m = value - chroma;
    return color + m;
}

