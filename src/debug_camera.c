#include "debug_camera.h"

#include <whitgl/logging.h>

#include <whitgl/input.h>

ld37_debug_camera ld37_debug_camera_update(ld37_debug_camera camera)
{
	whitgl_fvec joystick = whitgl_input_joystick();
	camera.yaw += joystick.x/20;
	whitgl_fvec facing = whitgl_fvec_from_angle(-camera.yaw);
	whitgl_fvec3 forwards = {facing.x, 0, facing.y};
	camera.pos = whitgl_fvec3_add(camera.pos, whitgl_fvec3_scale_val(forwards, -joystick.y/32));
	if(whitgl_input_held(WHITGL_INPUT_A))
		camera.pos.y -= 1/20.0;
	if(whitgl_input_held(WHITGL_INPUT_B))
		camera.pos.y += 1/20.0;
	return camera;
}
whitgl_fmat ld37_debug_camera_matrix(ld37_debug_camera camera)
{
	whitgl_fvec3 up = {0,1,0};
	whitgl_fvec facing = whitgl_fvec_from_angle(-camera.yaw);
	whitgl_fvec3 forwards = {facing.x, -0.2, facing.y};
	whitgl_fvec3 camera_to = whitgl_fvec3_add(camera.pos, forwards);
	whitgl_fmat view = whitgl_fmat_lookAt(camera.pos, camera_to, up);
	return view;
}
