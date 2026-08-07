
#pragma once

#include <math/types.h>
#include <math/vector.h>

static inline v2 math_closest_point_on_segment(v2 p, v2 a, v2 b)
{
    v2  ab  = v2sub(b, a);
    f32 ab2 = v2dot(ab, ab);

    if (ab2 <= 0.0f)
    {
        return a;
    }

    f32 t = v2dot(v2sub(p, a), ab) / ab2;
    t     = CLAMP(t, 0.0f, 1.0f);

    return v2add(a, v2scale(ab, t));
}

static inline v2 math_closest_point_on_triangle(v2 p, v2 a, v2 b, v2 c)
{
    f32 ab = v2cross(v2sub(b, a), v2sub(p, a));
    f32 bc = v2cross(v2sub(c, b), v2sub(p, b));
    f32 ca = v2cross(v2sub(a, c), v2sub(p, c));

    if ((ab >= 0.0f && bc >= 0.0f && ca >= 0.0f) ||
        (ab <= 0.0f && bc <= 0.0f && ca <= 0.0f))
    {
        return p;
    }

    v2 q_ab = math_closest_point_on_segment(p, a, b);
    v2 q_bc = math_closest_point_on_segment(p, b, c);
    v2 q_ca = math_closest_point_on_segment(p, c, a);

    f32 d_ab = v2dist(q_ab, p);
    f32 d_bc = v2dist(q_bc, p);
    f32 d_ca = v2dist(q_ca, p);

    if (d_ab <= d_bc && d_ab <= d_ca)
    {
        return q_ab;
    }

    if (d_bc <= d_ca)
    {
        return q_bc;
    }

    return q_ca;
}
