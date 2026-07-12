
#version 450

layout (location = 0) in vec2 in_pos;

vec3[] vertices = vec3[](
    vec3(0.5, 0.5, 0.0),
    vec3(-0.5, 0.5, 0.0),
    vec3(0.0, -0.5, 0.0)
);

void main()
{
    gl_Position = vec4(in_pos, 0.0, 1.0);
    gl_PointSize = 5.0;
}
