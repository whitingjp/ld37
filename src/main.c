#include <stdbool.h>
#include <stddef.h>

#include <whitgl/input.h>
#include <whitgl/logging.h>
#include <whitgl/math.h>
#include <whitgl/random.h>
#include <whitgl/sound.h>
#include <whitgl/sys.h>
#include <whitgl/timer.h>


int main()
{
	WHITGL_LOG("Starting main.");

	whitgl_sys_setup setup = whitgl_sys_setup_zero;
	setup.size.x = 32;
	setup.size.y = 32;
	setup.pixel_size = 16;
	setup.name = "main";

	if(!whitgl_sys_init(&setup))
		return 1;

	whitgl_sys_color bg = {0xc7,0xb2,0xf6,0xff};
	whitgl_sys_set_clear_color(bg);

	whitgl_sound_init();
	whitgl_input_init();

	// whitgl_sound_add(0, "data/beam.ogg");
	// whitgl_sys_add_image(0, "data/sprites.png");
	// whitgl_load_model(0, "data/torus.wmd");

	whitgl_timer_init();
	bool running = true;
	while(running)
	{
		whitgl_sound_update();

		whitgl_timer_tick();
		while(whitgl_timer_should_do_frame(60))
		{
			whitgl_input_update();
			if(whitgl_input_pressed(WHITGL_INPUT_ESC))
				running = false;
			if(whitgl_sys_should_close())
				running = false;
		}

		whitgl_sys_draw_init();

		// whitgl_float fov = whitgl_pi/2;
		// whitgl_fmat perspective = whitgl_fmat_perspective(fov, (float)setup.size.x/(float)setup.size.y, 0.1f, 10.0f);
		// whitgl_fvec3 up = {0,1,0};
		// whitgl_fvec3 camera_pos = {0,0,-2};
		// whitgl_fvec3 camera_to = {0,0,0};
		// whitgl_fmat view = whitgl_fmat_lookAt(camera_pos, camera_to, up);
		// whitgl_sys_draw_model(shape, model_matrix, view, perspective);
		whitgl_sys_draw_finish();
	}

	whitgl_input_shutdown();
	whitgl_sound_shutdown();

	whitgl_sys_close();

	return 0;
}
