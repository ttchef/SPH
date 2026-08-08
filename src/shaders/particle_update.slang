
import utils;

struct PushConstant
{
    float4             *positions_read;
    float4             *positions_write;
    float4             *velocities_read;
    float4             *velocities_write;
    float              *densities;
    SpatialLookupEntry *spatial_lookup;
    uint               *start_indices;
    float              *pressures;
    float4              box_min;
    float4              box_max;
    float               dt;
    float               first_run;
};

[[vk::push_constant]]
PushConstant pc;

float4 W_spiky_gradient(float4 delta, float d)
{
    float shape_func = (scene_ubo.smoothing_radius - d) * (scene_ubo.smoothing_radius - d) / d;
    return delta * scene_ubo.spiky_gradient_normalization * shape_func;
}

float4 random_unit_vector(uint seed)
{
    float a = fract(sin(float(seed) * 12.9898) * 43758.5453);
    float b = fract(sin(float(seed) * 78.2330) * 12543.548);

    float theta = a * 2.0 * PI;
    float z     = b * 2.0 - 1.0;
    float r     = sqrt(1.0 - z * z);

    return float4(r * cos(theta), r * sin(theta), z, 0.0);
}

float4 calc_force(uint particle_index)
{
    float4 pressure  = float4(0).xxxx;
    float4 viscosity = float4(0).xxxx;

    float4 pos_i = pc.positions_read[particle_index];
    float4 vel_i = pc.velocities_read[particle_index];

    int3 cell = position_to_cell(pos_i);

    float term1     = pc.pressures[particle_index];

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
            if (i == particle_index)
            {
                continue;
            }

            float4 pos_j = pc.positions_read[i];
            float4 delta = pos_i - pos_j;
            float  d     = dot(delta, delta);

            if (d <= 0.00001)
            {
                float4 dir = random_unit_vector(particle_index);
                pressure -= dir * 500.0;
                continue;
            }
            if (d > scene_ubo.smoothing_radius * scene_ubo.smoothing_radius)
            {
                continue;
            }
            d = sqrt(d);

            float4 gradient = W_spiky_gradient(delta, d);

            float density_j = pc.densities[i];
            float term2     = pc.pressures[i];

            pressure += gradient * (term1 + term2);

            float4 v_ij = vel_i - pc.velocities_read[i];
            float4 x_ij = pos_i - pc.positions_read[i];

            float numerator   = dot(x_ij, gradient);
            float denominator = dot(x_ij, x_ij) + 0.01 * scene_ubo.smoothing_radius * scene_ubo.smoothing_radius;

            viscosity += v_ij * (1.0 / density_j) * (numerator / denominator) * scene_ubo.visocity_coeff;
        }
    }

    pressure = -pressure;
    viscosity *= 2;

    return pressure + viscosity;
}

[shader("compute")]
[numthreads(256, 1, 1)]
void computeMain(uint index : SV_DispatchThreadID)
{
    if (index >= scene_ubo.particle_count)
    {
        return;
    }

    float4 pos = pc.positions_read[index];
    float4 vel = pc.velocities_read[index];

    float4 acc = float4(0, -9, 0, 0);
    acc += calc_force(index);

    if (pc.first_run == 1.0)
    {
        vel += acc * (pc.dt / 2.0);
    }
    else
    {
        vel += acc * pc.dt;
    }

    pos += vel * pc.dt;

    bool3 hit = (pos.xyz < pc.box_min.xyz) || (pos.xyz > pc.box_max.xyz);
    vel.xyz = select(hit, -vel.xyz, vel.xyz);
    pos.xyz = clamp(pos.xyz, pc.box_min.xyz, pc.box_max.xyz);

    pc.positions_write[index]  = pos;
    pc.velocities_write[index] = vel;
}
