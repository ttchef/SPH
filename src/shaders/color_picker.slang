

import utils;

struct PushConstant
{
    float4x4 model;
    float4 data;
};

[[vk::push_constant]]
PushConstant pc;

struct VSOutput
{
    float4 position : SV_Position;
    float2 uv;
};

[shader("vertex")]
VSOutput vertexMain(uint index: SV_VulkanVertexID)
{
    VSOutput output;

    const float2 vertices[6] = {
        float2(-0.5, -0.5),
        float2(-0.5, 0.5),
        float2(0.5, 0.5),

        float2(-0.5, -0.5),
        float2(0.5, -0.5),
        float2(0.5, 0.5),
    };

    const float2 uv[6] = {
        float2(0.0f, 1.0f),
        float2(0.0f, 0.0f),
        float2(1.0f, 0.0f),

        float2(0.0f, 1.0f),
        float2(1.0f, 1.0f),
        float2(1.0f, 0.0f),
    };

    output.position = mul(ubo.orthographic, mul(pc.model, float4(vertices[index], 0.0, 1.0)));
    output.uv       = uv[index];

    return output;
}

float sdfEquilateralTriangle(float2 p, float r)
{
    const float k = sqrt(3.0);

    p.x = abs(p.x) - r;
    p.y = p.y + r / k;

    if(p.x + k * p.y > 0.0)
    {
        p = float2(p.x - k * p.y, -k * p.x - p.y) / 2.0;
    }
    p.x -= clamp(p.x, -2.0 * r, 0.0);
    return -length(p) * sign(p.y);
}

float sdfRing(float2 p, float inner, float outer)
{
    float d = length(p);
    return max(inner - d, d - outer);
}

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
    float4 color = float4(0.0, 0.0, 0.0, 0.0);

    float hue = pc.data.w;

	float2 remapped_uv = input.uv * 2 - 1;
    float2 triangle_uv = rotate(remapped_uv, degToRad(-hue + 90.0f));
    const float triangle_size = 0.75;

	float triangle_sdf = sdfEquilateralTriangle(triangle_uv, triangle_size);
	float circle_sdf = sdfRing(remapped_uv, 0.9, 1.0);

    if (triangle_sdf <= 0.015)
    {
        const float k = sqrt(3.0);
        const float2 v0 = float2(0.0, 2.0 * triangle_size / k);
        const float2 v1 = float2(-triangle_size, -triangle_size / k);
        const float2 v2 = float2(triangle_size, -triangle_size / k);

        const float3 colors[] = {
            float3(hue, 0.0, 1.0),
            float3(hue, 1.0, 0.0),
            float3(hue, 1.0, 1.0),
        };

        float2 p = triangle_uv;

        const float2 v0v1 = v1 - v0;
        const float2 v0v2 = v2 - v0;
        
        float u = (v0.x * v0v2.y + (p.y - v0.y) * v0v2.x - p.x * v0v2.y) / (v0v1.y * v0v2.x - v0v1.x * v0v2.y);
        float v = (p.y - v0.y - u * v0v1.y) / v0v2.y;
        float w = 1.0 - u - v; 
                
        float3 rgb = hsvToRgb(u * colors[0] + v * colors[1] + w * colors[2]);
        float pd = fwidth(triangle_sdf);
        float alhpa = 1.0 - saturate(triangle_sdf / pd);
        color = lerp(color, float4(rgb, 1.0), alhpa);
    }
    if (circle_sdf <= 0.015)
    {
        float angle = radToDeg(atan2(remapped_uv.y, remapped_uv.x));

        if (angle < 0.0)
        {
            angle += 360.0;
        }

        float3 rgb = hsvToRgb(angle, 1.0, 1.0);
        float pd = fwidth(circle_sdf);
        float alpha = 1.0 - saturate(circle_sdf / pd);
        color = lerp(color, float4(rgb, 1.0), alpha);
    }

    float triangle_point_valid = pc.data.z;
    float selected_circle_sdf = 10000000.0;
    if (triangle_point_valid == 1.0)
    {
        selected_circle_sdf = sdfRing(triangle_uv - pc.data.xy, 0.05, 0.07);
        float pd = fwidth(selected_circle_sdf);
        float alpha = 1.0 - saturate(selected_circle_sdf / pd);
        color = lerp(color, float4(1.0, 1.0, 1.0, 1.0), alpha);
    }

    return float4(gamma_correct(color.xyz), color.w);
}
