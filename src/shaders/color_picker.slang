

import utils;

struct PushConstant
{
    float4x4 model;
    float4 color;
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
    p.y = p.y + r/k;

    if( p.x+k*p.y>0.0 ) p = float2(p.x-k*p.y,-k*p.x-p.y)/2.0;
    p.x -= clamp( p.x, -2.0*r, 0.0 );
    return -length(p)*sign(p.y);
}

float sdfCircle(float2 p, float inner, float outer)
{
    float d = length(p);
    return max(inner - d, d - outer);
}

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
    float4 color = pc.color;

	float2 remapped_uv = input.uv * 2 - 1;
	float triangle_sdf = sdfEquilateralTriangle(remapped_uv, 0.75);
	float circle_sdf = sdfCircle(remapped_uv, 0.9, 1.0);

    if (triangle_sdf <= 0.015)
    {
        const float2 v0 = float2(0.0, 0.75);
        const float2 v1 = float2(-0.75, -0.75);
        const float2 v2 = float2(0.75, -0.75);

        float2 p = remapped_uv;

        float u = (v0.x * (v2.y - v0.x) + (p.y - v0.y) * (v2.x - v0.x) - p.x * (v2.y - v0.y)) / ((v1.y - v0.y) * (v2.x - v0.x) - (v1.x - v0.x) * (v2.y - v0.y));
        float v = (p.y - v0.y - u * (v1.y - v0.y)) / (v2.y - v0.y);
        float w = 1.0 - u - v; 
                
        color = float4(v, u, w, 1.0);
    }
    if (circle_sdf <= 0.015)
    {
        float angle = atan2(remapped_uv.y, remapped_uv.x) * (180 / PI);

        if (angle < 0.0)
        {
            angle += 360.0;
        }

        color = float4(hsvToRgb(angle, 1.0, 1.0), 1.0);
    }

	float sdf = min(triangle_sdf, circle_sdf);
	float pd = fwidth(sdf);
	float mask = 1.0 - saturate(sdf / pd);

    color *= mask;
    return float4(gamma_correct(color.xyz), color.w);
}
