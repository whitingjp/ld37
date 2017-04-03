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

#include <capture.h>
#include <debug_camera.h>
#include <tank.h>
#include <pause.h>

const char* model_src = "\
#version 150\
\n\
in vec3 fragmentColor;\
in vec3 fragmentNormal;\
out vec4 outColor;\
void main()\
{\
	float r = dot(fragmentNormal, vec3(-0.5,1.0,-0.25))/2+1;\
	float g = dot(fragmentNormal, vec3(-0.6,1.0,-0.25))/2+1;\
	float b = dot(fragmentNormal, vec3(-0.75,1.0,-0.25))/2+1;\
	vec3 col = vec3(r,g,b)*fragmentColor;\
	float overdrive = pow((r+g+b)/3,4);\
	col = col*0.8 + vec3(overdrive)/20;\
	outColor = vec4(col,1);\
}\
";

const char* texture_src = "\
#version 150\
\n\
in vec2 Texturepos;\
out vec4 outColor;\
uniform sampler2D tex;\
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
	vec4 cola = texture( tex, Texturepos );\
	vec3 col = cola.rgb;\
	vec3 hsv = rgb2hsv(col);\
	hsv.x += 0.12;\
	vec3 shift = hsv2rgb(hsv);\
	outColor = vec4(shift,cola.a);\
}\
";

#define MAX_DEPTH (6)

static const whitgl_int directions_to_mid[] =
{
	0,0,0,0,0,0,0,
	1,
	0,0,0,0,
	3,
	0,0,
	1,
	0,0,0,0,
	1,
	0,0,0,0,
	1,
	0,0,
	3,
	0,0,0,0,0,
	0,2,
	-1,
};
static const whitgl_int automove[4][6] =
{
	{0,2,-1},
	{1,0,2,3,-1},
	{2,0,-1},
	{3,0,2,1,-1},
};

#define MAX_HISTORY (1024)
typedef struct
{
	ld37_location history[MAX_HISTORY][MAX_DEPTH];
	whitgl_float countdown;
	whitgl_bool rewinding;
	whitgl_int step;
	whitgl_float timer;
	whitgl_float rewind_speed;
	whitgl_int depth_recorded;
	whitgl_float reset_trans;
} ld37_rewinder;
static const ld37_rewinder ld37_rewinder_zero = {{}, 0, false, 0, -5, 0, 0, 1};

void record_rewinder(ld37_rewinder* rewinder, ld37_tank tanks[MAX_DEPTH])
{
	whitgl_int i;
	for(i=0; i<MAX_DEPTH; i++)
	{
		rewinder->history[rewinder->step][i] = tanks[i].next;
	}
	rewinder->step++;
	if(rewinder->step >= MAX_HISTORY)
		rewinder->step = MAX_HISTORY-1; // just re-use final one forever
}
void update_rewinder(ld37_rewinder* rewinder, ld37_tank tanks[MAX_DEPTH])
{
	whitgl_bool manual_rewind = false;
	if(whitgl_input_held(WHITGL_INPUT_ESC) || whitgl_input_held(WHITGL_INPUT_Y))
		manual_rewind = true;
	if(whitgl_input_held(WHITGL_INPUT_ANY) && !manual_rewind)
	{
		rewinder->countdown = 0;
		rewinder->timer = -2;
		rewinder->rewind_speed = 0;
	}
	rewinder->countdown += 1.0/(60.0*30);
	whitgl_bool should_rewind = false;
	if(rewinder->countdown > 1)
		should_rewind = true;
	if(manual_rewind)
		should_rewind = true;
	if(rewinder->step <= 1)
	{
		should_rewind = false;
		rewinder->depth_recorded = 0;
		rewinder->rewind_speed = 1;
	}
	if(should_rewind && !rewinder->rewinding)
	{
		record_rewinder(rewinder, tanks);
		rewinder->rewinding = true;
	}
	if(!should_rewind)
		rewinder->rewinding = false;

	if(rewinder->rewinding)
		rewinder->reset_trans = whitgl_fclamp(rewinder->reset_trans-0.2,0,1);
	else
		rewinder->reset_trans = whitgl_fclamp(rewinder->reset_trans+0.2,0,1);

	if(!rewinder->rewinding)
		return;

	rewinder->timer += 1/12.0;
	if(rewinder->step > 5)
		rewinder->rewind_speed = whitgl_fclamp(rewinder->rewind_speed+0.004, 1, 4);
	else
		rewinder->rewind_speed = whitgl_fclamp(rewinder->rewind_speed-0.01, 1, 4);

	while(rewinder->timer > 1)
	{
		rewinder->timer-=1;
		if(rewinder->step <= 1)
		{
			rewinder->step = 1;
			rewinder->rewind_speed = 1;
			rewinder->depth_recorded = 0;
			break;
		}
		rewinder->step--;


		whitgl_int i;
		for(i=0; i<MAX_DEPTH; i++)
		{
			whitgl_int next_step = rewinder->step-1;
			if(next_step < 0)
				next_step = 0;
			tanks[i].current = rewinder->history[rewinder->step][i];
			tanks[i].next = rewinder->history[next_step][i];
			tanks[i].transition = 1;
		}
	}
}

