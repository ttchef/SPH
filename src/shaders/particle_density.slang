
import utils;

struct PushConstant
{
    float              *densities;
    float              *pressures;
    float4             *positions_read;
    SpatialLookupEntry *spatial_lookup;
    uint               *start_indices;
};

[[vk::push_constant]]
PushConstant pc;

float W_poly6(float4 p_i, float4 p_j)
{
    float4 delta = p_i - p_j;
    float  d     = dot(delta, delta);

    if (d > scene_ubo.smoothing_radius * scene_ubo.smoothing_radius)
    {
        return 0.0;
    }

    float shape_func = pow((scene_ubo.smoothing_radius * scene_ubo.smoothing_radius) - d, 3);

    return scene_ubo.poly6_normalization * shape_func;
}

float calc_pressure(float density)
{
    const float k = scene_ubo.pressure_multiplier;

    float t        = density / scene_ubo.target_density;
    t              = t * t * t * t * t * t * t;
    float pressure = k * (t - 1);

    return pressure;
}

[shader("compute")]
    [numthreads(256, 1, 1)] void
    computeMain(uint index : SV_DispatchThreadID)
{
    if (index >= scene_ubo.particle_count)
    {
        return;
    }

    float  density = 0.0;
    float4 p_i     = pc.positions_read[index];
    int3   cell    = position_to_cell(p_i);

    uint count = neighbour_offsets.getCount();
    for (uint n = 0; n < count; n++)
    {
        uint key   = cell_hash(cell + neighbour_offsets[n]);
        uint start = pc.start_indices[key];

        if (start == U32_MAX)
        {
            continue;
        }

        for (uint i = start; i < scene_ubo.particle_count && pc.spatial_lookup[i].cell_key == key; i++)
        {
            float4 p_j = pc.positions_read[i];
            density += W_poly6(p_i, p_j);
        }
    }

    pc.densities[index] = density;
    pc.pressures[index] = calc_pressure(density) / (density * density);
}
