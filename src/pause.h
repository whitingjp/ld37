#ifndef LD37_PAUSE_H_
#define LD37_PAUSE_H_

#include <whitgl/math3d.h>

typedef struct
{
	whitgl_int selected;
	whitgl_int volume;
	whitgl_bool paused;
	whitgl_bool should_exit;
	whitgl_bool autoplay;
	whitgl_bool can_autoplay;

	whitgl_float transition;
} ld37_pause;

static const ld37_pause ld37_pause_zero = {0,7,false,false,false,false,0};

ld37_pause ld37_pause_update(ld37_pause pause);
void ld37_pause_draw(ld37_pause pause, whitgl_ivec setup_size);

#endif // LD37_PAUSE_H_
