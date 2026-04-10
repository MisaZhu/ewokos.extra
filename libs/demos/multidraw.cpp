// Use only portablegl, no rsw_math
#define PORTABLEGL_IMPLEMENTATION
#include "portablegl.h"

#include <vector>
#include <SDL2/SDL.h>
#include <stdio.h>

#define WIDTH 640
#define HEIGHT 480

using namespace std;

// Use portablegl's types directly
// vec3, vec4, mat4 are defined in portablegl.h

SDL_Window* win;
SDL_Renderer* ren;
SDL_Texture* tex;

pix_t* bbufpix;

glContext the_Context;

typedef struct My_Uniforms
{
	mat4 mvp_mat;
	vec4 color;
} My_Uniforms;

int polygon_mode;
int use_elements;
int minus_1;

// Simple matrix stack implementation using portablegl's mat4
typedef struct {
	mat4 matrices[32];
	int top;
} simple_mat_stack;

simple_mat_stack mat_stack;
My_Uniforms the_uniforms;

void cleanup();
void setup_context();
int handle_events();

// Helper functions for matrix operations
void mat4_identity(mat4 m);
void mat4_translate(mat4 m, float x, float y, float z);
void mat4_ortho(mat4 m, float left, float right, float bottom, float top, float near, float far);
void mat4_multiply(mat4 result, const mat4 a, const mat4 b);

// Shader functions
void basic_transform_vp(float* vs_output, vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms);
void white_fp(float* fs_input, Shader_Builtins* builtins, void* uniforms);
void uniform_color_fp(float* fs_input, Shader_Builtins* builtins, void* uniforms);

int main(int argc, char** argv)
{
	setup_context();

	// Initialize matrix stack
	mat_stack.top = 0;
	mat4_identity(mat_stack.matrices[0]);

	vector<vec3> tri_strips;
	vector<GLuint> strip_elems;

	int sq_dim = 20;
	vector<GLint> firsts;
	vector<GLintptr> first_elems;
	vector<GLsizei> counts;

	const int cols = 25;
	const int rows = 19;

	for (int j=0; j<rows; j++) {
		for (int i=0; i<cols; i++) {
			firsts.push_back(tri_strips.size());
			first_elems.push_back(strip_elems.size()*sizeof(GLuint));
			counts.push_back(4);

			// Create vertices manually
			vec3 v1, v2, v3, v4;
			v1.x = (float)(i*(sq_dim+5));        v1.y = (float)(j*(sq_dim+5));        v1.z = 0.0f;
			v2.x = (float)(i*(sq_dim+5));        v2.y = (float)(j*(sq_dim+5)+sq_dim); v2.z = 0.0f;
			v3.x = (float)(i*(sq_dim+5)+sq_dim); v3.y = (float)(j*(sq_dim+5));        v3.z = 0.0f;
			v4.x = (float)(i*(sq_dim+5)+sq_dim); v4.y = (float)(j*(sq_dim+5)+sq_dim); v4.z = 0.0f;
			tri_strips.push_back(v1);
			tri_strips.push_back(v2);
			tri_strips.push_back(v3);
			tri_strips.push_back(v4);

			strip_elems.push_back((j*cols+i)*4);
			strip_elems.push_back((j*cols+i)*4+1);
			strip_elems.push_back((j*cols+i)*4+2);
			strip_elems.push_back((j*cols+i)*4+3);
		}
	}

	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	GLuint square_buf;
	glGenBuffers(1, &square_buf);
	glBindBuffer(GL_ARRAY_BUFFER, square_buf);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vec3)*tri_strips.size(), &tri_strips[0], GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	GLuint elem_buf;
	glGenBuffers(1, &elem_buf);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elem_buf);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint)*strip_elems.size(), &strip_elems[0], GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);

	GLuint program = pglCreateProgram(basic_transform_vp, white_fp, 0, NULL, GL_FALSE);
	glUseProgram(program);
	pglSetUniform(&the_uniforms);

	// Set up orthographic projection
	mat4_ortho(mat_stack.matrices[mat_stack.top], 0.0f, (float)WIDTH, 0.0f, (float)HEIGHT, 1.0f, -1.0f);

	unsigned int old_time = 0, new_time=0, counter = 0;
	while (1) {
		if (handle_events())
			break;

		counter++;
		new_time = SDL_GetTicks();
		if (new_time - old_time > 3000) {
			printf("%f FPS\n", counter*1000.f/(new_time-old_time));
			old_time = new_time;
			counter = 0;
		}

		glClear(GL_COLOR_BUFFER_BIT);

		// Push matrix and translate
		if (mat_stack.top < 31) {
			mat_stack.top++;
			for (int i = 0; i < 16; i++) {
				mat_stack.matrices[mat_stack.top][i] = mat_stack.matrices[mat_stack.top-1][i];
			}
		}
		
		// Apply translation
		mat4_translate(mat_stack.matrices[mat_stack.top], 10.0f, 10.0f, 0.0f);

		// Copy matrix to uniforms
		for (int i = 0; i < 16; i++) {
			the_uniforms.mvp_mat[i] = mat_stack.matrices[mat_stack.top][i];
		}

		if (!use_elements) {
			glMultiDrawArrays(GL_TRIANGLE_STRIP, &firsts[0], &counts[0], 100);
		} else {
			glMultiDrawElements(GL_TRIANGLE_STRIP, &counts[0], GL_UNSIGNED_INT, (GLvoid* const*)&first_elems[0], 475);
		}

		// Pop matrix
		if (mat_stack.top > 0) {
			mat_stack.top--;
		}

		SDL_UpdateTexture(tex, NULL, bbufpix, WIDTH * sizeof(pix_t));
		SDL_RenderCopy(ren, tex, NULL, NULL);
		SDL_RenderPresent(ren);
	}

	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(1, &square_buf);
	glDeleteProgram(program);

	cleanup();

	return 0;
}

