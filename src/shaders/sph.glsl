

#define PI 3.1415926538

#define SMOOTHING_RADIUS 25.0
#define VISCOSITY_COEFF 5000.0

// NOTE: Get from simulating in a grid
// #define TARGET_DENSITY 0.0604654476
#define TARGET_DENSITY 0.0087538725
#define U32_MAX (~0u)    

struct particle
{
    vec4 pos;
    vec4 vel;
    float mass;
    float density;
    float padding[2];      
};

struct spatial_lookup_entry
{
    uint particle_index;
    uint cell_key;       
};

// NOTE: changed in the pipeline creation
layout (constant_id = 1) const uint PARTICLE_COUNT = 1024;

#define NEIGHBOR_CELL_COUNT 27

const ivec3 NEIGHBOR_OFFSETS[NEIGHBOR_CELL_COUNT] = ivec3[](
    ivec3(-1,-1, 0), ivec3(0,-1, 0), ivec3(1,-1, 0),
    ivec3(-1, 0, 0), ivec3(0, 0, 0), ivec3(1, 0, 0),
    ivec3(-1, 1, 0), ivec3(0, 1, 0), ivec3(1, 1, 0),
    
    ivec3(-1,-1, 1), ivec3(0,-1, 1), ivec3(1,-1, 1),
    ivec3(-1, 0, 1), ivec3(0, 0, 1), ivec3(1, 0, 1),
    ivec3(-1, 1, 1), ivec3(0, 1, 1), ivec3(1, 1, 1),
    
    ivec3(-1,-1, -1), ivec3(0,-1, -1), ivec3(1,-1, -1),
    ivec3(-1, 0, -1), ivec3(0, 0, -1), ivec3(1, 0, -1),
    ivec3(-1, 1, -1), ivec3(0, 1, -1), ivec3(1, 1, -1)
    
);

ivec3 position_to_cell(vec4 pos)
{
    return ivec3(pos.x / SMOOTHING_RADIUS, pos.y / SMOOTHING_RADIUS, pos.z / SMOOTHING_RADIUS);        
}

uint cell_hash(ivec3 cell, uint particle_count)
{
    uint a = uint(cell.x) * 15824;
    uint b = uint(cell.y) * 9737333;
    uint c = uint(cell.z) * 74771;
    return (a + b + c) % particle_count;        
}

