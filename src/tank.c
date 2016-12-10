#include "tank.h"

#include <whitgl/input.h>

ld37_tank ld37_tank_update(ld37_tank tank, whitgl_bool input_dirs[4])
{
	tank.just_arrived = false;
	if(tank.transition > 0)
	{
		tank.transition -= 1/16.0;
		if(tank.transition <= 0)
		{
			if(tank.next.facing == tank.current.facing)
				tank.just_arrived = true;
			tank.current = tank.next;
		}
		return tank;
	}
	tank.next = tank.current;
	if(input_dirs[3])
	{
		tank.next.facing = whitgl_iwrap(tank.current.facing+1,0,4);
		tank.transition = 1;
	}
	else if(input_dirs[1])
	{
		tank.next.facing = whitgl_iwrap(tank.current.facing-1,0,4);
		tank.transition = 1;
	}
	else if(input_dirs[0])
	{
		tank.next.pos = whitgl_ivec_add(tank.current.pos, whitgl_facing_to_ivec(tank.current.facing));
		tank.transition = 1;
	}
	else if(input_dirs[2])
	{
		tank.next.pos = whitgl_ivec_add(tank.current.pos, whitgl_ivec_inverse(whitgl_facing_to_ivec(tank.current.facing)));
		tank.transition = 1;
	}
	return tank;
}
whitgl_fmat ld37_tank_camera_matrix(ld37_tank tank)
{
	whitgl_fvec current_pos2d = whitgl_ivec_to_fvec(tank.current.pos);
	whitgl_fvec3 current_pos = {current_pos2d.x, 0.5, current_pos2d.y};
	whitgl_fvec next_pos2d = whitgl_ivec_to_fvec(tank.next.pos);
	whitgl_fvec3 next_pos = {next_pos2d.x, 0.5, next_pos2d.y};
	whitgl_fvec3 pos = whitgl_fvec3_interpolate(next_pos, current_pos, tank.transition);
	pos.x -= 5.5;
	pos.z += 5.5;
	pos.z = -pos.z;

	whitgl_fvec3 up = {0,1,0};
	whitgl_float current_angle = whitgl_facing_to_angle(tank.current.facing);
	whitgl_float next_angle = whitgl_facing_to_angle(tank.next.facing);
	whitgl_fvec facing = whitgl_fvec_from_angle(whitgl_angle_lerp(next_angle, current_angle, tank.transition));
	whitgl_fvec3 forwards = {facing.x, 0.1, facing.y};
	whitgl_fvec3 camera_to = whitgl_fvec3_add(pos, forwards);
	whitgl_fmat view = whitgl_fmat_lookAt(pos, camera_to, up);
	return view;
}
