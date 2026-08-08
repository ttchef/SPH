
#pragma once

#include <SDL3/SDL_timer.h>
#include <types.h>

// NOTE: All in ms
typedef struct time
{
    u64 last;

    f64 delta;
    f64 smooth_delta;
    // NOTE: Never gets resetted
    f64 elapsed;
} time;

static inline time time_create(void)
{
    time result = {0};

    result.last    = SDL_GetPerformanceCounter();
    result.elapsed = 0.0;
    result.delta   = 0.0;

    return result;
}

static inline void time_update(time *time)
{
    assert(time);

    const u64 now  = SDL_GetPerformanceCounter();
    const u64 freq = SDL_GetPerformanceFrequency();

    time->delta        = (f64)(now - time->last) / (f64)freq;
    time->smooth_delta = 0.9f * time->smooth_delta + 0.1f * time->delta;
    time->last         = now;

    time->elapsed += time->delta;
}
