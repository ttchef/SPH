

#define PI 3.1415926538

#define SMOOTHING_RADIUS 65.0

// NOTE: Get from simulating in a grid
#define TARGET_DENSITY 0.0006076722    

struct particle
{
    vec2 pos;
    vec2 vel;
    float mass;
    float density;       
};


