#include <stdbool.h>
#include <stddef.h>
#include <unistd.h>

#include <whitgl/input.h>
#include <whitgl/logging.h>
#include <whitgl/math.h>
#include <whitgl/random.h>
#include <whitgl/sound.h>
#include <whitgl/sys.h>
#include <whitgl/timer.h>

#include <debug_camera.h>

const char* model_src = "\
#version 150\
\n\
in vec3 fragmentColor;\
in vec3 fragmentNormal;\
out vec4 outColor;\
void main()\
{\
	float r = dot(fragmentNormal, vec3(0.5,1.0,0.25))/2+1;\
	float g = dot(fragmentNormal, vec3(0.6,1.0,0.25))/2+1;\
	float b = dot(fragmentNormal, vec3(0.75,1.0,0.25))/2+1;\
	vec3 col = vec3(r,g,b)*fragmentColor;\
	outColor = vec4(col,1);\
}\
";

void draw(ld37_debug_camera debug_camera, whitgl_ivec setup_size)
{
	whitgl_float fov = whitgl_pi/2;
	whitgl_fmat perspective = whitgl_fmat_perspective(fov, (float)setup_size.x/(float)setup_size.y, 0.1f, 32.0f);
	whitgl_fmat view = ld37_debug_camera_matrix(debug_camera);
	whitgl_sys_draw_model(0, whitgl_fmat_identity, view, perspective);
}

int main()
{
	WHITGL_LOG("Starting main.");

	whitgl_sys_setup setup = whitgl_sys_setup_zero;
	setup.size.x = 16*64;
	setup.size.y = 9*64;
	setup.pixel_size = 1;
	setup.name = "main";
	setup.start_focused = false;

	if(!whitgl_sys_init(&setup))
		return 1;

	whitgl_shader model_shader = whitgl_shader_zero;
	model_shader.fragment_src = model_src;

	if(!whitgl_change_shader(WHITGL_SHADER_MODEL, model_shader))
	 	return 1;

	whitgl_sys_color bg = {0xc7,0xb2,0xf6,0xff};
	whitgl_sys_set_clear_color(bg);

	whitgl_sound_init();
	whitgl_input_init();

	// whitgl_sound_add(0, "data/beam.ogg");
	// whitgl_sys_add_image(0, "data/sprites.png");
	whitgl_load_model(0, "data/model/room.wmd");
	whitgl_load_model(1, "data/model/plane.wmd");

	whitgl_float width = 5-0.5;
	whitgl_float height = 3-0.5;
	whitgl_float bottom = (3-height)/2;
	whitgl_float right = 5.75-width;

	ld37_debug_camera debug_camera = ld37_debug_camera_zero;
	debug_camera.pos.x = -3;
	debug_camera.pos.y = 0.5;
	debug_camera.pos.z = right+width/2;
	debug_camera.yaw = whitgl_pi/2;

	whitgl_timer_init();
	bool running = true;
	while(running)
	{
		whitgl_sound_update();

		whitgl_timer_tick();
		while(whitgl_timer_should_do_frame(60))
		{
			whitgl_input_update();
			debug_camera = ld37_debug_camera_update(debug_camera);
			if(whitgl_input_pressed(WHITGL_INPUT_ESC))
				running = false;
			if(whitgl_sys_should_close())
				running = false;
		}

		whitgl_sys_draw_init(1);

		draw(debug_camera, setup.size);

		whitgl_sys_draw_init(0);

		// draw(debug_camera, setup.size);
		whitgl_float fov = whitgl_pi/2;

		whitgl_fmat perspective = whitgl_fmat_perspective(fov, (float)setup.size.x/(float)setup.size.y, 0.1f, 32.0f);

		whitgl_fmat view = ld37_debug_camera_matrix(debug_camera);
		whitgl_sys_draw_model(0, whitgl_fmat_identity, view, perspective);


		whitgl_fvec3 pane_verts[4] =
		{
			{-5.999,bottom,width+right},
			{-5.999,bottom,right},
			{-5.999,bottom+height,width+right},
			{-5.999,bottom+height,right},
		};
		whitgl_sys_draw_buffer_pane(1, pane_verts, whitgl_fmat_identity, view, perspective);
		// whitgl_sys_draw_buffer_pane(1, whitgl_fmat_identity, view, perspective);


		whitgl_sys_draw_finish();


		if(!whitgl_sys_window_focused())
			usleep(10000);
	}

	whitgl_input_shutdown();
	whitgl_sound_shutdown();

	whitgl_sys_close();

	return 0;
}
