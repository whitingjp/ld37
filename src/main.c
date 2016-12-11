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
#include <tank.h>

const char* model_src = "\
#version 150\
\n\
in vec3 fragmentColor;\
in vec3 fragmentNormal;\
out vec4 outColor;\
uniform float hueShift;\
vec3 rgb2hsv(vec3 c)\
{\
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);\
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));\
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));\
    float d = q.x - min(q.w, q.y);\
    float e = 1.0e-10;\
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);\
}\
vec3 hsv2rgb(vec3 c)\
{\
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);\
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);\
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);\
}\
void main()\
{\
	float r = dot(fragmentNormal, vec3(0.5,1.0,0.25))/2+1;\
	float g = dot(fragmentNormal, vec3(0.6,1.0,0.25))/2+1;\
	float b = dot(fragmentNormal, vec3(0.75,1.0,0.25))/2+1;\
	vec3 col = vec3(r,g,b)*fragmentColor;\
	float overdrive = pow((r+g+b)/3,4);\
	col = col*0.8 + vec3(overdrive)/20;\
	vec3 hsv = rgb2hsv(col);\
	hsv.x -= hueShift*3;\
	vec3 shift = hsv2rgb(hsv);\
	outColor = vec4(shift,1);\
}\
";

#define MAX_DEPTH (6)

int main()
{
	WHITGL_LOG("Starting main.");

	whitgl_sys_setup setup = whitgl_sys_setup_zero;
	setup.size.x = 16*64;
	setup.size.y = 9*64;
	setup.pixel_size = 1;
	setup.name = "main";
	setup.start_focused = false;
	// setup.fullscreen = true;
	setup.cursor = CURSOR_HIDE;

	if(!whitgl_sys_init(&setup))
		return 1;

	whitgl_shader model_shader = whitgl_shader_zero;
	model_shader.fragment_src = model_src;
	model_shader.num_uniforms = 1;
	model_shader.uniforms[0] = "hueShift";

	if(!whitgl_change_shader(WHITGL_SHADER_MODEL, model_shader))
	 	return 1;

	whitgl_sys_color bg = {0xc7,0xb2,0xf6,0xff};
	whitgl_sys_set_clear_color(bg);

	whitgl_sound_init();
	whitgl_input_init();

	// whitgl_sound_add(0, "data/beam.ogg");
	// whitgl_sys_add_image(0, "data/sprites.png");
	whitgl_load_model(0, "data/model/room.wmd");

	whitgl_float width = 5-0.5;
	whitgl_float height = 3-0.5;
	whitgl_float bottom = (3-height)/2;
	whitgl_float right = 5.75-width;

	ld37_debug_camera debug_camera = ld37_debug_camera_zero;
	ld37_tank tanks[MAX_DEPTH];
	whitgl_int i;
	for(i=0; i<MAX_DEPTH; i++)
		tanks[i] = ld37_tank_zero;

	whitgl_int input_queue = -1;

	whitgl_timer_init();
	bool running = true;
	while(running)
	{
		whitgl_sound_update();

		whitgl_timer_tick();
		while(whitgl_timer_should_do_frame(60))
		{
			whitgl_input_update();
			if(whitgl_input_pressed(WHITGL_INPUT_UP)) input_queue = 0;
			if(whitgl_input_pressed(WHITGL_INPUT_RIGHT)) input_queue = 1;
			if(whitgl_input_pressed(WHITGL_INPUT_DOWN)) input_queue = 2;
			if(whitgl_input_pressed(WHITGL_INPUT_LEFT)) input_queue = 3;
			debug_camera = ld37_debug_camera_update(debug_camera);
			for(i=0; i<MAX_DEPTH; i++)
			{
				if(i==0)
				{
					whitgl_int input_dir = -1;
					if(tanks[i].transition <= 0 && input_queue > -1)
					{
						input_dir = input_queue;
						input_queue = -1;
					}
					tanks[i] = ld37_tank_update(tanks[i], input_dir);
				}
				else
				{
					whitgl_int input_dir = -1;
					if(tanks[i-1].just_arrived)
					{
						whitgl_ivec pos = tanks[i-1].current.pos;
						if(pos.x == 1 && pos.y==-9) input_dir = 0;
						if(pos.x == 2 && pos.y==-8) input_dir = 1;
						if(pos.x == 3 && pos.y==-9) input_dir = 2;
						if(pos.x == 2 && pos.y==-10) input_dir = 3;
					}
					tanks[i] = ld37_tank_update(tanks[i], input_dir);
				}
			}

			if(whitgl_input_pressed(WHITGL_INPUT_ESC))
				running = false;
			if(whitgl_sys_should_close())
				running = false;
		}

		whitgl_fvec3 pane_verts[4] =
		{
			{-5.999,bottom,width+right},
			{-5.999,bottom,right},
			{-5.999,bottom+height,width+right},
			{-5.999,bottom+height,right},
		};

		whitgl_float fov = whitgl_pi/2;
		whitgl_fmat perspective = whitgl_fmat_perspective(fov, (float)setup.size.x/(float)setup.size.y, 0.01f, 32.0f);
		// whitgl_fmat view = ld37_debug_camera_matrix(debug_camera);

		for(i=0; i<MAX_DEPTH; i++)
		{
			whitgl_fmat view = ld37_tank_camera_matrix(tanks[MAX_DEPTH-i-1]);

			whitgl_sys_draw_init(MAX_DEPTH-1-i);
			whitgl_set_shader_uniform(WHITGL_SHADER_MODEL, 0, (MAX_DEPTH-i-1)*-0.05);
			whitgl_sys_draw_model(0, whitgl_fmat_identity, view, perspective);
			if(i>0)
				whitgl_sys_draw_buffer_pane(MAX_DEPTH-1-i+1, pane_verts, whitgl_fmat_identity, view, perspective);
		}

		whitgl_sys_draw_finish();


		if(!whitgl_sys_window_focused())
			usleep(10000);
	}

	whitgl_input_shutdown();
	whitgl_sound_shutdown();

	whitgl_sys_close();

	return 0;
}
