
#version 450

layout (push_constant) uniform pc
{
    mat4 mvp;       
};

vec3 cube_line_vertices[] = vec3[](
    vec3(-0.5, -0.5, 0.5), vec3(-0.5, 0.5, 0.5),
    vec3(-0.5, 0.5, 0.5), vec3(0.5, 0.5, 0.5),
    vec3(0.5, 0.5, 0.5), vec3(0.5, -0.5, 0.5),
    vec3(0.5, -0.5, 0.5), vec3(-0.5, -0.5, 0.5),

    vec3(-0.5, -0.5, -0.5), vec3(-0.5, 0.5, -0.5),
    vec3(-0.5, 0.5, -0.5), vec3(0.5, 0.5, -0.5),
    vec3(0.5, 0.5, -0.5), vec3(0.5, -0.5, -0.5),
    vec3(0.5, -0.5, -0.5), vec3(-0.5, -0.5, -0.5),

    vec3(-0.5, -0.5, 0.5), vec3(-0.5, -0.5, -0.5),
    vec3(-0.5, 0.5, 0.5), vec3(-0.5, 0.5, -0.5),
    vec3(0.5, 0.5, 0.5), vec3(0.5, 0.5, -0.5),
    vec3(0.5, -0.5, 0.5), vec3(0.5, -0.5, -0.5) 
);

void main()
{
    vec3 pos = cube_line_vertices[gl_VertexIndex];
    gl_Position = mvp * vec4(pos, 1.0);            
}

