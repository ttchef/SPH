

#define PI 3.1415926538

#define SMOOTHING_RADIUS 65.0
#define VISCOSITY_COEFF 5000.0

// NOTE: Get from simulating in a grid
#define TARGET_DENSITY 0.005576722    

struct particle
{
    vec2 pos;
    vec2 vel;
    float mass;
    float density;       
};


