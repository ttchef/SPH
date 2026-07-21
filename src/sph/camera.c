
#include <sph/camera.h>
#include <math/core.h>

camera camera_create(void)
{
	camera result = {0};

	result.pos = v3make(0, 0, 600);
	result.dir = v3make(0, 0, -1);

	result.yaw = -90.0f;
	result.pitch = 0.0f;

	result.speed = 100.0f;
	result.sensitivity = 0.1f;

	return result;	
}

void camera_update(camera *camera, window *window, input *input, f32 dt)
{
	const v3 forward = v3norm(v3make(camera->dir.x, 0.0f, camera->dir.z));
	const v3 up = v3up();
	const v3 right = v3norm(v3cross(camera->dir, up));

	// SDL_Log("[ENGINE] Camera Pos: (%.3f, %.3f, %.3f)", camera->pos.x, camera->pos.y, camera->pos.z);

	if (input_down(input, INPUT_W))
	{
		camera->pos = v3add(camera->pos, v3scale(forward, camera->speed * dt));
	}
	if (input_down(input, INPUT_S))
	{
		camera->pos = v3sub(camera->pos, v3scale(forward, camera->speed * dt));
	}
	if (input_down(input, INPUT_A))
	{
		camera->pos = v3sub(camera->pos, v3scale(right, camera->speed * dt));
	}
	if (input_down(input, INPUT_D))
	{
		camera->pos = v3add(camera->pos, v3scale(right, camera->speed * dt));
	}
	if (input_down(input, INPUT_SPACE))
	{
		camera->pos = v3add(camera->pos, v3scale(up, camera->speed * dt));
	}
	if (input_down(input, INPUT_LSHIFT))
	{
		camera->pos = v3sub(camera->pos, v3scale(up, camera->speed * dt));
	}

	if (input_pressed(input, INPUT_LMB))
	{
		SDL_SetWindowRelativeMouseMode(window->handle, true);
		camera->invis_cursor = true;
	}
	if (input_released(input, INPUT_LMB))
	{
		SDL_SetWindowRelativeMouseMode(window->handle, false);
		camera->invis_cursor = false;
	}

	if (camera->invis_cursor)
	{
		camera->yaw += input->mouse_delta.x * camera->sensitivity;
		camera->pitch -= input->mouse_delta.y * camera->sensitivity;

		camera->pitch = CLAMP(camera->pitch, -89.0f, 89.0f);

		f32 yaw_rad = TO_RADIANS(camera->yaw);
		f32 pitch_rad = TO_RADIANS(camera->pitch);

		v3 look_dir = v3make(SDL_cosf(pitch_rad) * SDL_cosf(yaw_rad), SDL_sinf(pitch_rad), SDL_cosf(pitch_rad) * SDL_sinf(yaw_rad));
		camera->dir = v3norm(look_dir);
	}
}

m4 camera_view(camera *camera)
{
    f32 pitch_rad = TO_RADIANS(camera->pitch);
    f32 yaw_rad = TO_RADIANS(camera->yaw);

    m4 translation = m4translate(-camera->pos.x, -camera->pos.y, -camera->pos.z);

    m4 rotation_x = m4rotate(-pitch_rad, 1.0f, 0.0f, 0.0f);
    m4 rotation_y = m4rotate((yaw_rad + TO_RADIANS(90.0f)), 0.0f, 1.0f, 0.0f);
    m4 rotation = m4mul(rotation_x, rotation_y);

    return m4mul(rotation, translation);		
}
