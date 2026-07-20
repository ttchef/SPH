
#version 450

layout (location = 0) out vec4 frag_color;

layout (push_constant) uniform pc
{
    mat4 mvp;
    vec4 color;     
};

void main()
{
    frag_color = color;
}