// Matrix helper implementations
void mat4_identity(mat4 m)
{
	m[0] = 1.0f;  m[4] = 0.0f;  m[8] = 0.0f;  m[12] = 0.0f;
	m[1] = 0.0f;  m[5] = 1.0f;  m[9] = 0.0f;  m[13] = 0.0f;
	m[2] = 0.0f;  m[6] = 0.0f;  m[10] = 1.0f; m[14] = 0.0f;
	m[3] = 0.0f;  m[7] = 0.0f;  m[11] = 0.0f; m[15] = 1.0f;
}

void mat4_translate(mat4 m, float x, float y, float z)
{
	m[12] += m[0]*x + m[4]*y + m[8]*z;
	m[13] += m[1]*x + m[5]*y + m[9]*z;
	m[14] += m[2]*x + m[6]*y + m[10]*z;
	m[15] += m[3]*x + m[7]*y + m[11]*z;
}

void mat4_ortho(mat4 m, float left, float right, float bottom, float top, float near, float far)
{
	float rl = right - left;
	float tb = top - bottom;
	float fn = far - near;

	m[0] = 2.0f / rl;  m[4] = 0.0f;      m[8] = 0.0f;       m[12] = -(right + left) / rl;
	m[1] = 0.0f;       m[5] = 2.0f / tb; m[9] = 0.0f;       m[13] = -(top + bottom) / tb;
	m[2] = 0.0f;       m[6] = 0.0f;      m[10] = -2.0f / fn; m[14] = -(far + near) / fn;
	m[3] = 0.0f;       m[7] = 0.0f;      m[11] = 0.0f;      m[15] = 1.0f;
}

// Vertex shader
void basic_transform_vp(float* vs_output, vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
{
	My_Uniforms* u = (My_Uniforms*)uniforms;
	// Manual matrix-vector multiplication
	vec4 result;
	result.x = u->mvp_mat[0]*vertex_attribs[0].x + u->mvp_mat[4]*vertex_attribs[0].y + u->mvp_mat[8]*vertex_attribs[0].z + u->mvp_mat[12]*vertex_attribs[0].w;
	result.y = u->mvp_mat[1]*vertex_attribs[0].x + u->mvp_mat[5]*vertex_attribs[0].y + u->mvp_mat[9]*vertex_attribs[0].z + u->mvp_mat[13]*vertex_attribs[0].w;
	result.z = u->mvp_mat[2]*vertex_attribs[0].x + u->mvp_mat[6]*vertex_attribs[0].y + u->mvp_mat[10]*vertex_attribs[0].z + u->mvp_mat[14]*vertex_attribs[0].w;
	result.w = u->mvp_mat[3]*vertex_attribs[0].x + u->mvp_mat[7]*vertex_attribs[0].y + u->mvp_mat[11]*vertex_attribs[0].z + u->mvp_mat[15]*vertex_attribs[0].w;
	builtins->gl_Position = result;
}

// Fragment shaders
void white_fp(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	builtins->gl_FragColor.x = 1.0f;
	builtins->gl_FragColor.y = 1.0f;
	builtins->gl_FragColor.z = 1.0f;
	builtins->gl_FragColor.w = 1.0f;
}

void uniform_color_fp(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	My_Uniforms* u = (My_Uniforms*)uniforms;
	builtins->gl_FragColor.x = u->color.x;
	builtins->gl_FragColor.y = u->color.y;
	builtins->gl_FragColor.z = u->color.z;
	builtins->gl_FragColor.w = u->color.w;
}

void setup_context()
{
	SDL_SetMainReady();
	if (SDL_Init(SDL_INIT_VIDEO)) {
		printf("SDL_Init error: %s\n", SDL_GetError());
		exit(0);
	}

	win = SDL_CreateWindow("Multidraw", 100, 100, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
	if (!win) {
		cleanup();
		exit(0);
	}

	ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
	tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);

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
	SDL_DestroyWindow(win);

	SDL_Quit();
}

int handle_events()
{
	SDL_Event e;
	int sc;
	while (SDL_PollEvent(&e)) {
		if (e.type == SDL_QUIT) {
			return 1;
		} else if (e.type == SDL_KEYDOWN) {
			sc = e.key.keysym.scancode;

			if (sc == SDL_SCANCODE_ESCAPE) {
				return 1;
			} else if (sc == SDL_SCANCODE_P) {
				polygon_mode = (polygon_mode + 1) % 3;
				if (polygon_mode == 0)
					glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
				else if (polygon_mode == 1)
					glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				else
					glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			} else if (sc == SDL_SCANCODE_E) {
				use_elements = !use_elements;
				if (use_elements)
					printf("Using glMultiDrawElements\n");
				else
					printf("Using glMultiDrawArrays\n");
			} else if (sc == SDL_SCANCODE_MINUS) {
				minus_1 = !minus_1;
			}
		}
	}
	return 0;
}
