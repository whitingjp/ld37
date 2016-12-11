#include "tank.h"

#include <whitgl/input.h>
#include <whitgl/logging.h>

whitgl_bool ld37_tank_valid(whitgl_ivec p)
{
	if(p.x < 0)
		return false;
	if(p.x > 11)
		return false;
	if(p.y > -1)
		return false;
	if(p.y < -11)
		return false;
	if(p.y == -6 && p.x < 6)
		return false;
	if((p.x == 6 || p.x == 7) && (p.y==-7 || p.y==-6))
		return false;
	if(p.x >= 8 && p.x <= 10 && p.y >=-10 && p.y <= -8)
		return false;
	if(p.x == 7 && p.y == -5)
		return false;
	if(p.x == 6 && p.y == -11)
		return false;
	if(p.x >= 7 && p.y == -1)
		return false;
	if(p.x == 11 && p.y >= -6)
		return false;
	if(p.x == 10 && p.y == -2)
		return false;
	return true;
}

ld37_tank ld37_tank_update(ld37_tank tank, whitgl_int input_dir)
{
	tank.just_arrived = false;
	tank.play_sound = false;
	if(tank.transition > 0)
	{
		tank.transition -= 1/12.0;
		if(tank.transition <= 0)
		{
			if(tank.next.facing == tank.current.facing)
				tank.just_arrived = true;
			tank.current = tank.next;
		}
		return tank;
	}
	tank.next = tank.current;
	if(input_dir==3)
	{
		tank.next.facing = whitgl_iwrap(tank.current.facing+1,0,4);
		tank.transition = 1;
	}
	if(input_dir==1)
	{
		tank.next.facing = whitgl_iwrap(tank.current.facing-1,0,4);
		tank.transition = 1;
	}
	if(input_dir==0)
	{
		tank.next.pos = whitgl_ivec_add(tank.current.pos, whitgl_facing_to_ivec(tank.current.facing));
		tank.transition = 1;
	}
	if(input_dir==2)
	{
		tank.next.pos = whitgl_ivec_add(tank.current.pos, whitgl_ivec_inverse(whitgl_facing_to_ivec(tank.current.facing)));
		tank.transition = 1;
	}
	if(!ld37_tank_valid(tank.next.pos))
	{
		tank.next.pos = tank.current.pos;
		tank.transition = 0;
	}
	if(tank.transition == 1)
		tank.play_sound = true;
	return tank;
}
whitgl_fvec3 ld37_tank_3dpos(whitgl_ivec pos2d)
{
	whitgl_fvec current_pos2d = whitgl_ivec_to_fvec(pos2d);
	whitgl_fvec3 pos = {current_pos2d.x, 0.5, current_pos2d.y};
	if(pos2d.x <= 2 && pos2d.y >= -5)
		pos.y += 0.5;
	if(pos2d.x == 3 && pos2d.y >= -5)
		pos.y += 0.5*0.666;
	if(pos2d.x == 4 && pos2d.y >= -5)
		pos.y += 0.5*0.333;
	pos.x -= 5.5;
	pos.z += 5.5;
	pos.z = -pos.z;
	return pos;
}
whitgl_fmat ld37_tank_camera_matrix(ld37_tank tank)
{
	whitgl_fvec3 current_pos = ld37_tank_3dpos(tank.current.pos);
	whitgl_fvec3 next_pos = ld37_tank_3dpos(tank.next.pos);
	whitgl_fvec3 pos = whitgl_fvec3_interpolate(next_pos, current_pos, tank.transition);

	whitgl_fvec3 up = {0,1,0};
	whitgl_float current_angle = whitgl_facing_to_angle(tank.current.facing);
	whitgl_float next_angle = whitgl_facing_to_angle(tank.next.facing);
	whitgl_fvec facing = whitgl_fvec_from_angle(whitgl_angle_lerp(next_angle, current_angle, tank.transition));
	whitgl_fvec3 forwards = {facing.x, 0.2, facing.y};
	whitgl_fvec3 camera_to = whitgl_fvec3_add(pos, forwards);
	whitgl_fmat view = whitgl_fmat_lookAt(pos, camera_to, up);
	return view;
}
