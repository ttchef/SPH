
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

typedef struct m4
{
	f32 m[16];
} m4;
