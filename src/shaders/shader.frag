
#version 450

#define MAX_VELO 200.0

layout (location = 0) in vec2 in_vel;

layout (location = 0) out vec4 frag_color;

void main()
{
    float speed = length(in_vel);
    float t = min(speed / MAX_VELO, 1.0);
    
    frag_color = vec4(t, 0.0, 1.0 - t, 1.0);
}
