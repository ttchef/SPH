
#version 450

layout (push_constant) uniform pc
{
    mat4 mvp;
    vec4 color;     
};

layout (location = 0) out vec4 frag_color;

void main()
{
    frag_color = color;        
}
