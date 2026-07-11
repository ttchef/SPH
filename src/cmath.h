
#pragma once

#include <types.h>

// NOTE: dont wanna use this cooked thingy
// just so i can have a good conversion func (see v2fromraylib function)
#include <raymath.h>
#include <math.h>

struct v2
{
	float x;
	float y;
};

static inline struct v2 v2make(float x, float y)
{
	return (struct v2){x, y};
}

static inline struct v2 v2fromraylib(Vector2 raylib_vec)
{
	return v2make(raylib_vec.x, raylib_vec.y);
}

static inline Vector2 v2toraylib(struct v2 v)
{
	return (Vector2){v.x, v.y};
}

static inline struct v2 v2negate(struct v2 v)
{
	return v2make(-v.x, -v.y);
}

static inline float v2lensquared(struct v2 v)
{
	return v.x * v.x + v.y * v.y;	
}

static inline float v2len(struct v2 v)
{
	return sqrtf(v2lensquared(v));
}

static inline struct v2 v2sub(struct v2 a, struct v2 b)
{
	return v2make(a.x - b.x, a.y - b.y);	
}

static inline struct v2 v2add(struct v2 a, struct v2 b)
{
	return v2make(a.x + b.x, a.y + b.y);
}

static inline struct v2 v2scale(struct v2 v, float s)
{
	return v2make(v.x * s, v.y * s);
}

static inline float v2dist(struct v2 a, struct v2 b)
{
	return v2len(v2sub(a, b));
}

static inline float v2dot(struct v2 a, struct v2 b)
{
	return a.x * b.x + a.y * b.y;
}

struct v2u
{
	unsigned int x;
	unsigned int y;
};

static inline struct v2u v2umake(unsigned int x, unsigned int y)
{
	return (struct v2u){x, y};
}
