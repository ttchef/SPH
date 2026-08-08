
#pragma once

#include <math/types.h>
#include <math/vector.h>

#include <SDL3/SDL_stdinc.h>

static inline f32 sdf_triangle2D(v2 p, f32 size)
{
    const f32 k = SDL_sqrtf(3.0);

    p.x = ABS(p.x) - size;
    p.y += size / k;

    if (p.x + k * p.y > 0.0f)
    {
        p = v2scale(v2make(p.x - k * p.y, -k * p.x - p.y), 0.5f);
    }
    p.x -= CLAMP(p.x, -2.0f * size, 0.0f);
    return -v2len(p) * SIGN(p.y);
}

static inline f32 sdf_ring2D(v2 p, f32 inner, f32 outer)
{
    f32 d = v2len(p);
    return MAX(inner - d, d - outer);
}
