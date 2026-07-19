
#pragma once

#include <types.h>

typedef struct v2
{
	f32 x;
	f32 y;
} v2;

static inline v2 v2make(f32 x, f32 y)
{
	return (v2){x, y};
}

typedef struct v3
{
	f32 x;
	f32 y;
	f32 z;
} v3;

static inline v3 v3make(f32 x, f32 y, f32 z)
{
	return (v3){x, y, z};
}

typedef struct v4
{
	f32 x;
	f32 y;
	f32 z;
	f32 w;
} v4;

static inline v4 v4make(f32 x, f32 y, f32 z, f32 w)
{
	return (v4){x, y, z, w};
}

typedef struct m4
{
	f32 m[16];
} m4;
