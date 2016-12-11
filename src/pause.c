#include "pause.h"

#include <whitgl/input.h>
#include <whitgl/logging.h>
#include <whitgl/math.h>
#include <whitgl/random.h>
#include <whitgl/sound.h>
#include <whitgl/sys.h>
#include <whitgl/timer.h>

ld37_pause ld37_pause_update(ld37_pause pause)
{
	if(whitgl_input_pressed(WHITGL_INPUT_UP))
		pause.selected = whitgl_iclamp(pause.selected-1,0,3);
	if(whitgl_input_pressed(WHITGL_INPUT_DOWN))
		pause.selected = whitgl_iclamp(pause.selected+1,0,3);
	if(whitgl_input_pressed(WHITGL_INPUT_ESC))
		pause.paused = !pause.paused;

	if(pause.selected == 1)
	{
		if(whitgl_input_pressed(WHITGL_INPUT_LEFT))
			pause.volume = whitgl_iclamp(pause.volume-1,0,9);
		if(whitgl_input_pressed(WHITGL_INPUT_RIGHT))
			pause.volume = whitgl_iclamp(pause.volume+1,0,9);
	}
	if(whitgl_input_pressed(WHITGL_INPUT_A))
	{
		switch(pause.selected)
		{
			case 0:
				pause.paused = false;
				break;
			case 2:
				pause.autoplay = true;
				break;
			case 3:
				pause.should_exit = true;
				break;
		}
	}
	return pause;
}
void ld37_pause_draw(ld37_pause pause, whitgl_ivec setup_size)
{
	if(!pause.paused)
		return;
	whitgl_sprite text_sprite = {0, {0,0}, {5*16,5*16}};
	whitgl_ivec pause_pos = {setup_size.x/2-(text_sprite.size.x/2.0)*5, 16*2};
	whitgl_sys_draw_text(text_sprite, "pause", pause_pos);

	whitgl_int left = 8*16;
	whitgl_ivec resume_pos = {left, 16*2+7*16};
	whitgl_sys_draw_text(text_sprite, "resume", resume_pos);

	whitgl_ivec volume_pos = {left, 16*2+13*16};
	whitgl_sys_draw_text(text_sprite, "volume", volume_pos);

	whitgl_int i;
	for(i=0; i<9; i++)
	{
		whitgl_int t = 16*2+7*16+6*16;
		whitgl_int left = 16*38+32*i;
		whitgl_iaabb indicator = {{left,t},{left+1*16,t+5*16}};
		whitgl_sys_color faded = {0xff,0xff,0xff,0x40};

		whitgl_sys_color col = i < pause.volume ? whitgl_sys_color_white : faded;
		whitgl_sys_draw_iaabb(indicator, col);
	}

	whitgl_ivec auto_pos = {left, 16*2+19*16};
	whitgl_sys_draw_text(text_sprite, "autoplay", auto_pos);

	whitgl_ivec exit_pos = {left, 16*2+25*16};
	whitgl_sys_draw_text(text_sprite, "exit", exit_pos);

	whitgl_float top = 16*2+7*16+6*16*pause.selected;
	whitgl_iaabb selector = {{32,top},{32+5*16,5*16+top}};
	whitgl_sys_draw_iaabb(selector, whitgl_sys_color_white);
}
