
#pragma once

#include <types.h>
#include <math/types.h>
#include <sph/input.h>
#include <sph/window.h>

typedef struct camera {
    f32 speed;
    f32 sensitivity;

    f32 yaw;
    f32 pitch;

    v3 pos;
    v3 dir;

    bool invis_cursor;
} camera;

camera camera_create(void);

void camera_update(camera *camera, window *window, input *input, f32 dt);

m4 camera_view(camera *camera);
