
#pragma once

#include <types.h>
#include <SDL3/SDL_timer.h>

// NOTE: All in ms
typedef struct time
{
    f64 last;
    f64 delta;
    // NOTE: Never gets resetted
    f64 accumulated;
} time;

static inline void time_init(time *time)
{
	assert(time);

	time->last = ((f64)SDL_GetTicks()) / 1000.0;
	time->accumulated = 0.0;
}

static inline void time_update(time *time)
{
    assert(time);

    const f64 now = ((f64)SDL_GetTicks()) / 1000.0;
    time->delta = now - time->last;
    time->last = now;

    time->accumulated += time->delta;
}