ld37_rewinder rewinder;


whitgl_int get_next_autostep(ld37_tank tanks[MAX_DEPTH], whitgl_int layer)
{
	whitgl_int next = -1;
	if(layer >= MAX_DEPTH)
		return next;
	if(!tanks[layer].autoinplace)
	{
		next = directions_to_mid[tanks[layer].autostep];
		if(layer==0)
			tanks[layer].autostep++;

		if(next == -1)
		{
			tanks[layer].autoinplace = true;
			tanks[layer].autostep = 0;
		}
	} else
	{
		whitgl_int move = get_next_autostep(tanks, layer+1);

		if(move != -1)
		{
			next = automove[move][tanks[layer].autostep];
			if(layer==0)
				tanks[layer].autostep++;
			if(next == -1)
			{
				tanks[layer+1].autostep++;
				tanks[layer].autostep=0;
			}
		}
	}
	return next;
}

int main()
{
	WHITGL_LOG("Starting main.");

	whitgl_bool event_mode = false;

	whitgl_sys_setup setup = whitgl_sys_setup_zero;
	setup.size.x = 16*64;
	setup.size.y = 9*64;
	setup.pixel_size = 1;
	setup.name = "Nest";
	setup.cursor = CURSOR_HIDE;
	setup.fullscreen = true;
	setup.num_framebuffers = 6;

	if(!whitgl_sys_init(&setup))
		return 1;

	whitgl_sys_enable_depth(true);

	whitgl_shader model_shader = whitgl_shader_zero;
	model_shader.fragment_src = model_src;

	if(!whitgl_change_shader(WHITGL_SHADER_MODEL, model_shader))
	 	return 1;

	whitgl_shader texture_shader = whitgl_shader_zero;
	texture_shader.fragment_src = texture_src;
	if(!whitgl_change_shader(WHITGL_SHADER_TEXTURE, texture_shader))
	 	return 1;

	whitgl_sys_color bg = {0x89,0xe0,0xfc,0xff};
	whitgl_sys_set_clear_color(bg);

	whitgl_sound_init();
	whitgl_input_init();

	whitgl_sound_add(0, "data/sound/9.wav");
	whitgl_sound_add(1, "data/sound/10.wav");
	whitgl_sound_add(2, "data/sound/11.wav");
	whitgl_sound_add(3, "data/sound/12.wav");
	whitgl_sound_add(4, "data/sound/13.wav");
	whitgl_sound_add(5, "data/sound/14.wav");
	whitgl_sound_add(6, "data/sound/15.wav");
	whitgl_sound_add(7, "data/sound/16.wav");
	whitgl_sound_add(8, "data/sound/1.wav");
	whitgl_sound_add(9, "data/sound/2.wav");
	whitgl_sound_add(10, "data/sound/3.wav");
	whitgl_sound_add(11, "data/sound/4.wav");
	whitgl_sound_add(12, "data/sound/17.wav");
	whitgl_sound_add(13, "data/sound/18.wav");
	whitgl_sound_add(14, "data/sound/19.wav");
	whitgl_sound_add(15, "data/sound/20.wav");
	whitgl_sound_add(16, "data/sound/5.wav");
	whitgl_sound_add(17, "data/sound/6.wav");
	whitgl_sound_add(18, "data/sound/7.wav");
	whitgl_sound_add(19, "data/sound/8.wav");
	whitgl_sound_add(20, "data/sound/21.wav");
	whitgl_sound_add(21, "data/sound/22.wav");
	whitgl_sound_add(22, "data/sound/23.wav");
	whitgl_sound_add(23, "data/sound/24.wav");


	whitgl_sys_add_image(0, "data/sprites/sprites.png");
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

	whitgl_bool finished = false;
	whitgl_float finish_timer = 0;

	whitgl_timer_init();
	bool running = true;
	whitgl_float fps = 60;

	whitgl_float time = 0;

	whitgl_bool any_pressed = false;
	whitgl_float title_transition = 0;

	ld37_pause pause = ld37_pause_zero;
	// capture_info capture = capture_info_zero; 

	rewinder = ld37_rewinder_zero;
	record_rewinder(&rewinder, tanks);
	while(running)
	{
		whitgl_sound_update();

		whitgl_timer_tick();
		while(whitgl_timer_should_do_frame(fps))
		{
			if(event_mode)
				update_rewinder(&rewinder, tanks);
			fps = 60;
			if(!pause.paused && pause.autoplay && !finished && whitgl_input_held(WHITGL_INPUT_A))
				fps *= 4;
			if(!pause.paused && pause.autoplay && !finished && whitgl_input_held(WHITGL_INPUT_B))
				fps *= 4;
			if(rewinder.rewinding)
			{
				fps = 60*rewinder.rewind_speed;
			}
			if(finished && rewinder.rewinding)
			{
				finished = false;
				finish_timer = 0;
			}
			if(finished && finish_timer > 1.2 && pause.paused)
			{
				finished = false;
				finish_timer = 0;
				whitgl_int i;
				for(i=0; i<MAX_DEPTH; i++)
					tanks[i] = ld37_tank_zero;
			}
			time += 1/60.0;
			whitgl_input_update();
			if(event_mode && rewinder.rewinding)
				any_pressed = false;
			if(whitgl_input_pressed(WHITGL_INPUT_UP))
				any_pressed = true;
			if(whitgl_input_pressed(WHITGL_INPUT_RIGHT))
				any_pressed = true;
			if(whitgl_input_pressed(WHITGL_INPUT_DOWN))
				any_pressed = true;
			if(whitgl_input_pressed(WHITGL_INPUT_LEFT))
				any_pressed = true;
			if(whitgl_input_pressed(WHITGL_INPUT_ESC) && !event_mode)
				any_pressed = true;
			if(any_pressed)
				title_transition = whitgl_fclamp(title_transition+0.1, 0, 1);
			else if(rewinder.step <= 1)
				title_transition = whitgl_fclamp(title_transition-0.2, 0, 1);
			whitgl_bool old_autoplay = pause.autoplay;
			if(!event_mode)
				pause = ld37_pause_update(pause);
			if(!old_autoplay && pause.autoplay)
			{
				whitgl_int i;
				for(i=0; i<MAX_DEPTH; i++)
					tanks[i] = ld37_tank_zero;
			}
			if(whitgl_sys_should_close())
				running = false;
			if(pause.should_exit)
				running = false;
			if(pause.paused || !running)
				continue;
			if(!pause.autoplay)
			{
				whitgl_bool vertical = whitgl_input_held(WHITGL_INPUT_UP) || whitgl_input_held(WHITGL_INPUT_DOWN);
				whitgl_bool horizontal = whitgl_input_held(WHITGL_INPUT_LEFT) || whitgl_input_held(WHITGL_INPUT_RIGHT);

				if(whitgl_input_pressed(WHITGL_INPUT_UP) && !horizontal) input_queue = 0;
				if(whitgl_input_pressed(WHITGL_INPUT_RIGHT) && !vertical) input_queue = 1;
				if(whitgl_input_pressed(WHITGL_INPUT_DOWN) && !horizontal) input_queue = 2;
				if(whitgl_input_pressed(WHITGL_INPUT_LEFT) && !vertical) input_queue = 3;
			} else
			{
				if(input_queue == -1)
				{
					whitgl_int next =  get_next_autostep(tanks, 0);
					if(next != -1)
						input_queue = next;
				}
			}
			debug_camera = ld37_debug_camera_update(debug_camera);
			// whitgl_int old_input_queue = input_queue;
			for(i=0; i<MAX_DEPTH; i++)
			{
				if(finished)
					input_queue = -1;
				if(i==0)
				{
					whitgl_int input_dir = -1;
					if(tanks[i].transition <= 0 && input_queue > -1)
					{
						input_dir = input_queue;
						input_queue = -1;
					}
					tanks[i] = ld37_tank_update(tanks[i], input_dir);
					if(tanks[i].transition == 1 && rewinder.depth_recorded <= i && event_mode)
					{
						rewinder.depth_recorded = i;
						record_rewinder(&rewinder, tanks);
					}
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
					if(tanks[i].transition == 1 && rewinder.depth_recorded <= i && event_mode)
					{
						rewinder.depth_recorded = i;
						record_rewinder(&rewinder, tanks);
					}
				}
				if(tanks[i].play_sound)
				{
					const whitgl_int sample_order[] = {0,1,2,3};
					whitgl_int sound = i*4+sample_order[tanks[i].steps%4];
					whitgl_float volume = (float)(i+1)/(MAX_DEPTH+1);
					whitgl_sound_play(sound, volume/2, 1);
					tanks[i].steps = tanks[i].steps+1;

					break;
				}
			}

			whitgl_int count_stages = 0;
			for(i=0; i<MAX_DEPTH; i++)
			{
				if(tanks[i].current.pos.x > 4) continue;
				if(tanks[i].current.pos.y > -7) continue;
				if(tanks[i].current.facing != 3) continue;
				count_stages++;
			}
			if(count_stages >= 2)
			{
				pause.can_autoplay = true;
			}
			if(count_stages == MAX_DEPTH && !finished)
			{
				finished = true;
				fps = 60;
			}
			if(finished)
			{
				whitgl_float old_timer = finish_timer;
				finish_timer += 1/420.0;
				if(old_timer <= 1 && (int)(finish_timer*MAX_DEPTH*4) != (int)(old_timer*MAX_DEPTH*4))
					whitgl_sound_play(MAX_DEPTH*4-(int)(finish_timer*MAX_DEPTH*4), 1, 1);
				if(old_timer < 1.2 && finish_timer >= 1.2)
				{
					for(i=0; i<MAX_DEPTH; i++)
						whitgl_sound_play(i*4,1,1);
					whitgl_sys_set_clear_color(whitgl_sys_color_black);
				}
			}
	}

		whitgl_fvec3 pane_verts[4] =
		{
			{-5.995,bottom,width+right},
			{-5.995,bottom,right},
			{-5.995,bottom+height,width+right},
			{-5.995,bottom+height,right},
		};

		whitgl_float fov = whitgl_pi/2.25;
		whitgl_fmat perspective = whitgl_fmat_perspective(fov, (float)setup.size.x/(float)setup.size.y, 0.01f, 32.0f);

		whitgl_int cutoff = finish_timer*MAX_DEPTH;
		for(i=0; i<MAX_DEPTH; i++)
		{
			whitgl_fmat view = ld37_tank_camera_matrix(tanks[MAX_DEPTH-i-1]);
			// if(i==MAX_DEPTH-1)
			// 	view = ld37_debug_camera_matrix(debug_camera);

			whitgl_sys_draw_init(MAX_DEPTH-1-i);

			if(i<cutoff)
				continue;
			whitgl_sys_draw_model(0, whitgl_fmat_identity, view, perspective);
			if(i>0)
				whitgl_sys_draw_buffer_pane(MAX_DEPTH-1-i+1, pane_verts, whitgl_fmat_identity, view, perspective);
		}

		ld37_pause_draw(pause, setup.size);

		whitgl_sprite text_sprite = {0, {0,0}, {5*16,5*16}};
		whitgl_float trans = title_transition*title_transition;
		whitgl_ivec nest_pos = {setup.size.x/2-(text_sprite.size.x/2.0)*4+trans*setup.size.x, setup.size.y/4-(text_sprite.size.y/4.0)};
		whitgl_sys_draw_text(text_sprite, "nest", nest_pos);


		whitgl_ivec reset_pos = {setup.size.x/2-(text_sprite.size.x/2.0)*5, (setup.size.y*3)/4-text_sprite.size.y/2+rewinder.reset_trans*rewinder.reset_trans*(setup.size.y/4+text_sprite.size.y)};
		whitgl_sys_draw_text(text_sprite, "reset", reset_pos);

		whitgl_sys_draw_finish();


		//if(!whitgl_sys_window_focused())
		//	usleep(10000);
	}

	whitgl_input_shutdown();
	whitgl_sound_shutdown();

	whitgl_sys_close();

	return 0;
}
