
#pragma once

#include <types.h>

typedef struct V2
{
	f32 x;
	f32 y;
} V2;

static inline V2 v2(f32 x, f32 y)
{
	return (V2){x, y};
}

typedef struct M4
{
	f32 m[16];
} M4;
