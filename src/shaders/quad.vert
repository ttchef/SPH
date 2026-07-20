
#version 450

layout (push_constant) uniform pc
{
    mat4 mvp;
    vec4 color;     
};

const vec3 quad_vertices[6] = vec3[](
    vec3(-0.5, -0.5, 0.0), vec3(-0.5, 0.5, 0.0), vec3(0.5, 0.5, 0.0),
    vec3(0.5, 0.5, 0.0), vec3(0.5, -0.5, 0.0), vec3(-0.5, -0.5, 0.0)
);

void main()
{
    vec3 pos = quad_vertices[gl_VertexIndex];
    gl_Position = mvp * vec4(pos, 1.0);            
}
