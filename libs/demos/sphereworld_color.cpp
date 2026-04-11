
#define SDL_MAIN_HANDLED
#include <SDL.h>

#define PGL_PREFIX_TYPES
//#define PGL_D16
//#define PGL_RGB565

#define PORTABLEGL_IMPLEMENTATION
#include "portablegl.h"

#include "rsw_math.h"
#include "gltools.h"
#include "GLObjects.h"
#include "rsw_glframe.h"
#include "rsw_primitives.h"
#include "rsw_halfedge.h"
#include "controls.h"


#include <stdio.h>

#include <iostream>
#include <vector>
#include <algorithm>

#define WIDTH 640
#define HEIGHT 480

#define PIX_FORMAT SDL_PIXELFORMAT_ARGB8888


using namespace std;

typedef struct My_Uniforms
{
	rsw::mat4 mvp_mat;
	rsw::mat4 mv_mat;
	rsw::mat3 normal_mat;

	rsw::vec4 color;

	rsw::vec3 light_dir;

	rsw::vec3 Ka;           // Ambient reflectivity
	rsw::vec3 Kd;           // Diffuse reflectivity
	rsw::vec3 Ks;           // Specular reflectivity
	float shininess;   // Specular shininess factor

	rsw::vec3 light_pos;
	rsw::vec4 modulate;
} My_Uniforms;

void cleanup();
void setup_context();
int handle_events(GLFrame& camera_frame, unsigned int last_time, unsigned int cur_time);

void basic_transform_vp(float* vs_output, pgl_vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms);
void uniform_color_fp(float* fs_input, Shader_Builtins* builtins, void* uniforms);

void gouraud_ads_vp(float* vs_output, pgl_vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms);
void gouraud_ads_fp(float* fs_input, Shader_Builtins* builtins, void* uniforms);

void phong_ads_vp(float* vs_output, pgl_vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms);
void phong_ads_fp(float* fs_input, Shader_Builtins* builtins, void* uniforms);


enum
{
	ATTR_VERTEX = 0,
	ATTR_NORMAL = 1,
	ATTR_TEXCOORD = 2,
	ATTR_INSTANCE = 3
};

SDL_Window* window;
SDL_Renderer* ren;
SDL_Texture* tex;
glContext the_Context;
pix_t* bbufpix;

int width, height;
float fov, zmin, zmax;
rsw::mat4 proj_mat;





struct Mesh
{
	vector<rsw::vec3> verts;
	vector<rsw::ivec3> tris;
	vector<rsw::vec3> normals;

	vector<rsw::vec2> texcoords;

	half_edge_data he_data;
};


struct vert_attribs
{
	rsw::vec3 pos;
	rsw::vec3 normal;

	vert_attribs(rsw::vec3 p, rsw::vec3 n) : pos(p), normal(n) {}
};

#define NUM_SHADERS 2

int polygon_mode;
GLuint shaders[NUM_SHADERS];
int cur_shader;
My_Uniforms the_uniforms;


enum Control_Names {
	LEFT=0,
	RIGHT,
	FORWARD,
	BACK,
	UP,
	DOWN,
	TILTLEFT,
	TILTRIGHT,

	HIDECURSOR,
	FOVUP,
	FOVDOWN,
	ZMINUP,
	ZMINDOWN,
	PROVOKING,
	INTERPOLATION,
	SHADER,
	DEPTHTEST,
	POLYGONMODE,

	NCONTROLS
};


SDL_Scancode controls[NCONTROLS] =
{
	SDL_SCANCODE_A,
	SDL_SCANCODE_D,
	SDL_SCANCODE_W,
	SDL_SCANCODE_S,
	SDL_SCANCODE_LSHIFT,
	SDL_SCANCODE_SPACE,
	SDL_SCANCODE_Q,
	SDL_SCANCODE_E
};


#define FLOOR_SIZE 40



