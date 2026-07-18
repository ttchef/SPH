
#version 450

layout (location = 0) in vec2 in_pos;
layout (location = 1) in vec2 in_vel;
layout (location = 2) in float in_mass;
layout (location = 3) in float in_density;

layout (location = 0) out vec2 out_vel;

layout (push_constant) uniform pc
{
    mat4 orthographic;       
};

void main()
{
    gl_Position = orthographic * vec4(in_pos, 0.0, 1.0);
    gl_PointSize = 5.0;

    out_vel = in_vel;
}
