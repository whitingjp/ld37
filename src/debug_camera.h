#ifndef LD37_MATH3D_H_
#define LD37_MATH3D_H_

#include <whitgl/math3d.h>

typedef struct
{
	whitgl_fvec3 pos;
	whitgl_float yaw;
	whitgl_float pitch;
} ld37_debug_camera;

static const ld37_debug_camera ld37_debug_camera_zero = {{-4,0.5,3.5}, whitgl_pi/2, whitgl_pi/4};

ld37_debug_camera ld37_debug_camera_update(ld37_debug_camera camera);
whitgl_fmat ld37_debug_camera_matrix(ld37_debug_camera camera);

#endif // LD37_MATH3D_H_
