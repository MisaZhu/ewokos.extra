
#include <stdio.h>
#include <iostream>

#define SDL_MAIN_HANDLED
#include <SDL.h>

#define PGL_PREFIX_TYPES

#include "rsw_math.h"
#include "gltools.h"

#define PORTABLEGL_IMPLEMENTATION
#include "portablegl/portablegl.h"

#include "GLObjects.h"
#include "rsw_glframe.h"

#define WIDTH 640
#define HEIGHT 480
#define PIX_FORMAT SDL_PIXELFORMAT_ARGB8888

using namespace std;


// Colors will be defined in main to ensure proper initialization

SDL_Window* window;
SDL_Renderer* ren;
SDL_Texture* tex;

pix_t* bbufpix;

glContext the_Context;

int width, height;

typedef struct My_Uniforms
{
	rsw::mat4 mvp_mat;
	rsw::vec4 color;
} My_Uniforms;

My_Uniforms the_uniforms;

void cleanup();
void setup_context();
int handle_events(GLFrame& camera_frame);


void grass_vs(float* vs_output, pgl_vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms);
void simple_color_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);







int polygon_mode;

int main(int argc, char** argv)
{
	setup_context();

	polygon_mode = 2;

	width = WIDTH;
	height = HEIGHT;

	GLFrame camera(true, rsw::vec3(0, 0.5f, 3.0f));

	GLfloat grass_blade[] =
	{
		-0.3f,  0.0f,
		 0.3f,  0.0f,
		-0.20f, 1.0f,
		 0.1f, 1.3f,
		-0.05f, 2.3f,
		 0.0f,  3.3f
	};


	GLuint vao, buffer;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &buffer);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(grass_blade), grass_blade, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);


	GLuint myshader = pglCreateProgram(grass_vs, simple_color_fs, 0, NULL, GL_FALSE);
	glUseProgram(myshader);

	rsw::mat4 proj_mat, view_mat;
	make_perspective_matrix(proj_mat, DEG_TO_RAD(45), width/float(height), 0.01f, 100.0f);


	pglSetUniform(&the_uniforms);

	// Initialize color in main to ensure proper construction
	rsw::vec4 Green(0.0f, 1.0f, 0.0f, 1.0f);
	the_uniforms.color = Green;

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glViewport(0, 0, width, height);

	glEnable(GL_DEPTH_TEST);
	SDL_SetRelativeMouseMode(SDL_TRUE);



	unsigned int old_time = 0, new_time=0, counter = 0, last_time = SDL_GetTicks();
	while (1) {
		new_time = SDL_GetTicks();
		if (handle_events(camera))
			break;

		//cout << "origin = " << camera.origin << "\n\n";

		new_time = SDL_GetTicks();
		if (new_time - old_time >= 3000) {
			printf("%f FPS\n", counter*1000.0f/((float)(new_time-old_time)));
			old_time = new_time;
			counter = 0;
		}

		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

		for (int i = 0; i < width * height; ++i) {
			bbufpix[i] = 0xFF000000;
		}

		view_mat = camera.get_camera_matrix();
		//cout << view_mat << "\n\n";
		the_uniforms.mvp_mat = proj_mat * view_mat;
		//cout << the_uniforms.mvp_mat << "\n\n";
		glUseProgram(myshader);
		glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 6, 256*256);

		last_time = new_time;
		++counter;

		//Render the scene - flip vertically (OpenGL Y-up vs SDL Y-down)
		int row_size = width * sizeof(pix_t);
		pix_t tmp_row[WIDTH];
		for (int r = 0; r < height / 2; ++r) {
			memcpy(tmp_row, bbufpix + r * width, row_size);
			memcpy(bbufpix + r * width, bbufpix + (height - 1 - r) * width, row_size);
			memcpy(bbufpix + (height - 1 - r) * width, tmp_row, row_size);
		}

		SDL_UpdateTexture(tex, NULL, bbufpix, width * sizeof(pix_t));
		SDL_RenderCopy(ren, tex, NULL, NULL);
		SDL_RenderPresent(ren);
	}

	cleanup();

	return 0;
}




inline int random(int seed, int iterations)
{
	// get rid of undefined behavior, overflow and << a negative
	unsigned int val = seed;
	int n;
	for (n=0; n<iterations; ++n) {
		val = ((val >> 7) ^ (val << 9)) * 15485863;
	}
	return (int)val;
}


rsw::mat4 make_yrot_mat(float angle)
{
	float st = sin(angle);
	float ct = cos(angle);
	return rsw::mat4(rsw::vec4( ct, 0.0,  st, 0.0),
	            rsw::vec4(0.0, 1.0, 0.0, 0.0),
	            rsw::vec4(-st, 0.0,  ct, 0.0),
	            rsw::vec4(0.0, 0.0, 0.0, 1.0));
}

rsw::mat4 make_xrot_mat(float angle)
{
	float st = sin(angle);
	float ct = cos(angle);
	return rsw::mat4(rsw::vec4(1.0, 0.0, 0.0, 0.0),
	            rsw::vec4(0.0,  ct, -st, 0.0),
	            rsw::vec4(0.0,  st,  ct, 0.0),
	            rsw::vec4(0.0, 0.0, 0.0, 1.0));
}


