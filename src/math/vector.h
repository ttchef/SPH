
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
        a.x * b.y - a.y * b.x
    );	
}
