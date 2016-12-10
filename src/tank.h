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
} ld37_tank;

static const ld37_tank ld37_tank_zero = {{{0,0},0},{{0,0},0},0};

ld37_tank ld37_tank_update(ld37_tank tank, whitgl_bool input_dirs[4]);
whitgl_fmat ld37_tank_camera_matrix(ld37_tank tank);

#endif // LD37_TANK_H_
