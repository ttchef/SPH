
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

static inline v2 v2zero(void)
{
	return v2make(0, 0);
}

typedef struct v2u
{
	u32 x;
	u32 y;
} v2u;

static inline v2u v2umake(u32 x, u32 y)
{
	return (v2u){x, y};
}

static inline v2u v2uzero(void)
{
	return v2umake(0, 0);
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

static inline v3 v3zero(void)
{
	return v3make(0, 0, 0);
}

static inline v3 v3up(void)
{
	return v3make(0, 1, 0);
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

static inline v4 v4fromv3(v3 v, f32 w)
{
	return v4make(v.x, v.y, v.z, w);
}

static inline v4 v4fromcolor4(color4 color)
{
	return v4make(color.r, color.g, color.b, color.a);
}

static inline v4 v4zero(void)
{
	return v4make(0, 0, 0, 0);
}

typedef struct m4
{
	f32 m[16];
} m4;

// NOTE: Origin in center
typedef struct cube
{
	v3 pos;
	v3 size;
} cube;

static inline cube cubemake(v3 pos, v3 size)
{
	return (cube){pos, size};
}
