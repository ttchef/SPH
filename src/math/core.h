
#pragma once

//
// NOTE: This file includes all basic math utilities
// 

#include <math/types.h>
#include <math/matrix.h>
#include <math/vector.h>
#include <math/sdf.h>
#include <math/collision.h>

// NOTE: I dont know in which section to put it yet
static inline v3 math_barycentrics(v2 a, v2 b, v2 c, v2 p)
{
    v2 ab = v2sub(b, a);
    v2 ca = v2sub(c, a);

    f32 u = (a.x * ca.y + (p.y - a.y) * ca.x - p.x * ca.y) / (ab.y * ca.x - ab.x * ca.y);
    f32 v = (p.y - a.y - u * ab.y) / ca.y;
    f32 w = 1.0 - u - v;

    return v3make(u, v, w);
}
