
#pragma once

#include <types.h>
#include <math/types.h>
#include <sph/input.h>

typedef struct camera {
    f32 speed;
    f32 sensitivity;

    f32 yaw;
    f32 pitch;

    bool left_mouse_last;

    v2 last_mouse;
    v3 pos;
    v3 dir;

    bool invis_cursor;
} camera;

camera camera_create(void);

void camera_update(camera *camera, input *input, f32 dt);

m4 camera_view(camera *camera);
