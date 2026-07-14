
#pragma once

#include <math/types.h>

static inline m4 m4orthographic(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far)
{
	m4 result = (m4){ .m = {
		2 / (right - left), 0, 0, 0,
		0, 2 / (top - bottom), 0, 0,
		0, 0, 1 / (far - near), 0,
		-(right + left) / (right - left),
		-(top + bottom) / (top - bottom),
		-near / (far - near),
		1
	}};

	return result;
}


