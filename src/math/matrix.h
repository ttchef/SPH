
#pragma once

#include <math/types.h>

#include <math.h>

static inline m4 m4identity(void)
{
    return (m4){ .m = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    }};
}

static inline m4 m4multiply(m4 a, m4 b)
{
    m4 result;
    for (int col = 0; col < 4; ++col)
    {
        for (int row = 0; row < 4; ++row)
        {
            result.m[col * 4 + row] =
                a.m[0 * 4 + row] * b.m[col * 4 + 0] +
                a.m[1 * 4 + row] * b.m[col * 4 + 1] +
                a.m[2 * 4 + row] * b.m[col * 4 + 2] +
                a.m[3 * 4 + row] * b.m[col * 4 + 3];
        }
    }
    return result;
}

static inline m4 m4translate(f32 x, f32 y, f32 z)
{
    return (m4){ .m = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
           x,    y,    z, 1.0f
    }};
}

static inline m4 m4scale(f32 x, f32 y, f32 z)
{
    return (m4){ .m = {
           x, 0.0f, 0.0f, 0.0f,
        0.0f,    y, 0.0f, 0.0f,
        0.0f, 0.0f,    z, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    }};
}

static inline m4 m4rotate(f32 angle_radians, f32 axis_x, f32 axis_y, f32 axis_z)
{
    f32 length = sqrtf(axis_x * axis_x + axis_y * axis_y + axis_z * axis_z);
    if (length > 0.0f)
    {
        axis_x /= length;
        axis_y /= length;
        axis_z /= length;
    }

    f32 c = cosf(angle_radians);
    f32 s = sinf(angle_radians);
    f32 t = 1.0f - c;

    m4 result = (m4){ .m = {
        t * axis_x * axis_x + c,
        t * axis_x * axis_y + s * axis_z,
        t * axis_x * axis_z - s * axis_y,
        0.0f,

        t * axis_x * axis_y - s * axis_z,
        t * axis_y * axis_y + c,
        t * axis_y * axis_z + s * axis_x,
        0.0f,

        t * axis_x * axis_z + s * axis_y,
        t * axis_y * axis_z - s * axis_x,
        t * axis_z * axis_z + c,
        0.0f,

        0.0f, 0.0f, 0.0f, 1.0f
    }};

    return result;
}

static inline m4 m4orthographic(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far)
{
	m4 result = (m4){ .m = {
		2 / (right - left), 0, 0, 0,
		0, 2 / (top - bottom), 0, 0,
		0, 0, 1 / (far - near), 0,
		-(right + left) / (right - left),
		-(top + bottom) / (top - bottom),
		-near / (far - near), 1,
	}};

	return result;
}

static inline m4 m4perspective(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far)
{
    m4 result = (m4){ .m = {
        2 * near / (right - left), 0, 0, 0,
        0, 2 * near / (top - bottom), 0, 0,
        (right + left) / (right - left), (top + bottom) / (top - bottom), -far / (far - near), -1,
        0, 0, -(far * near) / (far - near), 0
    }};

    return result;
}

