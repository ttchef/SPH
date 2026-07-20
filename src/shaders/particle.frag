#version 450

#define MAX_VELO 200.0

layout(location = 0) in vec4 in_vel;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec3 in_view_pos;
layout(location = 3) in float in_radius;

layout(location = 0) out vec4 frag_color;

layout (push_constant) uniform pc
{
    mat4 view;
    mat4 perspective;       
};

void main() {
    float dist_sq = dot(in_uv, in_uv);
    if (dist_sq > 1.0) {
        discard; 
    }

    float z = sqrt(1.0 - dist_sq);
    vec3 normal = normalize(vec3(in_uv, z));

    vec3 light_dir = normalize(vec3(0.5, 0.5, 1.0));
    float diffuse = max(dot(normal, light_dir), 0.2);
    
    float speed = length(in_vel.xyz); 
    float t = min(speed / MAX_VELO, 1.0);
    vec3 base_color = vec3(t, 0.0, 1.0 - t);

    vec3 surface_view_pos = in_view_pos;
    surface_view_pos.z += z * in_radius;

    vec4 clip_pos = perspective * vec4(surface_view_pos, 1.0);
    gl_FragDepth = clip_pos.z / clip_pos.w;

    frag_color = vec4(base_color * diffuse, 1.0);
}
