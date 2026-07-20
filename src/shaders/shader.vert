
#version 450

#extension GL_GOOGLE_include_directive : require

#include "sph.glsl"

layout (location = 0) out vec4 out_vel;
layout (location = 1) out vec2 out_uv;
layout (location = 2) out vec3 out_view_pos;
layout (location = 3) out float out_radius;

layout (std430, set = 0, binding = 0) readonly buffer in_p
{
    particle particles[];
};

layout (push_constant) uniform pc
{
    mat4 view;
    mat4 perspective;       
};

void main()
{
    uint particle_id = gl_VertexIndex / 6;
    uint corner_id = gl_VertexIndex % 6;

    vec4 pos = particles[particle_id].pos;
    vec3 center_world = pos.xyz;

    const float radius = 1.0;

    const vec2 offsets[6] = vec2[](
        vec2(-1.0, -1.0), vec2(1.0, -1.0), vec2(-1.0, 1.0),
        vec2(-1.0, 1.0),  vec2(1.0, -1.0), vec2(1.0, 1.0)
    );

    vec2 offset = offsets[corner_id];

    vec4 view_pos = view * vec4(center_world, 1.0);
    view_pos.xy += offset * radius;

    gl_Position = perspective * view_pos;
        
    out_vel = particles[particle_id].vel;
    out_uv = offset;
    out_view_pos = view_pos.xyz;
    out_radius = radius;
}
