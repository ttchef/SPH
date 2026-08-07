
#pragma once

#include <math/types.h>

#include <SDL3/SDL_stdinc.h>

//
// NOTE: V2
//

static inline v2 v2add(v2 a, v2 b)
{
    return v2make(a.x + b.x, a.y + b.y);
}

static inline v2 v2sub(v2 a, v2 b)
{
    return v2make(a.x - b.x, a.y - b.y);
}

static inline v2 v2scale(v2 v, f32 s)
{
    return v2make(v.x * s, v.y * s);
}

static inline f32 v2lensq(v2 v)
{
    return v.x * v.x + v.y * v.y;
}

static inline f32 v2len(v2 v)
{
    return SDL_sqrtf(v2lensq(v));
}

static inline f32 v2dot(v2 a, v2 b)
{
    return a.x * b.x + a.y * b.y;
}

static inline f32 v2cross(v2 a, v2 b)
{
    return a.x * b.y - a.y * b.x;
}

static inline f32 v2dist(v2 a, v2 b)
{
    v2 d = v2sub(b, a);
    return v2dot(d, d);
}

static inline v2 v2lerp(v2 a, v2 b, f32 t)
{
    return v2make(t * b.x + (1.0f - t) * a.x, t * b.y + (1.0f - t) * a.y);
}

static inline v2 v2rotate(v2 v, f32 radians)
{
	f32 x = v.x * SDL_cos(radians) + v.y * -SDL_sin(radians);
	f32 y = v.x * SDL_sin(radians) + v.y * SDL_cos(radians);

	return v2make(x, y); 
}

//
// NOTE: V2u
// 

static inline v2u v2uadd(v2u a, v2u b)
{
    return v2umake(a.x + b.x, a.y + b.y);
}

//
// NOTE: V3
//

static inline v3 v3add(v3 a, v3 b)
{
    return v3make(a.x + b.x, a.y + b.y, a.z + b.z);
}

static inline v3 v3sub(v3 a, v3 b)
{
    return v3make(a.x - b.x, a.y - b.y, a.z - b.z);
}

static inline v3 v3scale(v3 v, f32 s)
{
    return v3make(v.x * s, v.y * s, v.z * s);
}

static inline f32 v3lensq(v3 v)
{
    return v.x * v.x + v.y * v.y + v.z * v.z;
}

static inline f32 v3len(v3 v)
{
    return SDL_sqrtf(v3lensq(v));
}

static inline v3 v3norm(v3 v)
{
    f32 len = v3len(v);
    if (len <= SDL_FLT_EPSILON)
    {
        return v3make(0, 0, 0);
    }

    return v3scale(v, 1.0f / len);
}

static inline v3 v3cross(v3 a, v3 b)
{
    return v3make(
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x);
}
