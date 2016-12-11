#ifndef LD37_TANK_H_
#define LD37_TANK_H_

#include <whitgl/math3d.h>

typedef struct
{
	whitgl_ivec pos;
	whitgl_int facing;
} ld37_location;

typedef struct
{
	ld37_location next;
	ld37_location current;
	whitgl_float transition;
	whitgl_bool just_arrived;
	whitgl_bool play_sound;
	whitgl_int autostep;
	whitgl_bool autoinplace;
	whitgl_int steps;
} ld37_tank;

static const ld37_tank ld37_tank_zero = {{{2,-3},1},{{2,-3},1},0,false,false,0,false,0};

ld37_tank ld37_tank_update(ld37_tank tank, whitgl_int input_dir);
whitgl_fmat ld37_tank_camera_matrix(ld37_tank tank);

#endif // LD37_TANK_H_
