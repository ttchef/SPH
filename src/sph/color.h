
#pragma once

#include <math/vector.h>
#include <types.h>

// NOTE: No need to be here but for now it doesnt matter
static inline f32 mod(f32 x, f32 y)
{
    return x - y * SDL_floor(x / y);
}

static inline color4 color4lerp(color4 c0, color4 c1, f32 t)
{
    f32 r = t * c1.r - (1.0 - t) * c0.r;
    f32 g = t * c1.g - (1.0 - t) * c0.g;
    f32 b = t * c1.b - (1.0 - t) * c0.b;
    f32 a = t * c1.a - (1.0 - t) * c0.a;

    return color4make(r, g, b, a);
}

static color4 color4fromhsv(f32 hue, f32 saturation, f32 value)
{
    f32 chroma = value * saturation;
    f32 h1     = hue / 60.0f;
    f32 X      = chroma * (1.0 - ABS(mod(h1, 2.0f) - 1));

    v3 color;

    if (h1 < 1.0)
    {
        color = v3make(chroma, X, 0.0);
    }
    else if (h1 < 2.0)
    {
        color = v3make(X, chroma, 0.0);
    }
    else if (h1 < 3.0)
    {
        color = v3make(0.0, chroma, X);
    }
    else if (h1 < 4.0)
    {
        color = v3make(0.0, X, chroma);
    }
    else if (h1 < 5.0)
    {
        color = v3make(X, 0.0, chroma);
    }
    else if (h1 < 6.0)
    {
        color = v3make(chroma, 0.0, X);
    }
    else
    {
        // NOTE: Easy to debug
        color = v3make(1.0, 1.0, 1.0);
    }

    f32 m = value - chroma;
    return color4make(color.x + m, color.y + m, color.z + m, 1.0f);
}

static inline color4 color4fromhsv2(v3 hsv)
{
    return color4fromhsv(hsv.x, hsv.y, hsv.z);
}
