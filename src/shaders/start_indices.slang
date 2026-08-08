
import utils;

struct PushConstant
{
    SpatialLookupEntry *spatial_lookup;
    uint               *start_indices;
}

[[vk::push_constant]]
PushConstant pc;

[shader("compute")]
    [numthreads(256, 1, 1)] void
    computeMain(uint index : SV_DispatchThreadID)
{
    if (index >= scene_ubo.particle_count)
    {
        return;
    }

    uint key      = pc.spatial_lookup[index].cell_key;
    uint key_prev = (index == 0) ? U32_MAX : pc.spatial_lookup[index - 1].cell_key;

    if (key != key_prev)
    {
        pc.start_indices[key] = index;
    }
}