int main(int argc, char** argv)
{
	setup_context();

	polygon_mode = 2;
	fov = 35;
	zmin = 0.5;
	zmax = 100;

	vector<rsw::vec3> line_verts;
	for (int i=0, j=-FLOOR_SIZE/2; i < 11; ++i, j+=FLOOR_SIZE/10) {
		line_verts.push_back(rsw::vec3(j, -1, -FLOOR_SIZE/2));
		line_verts.push_back(rsw::vec3(j, -1, FLOOR_SIZE/2));
		line_verts.push_back(rsw::vec3(-FLOOR_SIZE/2, -1, j));
		line_verts.push_back(rsw::vec3(FLOOR_SIZE/2, -1, j));
	}

	GLuint line_vao, line_buf;
	glGenVertexArrays(1, &line_vao);
	glBindVertexArray(line_vao);

	glGenBuffers(1, &line_buf);
	glBindBuffer(GL_ARRAY_BUFFER, line_buf);
	glBufferData(GL_ARRAY_BUFFER, line_verts.size()*3*sizeof(float), &line_verts[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(ATTR_VERTEX);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	Mesh torus;
	Mesh sphere;

	make_torus(torus.verts, torus.tris, torus.texcoords, 0.3, 0.1, 40, 20);
	make_sphere(sphere.verts, sphere.tris, sphere.texcoords, 0.1, 26, 13);

	rsw::mat3 rot_x;
	load_rotation_mat3(rot_x, rsw::vec3(1, 0, 0), DEG_TO_RAD(90));
	for (int i=0; i<torus.verts.size(); ++i) {
		torus.verts[i] = rot_x * torus.verts[i];
	}

	compute_normals(torus.verts, torus.tris, NULL, DEG_TO_RAD(30), torus.normals);
	compute_normals(sphere.verts, sphere.tris, NULL, DEG_TO_RAD(30), sphere.normals);


	vector<vert_attribs> vert_data;

	int v;
	for (int i=0, j=0; i<torus.tris.size(); ++i, j+=3) {
		v = torus.tris[i].x;
		vert_data.push_back(vert_attribs(torus.verts[v], torus.normals[j]));

		v = torus.tris[i].y;
		vert_data.push_back(vert_attribs(torus.verts[v], torus.normals[j+1]));

		v = torus.tris[i].z;
		vert_data.push_back(vert_attribs(torus.verts[v], torus.normals[j+2]));
	}

	for (int i=0, j=0; i<sphere.tris.size(); ++i, j+=3) {
		v = sphere.tris[i].x;
		vert_data.push_back(vert_attribs(sphere.verts[v], sphere.normals[j]));

		v = sphere.tris[i].y;
		vert_data.push_back(vert_attribs(sphere.verts[v], sphere.normals[j+1]));

		v = sphere.tris[i].z;
		vert_data.push_back(vert_attribs(sphere.verts[v], sphere.normals[j+2]));
	}

#define NUM_SPHERES 50
	vector<rsw::vec3> instance_pos;
	rsw::vec2 rand_pos;
	for (int i=0; i<NUM_SPHERES+1; ++i) {
		rand_pos = rsw::vec2(rsw::randf_range(-FLOOR_SIZE/2.0f, FLOOR_SIZE/2.0f), rsw::randf_range(-FLOOR_SIZE/2.0f, FLOOR_SIZE/2.0f));
		if (i)
			instance_pos.push_back(rsw::vec3(rand_pos.x, 0.4, rand_pos.y));
		else
			instance_pos.push_back(rsw::vec3());
	}




	GLuint vao, buffer;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &buffer);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);

	size_t total_size = (torus.tris.size()*3 + sphere.tris.size()*3) * sizeof(vert_attribs);
	size_t sphere_offset = torus.tris.size()*3;
	glBufferData(GL_ARRAY_BUFFER, total_size, &vert_data[0], GL_STATIC_DRAW);

	glEnableVertexAttribArray(ATTR_VERTEX);
	glVertexAttribPointer(ATTR_VERTEX, 3, GL_FLOAT, GL_FALSE, sizeof(vert_attribs), 0);
	glEnableVertexAttribArray(ATTR_NORMAL);
	pglVertexAttribPointer(ATTR_NORMAL, 3, GL_FLOAT, GL_FALSE, sizeof(vert_attribs), sizeof(rsw::vec3));

	GLuint inst_buf;
	glGenBuffers(1, &inst_buf);
	glBindBuffer(GL_ARRAY_BUFFER, inst_buf);
	glBufferData(GL_ARRAY_BUFFER, instance_pos.size()*3*sizeof(float), &instance_pos[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(ATTR_INSTANCE);
	glVertexAttribPointer(ATTR_INSTANCE, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glVertexAttribDivisor(ATTR_INSTANCE, 1);



	GLenum interpolation[5] = { PGL_SMOOTH3 };

	GLuint basic_shader = pglCreateProgram(basic_transform_vp, uniform_color_fp, 0, NULL, GL_FALSE);
	glUseProgram(basic_shader);
	pglSetUniform(&the_uniforms);

	shaders[0] = pglCreateProgram(gouraud_ads_vp, gouraud_ads_fp, 3, interpolation, GL_FALSE);

	glUseProgram(shaders[0]);
	pglSetUniform(&the_uniforms);

	shaders[1] = pglCreateProgram(phong_ads_vp, phong_ads_fp, 3, interpolation, GL_FALSE);
	glUseProgram(shaders[1]);
	pglSetUniform(&the_uniforms);


	cur_shader = 0;


	rsw::mat4 view_mat;
	rsw::mat4 mvp_mat;
	rsw::mat3 normal_mat;
	rsw::mat4 translate_sphere = rsw::translation_mat4(rsw::vec3(0.8f, 0.4f, 0.0f));
	rsw::mat4 translate_torus_back = rsw::translation_mat4(rsw::vec3(0.0f, 0.3f, -1.2f));

	rsw::make_perspective_matrix(proj_mat, DEG_TO_RAD(35.0f), WIDTH/(float)HEIGHT, 0.3f, 100.0f);



	rsw::vec4 floor_color(0, 1, 0, 1);

	rsw::vec3 torus_ambient(0.0, 0, 0);
	rsw::vec3 torus_diffuse(1.0, 0, 0);
	rsw::vec3 torus_specular(0, 0, 0);

	rsw::vec3 sphere_ambient(0, 0, 0.2);
	rsw::vec3 sphere_diffuse(0, 0, 0.7);
	rsw::vec3 sphere_specular(1, 1, 1);


	// don't actually need to set the shader before modifying uniforms like in
	// real OpenGL
	//glUseProgram(basic_shader);
	the_uniforms.color = floor_color;

	the_uniforms.shininess = 128.0f;


	rsw::vec3 light_direction(0, 10, 5);


	glUseProgram(basic_shader);

	GLFrame camera(true, rsw::vec3(0, 0.3, 2.5));


	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	
	SDL_SetRelativeMouseMode(SDL_TRUE);

	unsigned int old_time = 0, new_time=0, counter = 0, last_time = SDL_GetTicks();
	float total_time;
	while (1) {
		new_time = SDL_GetTicks();
		if (handle_events(camera, last_time, new_time))
			break;

		last_time = new_time;
		total_time = new_time/1000.0f;
		if (new_time - old_time > 3000) {
			printf("%f FPS\n", counter*1000.f/(new_time-old_time));
			old_time = new_time;
			counter = 0;
		}

		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

		for (int i = 0; i < width * height; ++i) {
			bbufpix[i] = 0xFF000000;
		}


		rsw::mat4 view_mat = camera.get_camera_matrix();
		rsw::mat4 mvp_mat = proj_mat * view_mat;


		glUseProgram(basic_shader);
		the_uniforms.mvp_mat = mvp_mat;
		glBindVertexArray(line_vao);
		glDrawArrays(GL_LINES, 0, line_verts.size());

		

		glBindVertexArray(vao);

		glUseProgram(shaders[cur_shader]);


		the_uniforms.light_dir = rsw::mat3(view_mat)*light_direction;

		the_uniforms.normal_mat = rsw::mat3(view_mat);
		the_uniforms.Ka = sphere_ambient;
		the_uniforms.Kd = sphere_diffuse;
		the_uniforms.Ks = sphere_specular;

		glDrawArraysInstancedBaseInstance(GL_TRIANGLES, sphere_offset, sphere.tris.size()*3, NUM_SPHERES, 1);

		rsw::mat4 rot_mat;
		rsw::load_rotation_mat4(rot_mat, rsw::vec3(0, 1, 0), -1*total_time*DEG_TO_RAD(60.0f));

		the_uniforms.mvp_mat = mvp_mat * rot_mat * translate_sphere;
		the_uniforms.normal_mat = rsw::mat3(view_mat*rot_mat);

		glDrawArrays(GL_TRIANGLES, sphere_offset, sphere.tris.size()*3);


		//draw rotating torus
		mvp_mat = proj_mat * view_mat;
		rsw::load_rotation_mat4(rot_mat, rsw::vec3(0, 1, 0), total_time*DEG_TO_RAD(60.0f));
		the_uniforms.mvp_mat = mvp_mat * rot_mat * translate_torus_back;
		the_uniforms.normal_mat = rsw::mat3(view_mat*rot_mat);

		the_uniforms.Ka = torus_ambient;
		the_uniforms.Kd = torus_diffuse;
		the_uniforms.Ks = torus_specular;

		glDrawArrays(GL_TRIANGLES, 0, torus.tris.size()*3);

		SDL_UpdateTexture(tex, NULL, bbufpix, width * sizeof(pix_t));
		SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
		SDL_RenderFillRect(ren, NULL);
		SDL_RenderCopy(ren, tex, NULL, NULL);
		SDL_RenderPresent(ren);

		last_time = new_time;
		++counter;
	}

	cleanup();

	return 0;
}


void setup_context()
{
	if (SDL_Init(SDL_INIT_VIDEO)) {
		printf("SDL_Init error: %s\n", SDL_GetError());
		exit(0);
	}

	// TODO resizable
	width = WIDTH;
	height = HEIGHT;

	window = SDL_CreateWindow("Sphereworld Color", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
	if (!window) {
		cleanup();
		exit(0);
	}

	ren = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
	tex = SDL_CreateTexture(ren, PIX_FORMAT, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
	//tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);

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

int handle_events(GLFrame& camera_frame, unsigned int last_time, unsigned int cur_time)
{
	SDL_Event event;
	int sc;
	bool remake_projection = false;

	while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_KEYDOWN:
			sc = event.key.keysym.sym;

			if (sc == SDLK_ESCAPE) {
				return 1;
			} else if (sc == SDLK_p) {
				polygon_mode = (polygon_mode + 1) % 3;
				if (polygon_mode == 0)
					glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
				else if (polygon_mode == 1)
					glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				else
					glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			} else if (sc == SDLK_l) {
				// gouraud is actually slower than phong, probably because
				// vs shader runs for everything, fs only for pixels that
				// isn't clipped or culled (z test does happen after though)
				// and the vertices outnumber that
				cur_shader++;
				cur_shader %= NUM_SHADERS;
			}
			break;

		case SDL_WINDOWEVENT:
			switch (event.window.event) {
			case SDL_WINDOWEVENT_RESIZED:
				printf("window size %d x %d\n", event.window.data1, event.window.data2);
				width = event.window.data1;
				height = event.window.data2;

				remake_projection = true;

				pglResizeFramebuffer(width, height);
				bbufpix = (pix_t*)pglGetBackBuffer();
				glViewport(0, 0, width, height);
				SDL_DestroyTexture(tex);
				tex = SDL_CreateTexture(ren, PIX_FORMAT, SDL_TEXTUREACCESS_STREAMING, width, height);
				break;
			}
			break;

		case SDL_MOUSEMOTION:
		{
			float degx = event.motion.xrel/20.0f;
			float degy = event.motion.yrel/20.0f;
			
			camera_frame.rotate_local_y(DEG_TO_RAD(-degx));
			camera_frame.rotate_local_x(DEG_TO_RAD(degy));
		}
			break;

		case SDL_QUIT:
			return 1;

		}
	}



	//SDL_PumpEvents() is called above in SDL_PollEvent()
	const Uint8 *state = SDL_GetKeyboardState(NULL);

	float time = (cur_time - last_time)/1000.0f;

#define MOVE_SPEED 5
	
	if (state[controls[LEFT]]) {
		camera_frame.move_right(time * MOVE_SPEED);
	}
	if (state[controls[RIGHT]]) {
		camera_frame.move_right(time * -MOVE_SPEED);
	}
	if (state[controls[UP]]) {
		camera_frame.move_up(time * MOVE_SPEED);
	}
	if (state[controls[DOWN]]) {
		camera_frame.move_up(time * -MOVE_SPEED);
	}
	if (state[controls[FORWARD]]) {
		camera_frame.move_forward(time * MOVE_SPEED);
	}
	if (state[controls[BACK]]) {
		camera_frame.move_forward(time * -MOVE_SPEED);
	}
	if (state[controls[TILTLEFT]]) {
		camera_frame.rotate_local_z(DEG_TO_RAD(-60*time));
	}
	if (state[controls[TILTRIGHT]]) {
		camera_frame.rotate_local_z(DEG_TO_RAD(60*time));
	}
	/*
	if (state[controls[FOVUP]]) {
		if (state[SDL_SCANCODE_LCTRL] || state[SDL_SCANCODE_RCTRL]) {
			if (zmax < 2000)
				zmax += 1;
		} else {
			if (fov < 170)
				fov += 0.2;
		}
		printf("zmax = %f\nfov = %f\n", zmax, fov);
		remake_projection = true;
	}
	if (state[controls[FOVDOWN]]) {
		if (state[SDL_SCANCODE_LCTRL] || state[SDL_SCANCODE_RCTRL]) {
			if (zmax > 20)
				zmax -= 1;
		} else {
			if (fov > 10)
				fov -= 0.2;
		}
		printf("zmax = %f\nfov = %f\n", zmax, fov);
		remake_projection = true;
	}
	if (state[controls[ZMINUP]]) {
		if (zmin < 50) {
			zmin += 0.1;
			remake_projection = true;
			printf("zmin = %f\n", zmin);
		}
	}
	if (state[controls[ZMINDOWN]]) {
		if (zmin > 0.5) {
			zmin -= 0.1;
			remake_projection = true;
			printf("zmin = %f\n", zmin);
		}
	}
	*/

	if (remake_projection) {
		rsw::make_perspective_matrix(proj_mat, DEG_TO_RAD(fov), float(width)/float(height), zmin, zmax);
		remake_projection = false;
	}


	return 0;
}


void basic_transform_vp(float* vs_output, pgl_vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
{
	My_Uniforms* u = (My_Uniforms*)uniforms;
	builtins->gl_Position = mult_m4_v4(*(pgl_mat4*)&u->mvp_mat, vertex_attribs[0]);
}

void uniform_color_fp(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	My_Uniforms* u = (My_Uniforms*)uniforms;
	builtins->gl_FragColor = make_v4(u->color.x, u->color.y, u->color.z, u->color.w);
}

void gouraud_ads_vp(float* vs_output, pgl_vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
{
	pgl_vec4* vert_attribs = vertex_attribs;
	My_Uniforms* u = (My_Uniforms*)uniforms;

	// Get surface normal in eye coordinates
	pgl_vec3 eye_normal = mult_m3_v3((float*)&u->normal_mat, make_v3(vert_attribs[ATTR_NORMAL].x, vert_attribs[ATTR_NORMAL].y, vert_attribs[ATTR_NORMAL].z));
	
	//non-local viewer and constant directional light
	pgl_vec3 light_dir = make_v3(u->light_dir.x, u->light_dir.y, u->light_dir.z);
	normalize_v3(&light_dir);
	pgl_vec3 eye_dir = make_v3(0, 0, 1);
	
	//as if all lights are white TODO
	pgl_vec3 out_light = make_v3(u->Ka.x, u->Ka.y, u->Ka.z);
	
	// Dot product gives us diffuse intensity
	float diff = max(0.0f, dot_v3s(eye_normal, light_dir));

	// add diffuse light
	out_light = add_v3s(out_light, scale_v3(make_v3(u->Kd.x, u->Kd.y, u->Kd.z), diff));

	// Specular Light
	pgl_vec3 r = reflect_v3(neg_v3(light_dir), eye_normal);
	float spec = max(0.0f, dot_v3s(eye_dir, r));
	if(diff > 0) {
		float fSpec = pow(spec, u->shininess);
		out_light = add_v3s(out_light, scale_v3(make_v3(u->Ks.x, u->Ks.y, u->Ks.z), fSpec));
	}
	
	vs_output[0] = out_light.x;
	vs_output[1] = out_light.y;
	vs_output[2] = out_light.z;
	
	// Don't forget to transform the geometry!
	pgl_vec4 tmp = make_v4(vert_attribs[ATTR_INSTANCE].x, vert_attribs[ATTR_INSTANCE].y, vert_attribs[ATTR_INSTANCE].z, 0);
	builtins->gl_Position = mult_m4_v4(*(pgl_mat4*)&u->mvp_mat, add_v4s(vert_attribs[ATTR_VERTEX], tmp));
}

void gouraud_ads_fp(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	pgl_vec4 color = { fs_input[0], fs_input[1], fs_input[2], 1 };
	builtins->gl_FragColor = color;
}


void phong_ads_vp(float* vs_output, pgl_vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
{
	pgl_vec4* vert_attribs = vertex_attribs;
	My_Uniforms* u = (My_Uniforms*)uniforms;

	pgl_vec3 eye_normal = mult_m3_v3((float*)&u->normal_mat, make_v3(vert_attribs[ATTR_NORMAL].x, vert_attribs[ATTR_NORMAL].y, vert_attribs[ATTR_NORMAL].z));
	vs_output[0] = eye_normal.x;
	vs_output[1] = eye_normal.y;
	vs_output[2] = eye_normal.z;

	pgl_vec4 tmp = make_v4(vert_attribs[ATTR_INSTANCE].x, vert_attribs[ATTR_INSTANCE].y, vert_attribs[ATTR_INSTANCE].z, 0);
	builtins->gl_Position = mult_m4_v4(*(pgl_mat4*)&u->mvp_mat, add_v4s(vert_attribs[ATTR_VERTEX], tmp));
}


void phong_ads_fp(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	My_Uniforms* u = (My_Uniforms*)uniforms;
	
	//non-local viewer
	pgl_vec3 eye_dir = make_v3(0, 0, 1);

	pgl_vec3 eye_normal = make_v3(fs_input[0], fs_input[1], fs_input[2]);

	pgl_vec3 s = make_v3(u->light_dir.x, u->light_dir.y, u->light_dir.z);
	normalize_v3(&s);
	pgl_vec3 n = eye_normal;
	normalize_v3(&n);
	pgl_vec3 v = eye_dir;

	// add ambient
	pgl_vec3 out_light = make_v3(u->Ka.x, u->Ka.y, u->Ka.z);
	
	// Dot product gives us diffuse intensity
	float lambertian = max(0.0f, dot_v3s(s, n));
	if (lambertian > 0) {

		// add diffuse light
		out_light = add_v3s(out_light, scale_v3(make_v3(u->Kd.x, u->Kd.y, u->Kd.z), lambertian));

		// Specular Light
		pgl_vec3 r = reflect_v3(neg_v3(s), n);
		
		float spec = max(0.0f, dot_v3s(r, v));

		float shine = pow(spec, u->shininess);

		out_light = add_v3s(out_light, scale_v3(make_v3(u->Ks.x, u->Ks.y, u->Ks.z), shine));
	}

	builtins->gl_FragColor = make_v4(out_light.x, out_light.y, out_light.z, 1.0);
}
