#version 450

#define MAX_VELO 200.0

layout(location = 0) in vec4 in_vel;
layout(location = 1) in vec2 in_uv;

layout(location = 0) out vec4 frag_color;

void main() {
    float dist_sq = dot(in_uv, in_uv);
    if (dist_sq > 1.0) {
        discard; 
    }

    float z = sqrt(1.0 - dist_sq);
    vec3 normal = normalize(vec3(in_uv, z));

    vec3 light_dir = normalize(vec3(0.5, 0.5, 1.0));
    float diffuse = max(dot(normal, light_dir), 0.2); // 0.2 acts as ambient light

    float speed = length(in_vel.xyz); 
    float t = min(speed / MAX_VELO, 1.0);
    vec3 base_color = vec3(t, 0.0, 1.0 - t);

    frag_color = vec4(base_color * diffuse, 1.0);
}
