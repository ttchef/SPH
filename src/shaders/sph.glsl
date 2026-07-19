

#define PI 3.1415926538

#define SMOOTHING_RADIUS 50.0
#define VISCOSITY_COEFF 5000.0

// NOTE: Get from simulating in a grid
// #define TARGET_DENSITY 0.0100026466
#define TARGET_DENSITY 0.0277777854
#define U32_MAX (~0u)    

struct particle
{
    vec2 pos;
    vec2 vel;
    float mass;
    float density;       
};

struct spatial_lookup_entry
{
    uint particle_index;
    uint cell_key;       
};

// NOTE: changed in the pipeline creation
layout (constant_id = 1) const uint PARTICLE_COUNT = 1024;

const ivec2 NEIGHBOR_OFFSETS[9] = ivec2[](
    ivec2(-1,-1), ivec2(0,-1), ivec2(1,-1),
    ivec2(-1, 0), ivec2(0, 0), ivec2(1, 0),
    ivec2(-1, 1), ivec2(0, 1), ivec2(1, 1)
);

ivec2 position_to_cell(vec2 pos)
{
    return ivec2(pos.x / SMOOTHING_RADIUS, pos.y / SMOOTHING_RADIUS);        
}

uint cell_hash(ivec2 cell, uint particle_count)
{
    uint a = uint(cell.x) * 15824;
    uint b = uint(cell.y) * 9737333;
    return (a + b) % particle_count;        
}