void grass_vs(float* vs_output, pgl_vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
{
	My_Uniforms* u = (My_Uniforms*)uniforms;
	GLint inst = builtins->gl_InstanceID;

	float ox = float(inst >> 8) - 128.0f;
	float oz = float(inst & 0xFF) - 128.0f;

	int num1 = random(inst, 3);
	int num2 = random(num1, 2);

	ox += float(num1 & 0xFF)/128.0f;
	oz += float(num2 & 0xFF)/128.0f;

	float angle1 = float(num2);
	float st = sin(angle1);
	float ct = cos(angle1);

	float vx = vertex_attribs[0].x;
	float vy = vertex_attribs[0].y;

	float rx =  ct * vx + st * vy;
	float rz = -st * vx + ct * vy;

	pgl_vec4 pos = make_v4(rx + ox, vy, rz + oz, 1.0f);

	builtins->gl_Position = mult_m4_v4(*(pgl_mat4*)&u->mvp_mat, pos);
}

void simple_color_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	My_Uniforms* u = (My_Uniforms*)uniforms;
	builtins->gl_FragColor = make_v4(u->color.x, u->color.y, u->color.z, u->color.w);
}

void setup_context()
{
	SDL_SetMainReady();
	if (SDL_Init(SDL_INIT_VIDEO)) {
		printf("SDL_init error: %s\n", SDL_GetError());
		exit(0);
	}

	window = SDL_CreateWindow("Grass", SDL_WINDOWPOS_CENTERED,  SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
	if (!window) {
		printf("Failed to create window\n");
		SDL_Quit();
		exit(0);
	}

	ren = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
	tex = SDL_CreateTexture(ren, PIX_FORMAT, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);

	if (!init_glContext(&the_Context, &bbufpix, WIDTH, HEIGHT)) {
		puts("Failed to initialize glContext");
		exit(0);
	}
}

void cleanup()
{
	free_glContext(&the_Context);

	SDL_DestroyTexture(tex);
	SDL_DestroyRenderer(ren);
	SDL_DestroyWindow(window);

	SDL_Quit();
}

int handle_events(GLFrame& camera_frame)
{
	SDL_Event event;

	static bool key_down[1024] = {false};
	static unsigned int last_time = 0, cur_time;

	while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_KEYUP:
		{
			int sym = event.key.keysym.sym;
			if (sym >= 0 && sym < 1024)
				key_down[sym] = false;
		}
			break;

		case SDL_KEYDOWN:
		{
			int sym = event.key.keysym.sym;
			if (sym >= 0 && sym < 1024)
				key_down[sym] = true;

			if (sym == SDLK_ESCAPE) {
				return 1;
			} else if (sym == SDLK_p) {
				polygon_mode = (polygon_mode + 1) % 3;
				if (polygon_mode == 0)
					glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
				else if (polygon_mode == 1)
					glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				else
					glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			}
		}
			break;

		case SDL_MOUSEMOTION:
		{
			float degx = event.motion.xrel/20.0f;
			float degy = event.motion.yrel/20.0f;

			camera_frame.rotate_local_y(DEG_TO_RAD(-degx));
			camera_frame.rotate_local_x(DEG_TO_RAD(degy));
		} break;

		case SDL_WINDOWEVENT:
			switch (event.window.event) {
			case SDL_WINDOWEVENT_SIZE_CHANGED:
				width = event.window.data1;
				height = event.window.data2;

				pglResizeFramebuffer(width, height);
				bbufpix = (pix_t*)pglGetBackBuffer();
				glViewport(0, 0, width, height);
				SDL_DestroyTexture(tex);
				tex = SDL_CreateTexture(ren, PIX_FORMAT, SDL_TEXTUREACCESS_STREAMING, width, height);
				break;
			}
			break;

		case SDL_QUIT:
			return 1;
		}
	}

	cur_time = SDL_GetTicks();
	float time = (cur_time - last_time)/1000.0f;

	const Uint8 *state = SDL_GetKeyboardState(NULL);

#define MOVE_SPEED 20

	if (key_down[SDLK_a]) {
		camera_frame.move_right(time * MOVE_SPEED);
	}
	if (key_down[SDLK_d]) {
		camera_frame.move_right(time * -MOVE_SPEED);
	}
	if (state[SDL_SCANCODE_LSHIFT]) {
		camera_frame.move_up(time * MOVE_SPEED);
	}
	if (state[SDL_SCANCODE_SPACE]) {
		camera_frame.move_up(time * -MOVE_SPEED);
	}
	if (key_down[SDLK_w]) {
		camera_frame.move_forward(time*20);
	}
	if (key_down[SDLK_s]) {
		camera_frame.move_forward(time*-20);
	}
	if (key_down[SDLK_q]) {
		camera_frame.rotate_local_z(DEG_TO_RAD(-60*time));
	}
	if (key_down[SDLK_e]) {
		camera_frame.rotate_local_z(DEG_TO_RAD(60*time));
	}

	last_time = cur_time;

	return 0;
}

