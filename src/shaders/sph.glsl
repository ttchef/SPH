

#define PI 3.1415926538

#define SMOOTHING_RADIUS 50.0
#define VISCOSITY_COEFF 5000.0

// NOTE: Get from simulating in a grid
#define TARGET_DENSITY 0.0100026466

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
