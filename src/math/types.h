
#pragma once

#include <types.h>

typedef struct
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

typedef struct
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

typedef struct
{
	f32 x;
	f32 y;
	f32 z;
} v3;

static inline v3 v3make(f32 x, f32 y, f32 z)
{
	return (v3){x, y, z};
}

static inline v3 v3fromv2(v2 v, f32 z)
{
	return v3make(v.x, v.y, z);
}

static inline v3 v3zero(void)
{
	return v3make(0, 0, 0);
}

static inline v3 v3up(void)
{
	return v3make(0, 1, 0);
}

typedef struct
{
	u32 x;
	u32 y;
	u32 z;
} v3u;

static inline v3u v3umake(u32 x, u32 y, u32 z)
{
	return (v3u){x, y, z};
}

typedef struct
{
	i32 x;
	i32 y;
	i32 z;
} v3i;

static inline v3i v3imake(i32 x, i32 y, i32 z)
{
	return (v3i){x, y, z};
}

typedef struct
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

static inline v4 v4make2(f32 *elements)
{
	return v4make(elements[0], elements[1], elements[2], elements[3]);
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

typedef struct
{
	f32 m[16];
} m4;

// NOTE: Origin in center
typedef struct
{
	v3 pos;
	v3 size;
} cube;

static inline cube cubemake(v3 pos, v3 size)
{
	return (cube){pos, size};
}
