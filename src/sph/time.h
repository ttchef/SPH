
#pragma once

#include <types.h>
#include <SDL3/SDL_timer.h>

// NOTE: All in ms
typedef struct time
{
    u64 last;

    f64 delta;
    // NOTE: Never gets resetted
    f64 accumulated;
} time;

static inline void time_init(time *time)
{
	assert(time);

	time->last = SDL_GetPerformanceCounter();
	time->accumulated = 0.0;
	time->delta = 0.0;
}

static inline void time_update(time *time)
{
    assert(time);

    const u64 now = SDL_GetPerformanceCounter();
    const u64 freq = SDL_GetPerformanceFrequency();

    time->delta = (f64)(now - time->last) / (f64)freq;
    time->last = now;

    time->accumulated += time->delta;
}
