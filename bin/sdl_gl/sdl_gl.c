/*
 * Red Sphere with Random Rotation
 * Using PortableGL
 */

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#define PORTABLEGL_IMPLEMENTATION
#include "portablegl/portablegl.h"

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#ifndef M_PI
#define M_PI 3.14159265
#endif

#define WIDTH 640
#define HEIGHT 480

void cleanup(void);
void setup_context(void);
int handle_events(void);

SDL_Window* window;
SDL_Renderer* ren;
SDL_Texture* tex;
pix_t* bbufpix;

glContext the_context;

int polygon_mode;

typedef struct My_Uniforms
{
	mat4 mvp_mat;
	mat4 normal_mat;
	vec3 material_color;
} My_Uniforms;

static My_Uniforms uniforms;

// Sphere data
static GLuint sphere_vbo;
static GLuint sphere_ibo;
static int sphere_vertex_count;
static int sphere_index_count;

// Rotation state
static float rot_x = 0.0f;
static float rot_y = 0.0f;
static float rot_z = 0.0f;
static float rot_speed_x = 0.0f;
static float rot_speed_y = 0.0f;
static float rot_speed_z = 0.0f;

/** The projection matrix */
static GLfloat ProjectionMatrix[16];

static void
multiply(GLfloat *m, const GLfloat *n)
{
	GLfloat tmp[16];
	const GLfloat *row, *column;
	div_t d;
	int i, j;

	for (i = 0; i < 16; i++) {
		tmp[i] = 0;
		d = div(i, 4);
		row = n + d.quot * 4;
		column = m + d.rem;
		for (j = 0; j < 4; j++)
			tmp[i] += row[j] * column[j * 4];
	}
	memcpy(m, &tmp, sizeof tmp);
}

static void
rotate(GLfloat *m, GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
	float s = sinf(angle);
	float c = cosf(angle);
	GLfloat r[16] = {
		x * x * (1 - c) + c,     y * x * (1 - c) + z * s, x * z * (1 - c) - y * s, 0,
		x * y * (1 - c) - z * s, y * y * (1 - c) + c,     y * z * (1 - c) + x * s, 0, 
		x * z * (1 - c) + y * s, y * z * (1 - c) - x * s, z * z * (1 - c) + c,     0,
		0, 0, 0, 1
	};
	multiply(m, r);
}

static void
translate(GLfloat *m, GLfloat x, GLfloat y, GLfloat z)
{
	GLfloat t[16] = { 1, 0, 0, 0,  0, 1, 0, 0,  0, 0, 1, 0,  x, y, z, 1 };
	multiply(m, t);
}

static void
identity(GLfloat *m)
{
	GLfloat t[16] = {
		1.0, 0.0, 0.0, 0.0,
		0.0, 1.0, 0.0, 0.0,
		0.0, 0.0, 1.0, 0.0,
		0.0, 0.0, 0.0, 1.0,
	};
	memcpy(m, t, sizeof(t));
}

static void 
transpose(GLfloat *m)
{
	GLfloat t[16] = {
		m[0], m[4], m[8],  m[12],
		m[1], m[5], m[9],  m[13],
		m[2], m[6], m[10], m[14],
		m[3], m[7], m[11], m[15]};
	memcpy(m, t, sizeof(t));
}

static void
invert(GLfloat *m)
{
	GLfloat t[16];
	identity(t);
	t[12] = -m[12]; t[13] = -m[13]; t[14] = -m[14];
	m[12] = m[13] = m[14] = 0;
	transpose(m);
	multiply(m, t);
}

void perspective(GLfloat *m, GLfloat fovy, GLfloat aspect, GLfloat zNear, GLfloat zFar)
{
	GLfloat tmp[16];
	identity(tmp);

	float radians = fovy / 2 * M_PI / 180;
	float deltaZ = zFar - zNear;
	float sine = sinf(radians);
	float cosine = cosf(radians);

	if ((deltaZ == 0) || (sine == 0) || (aspect == 0))
		return;

	float cotangent = cosine / sine;

	tmp[0] = cotangent / aspect;
	tmp[5] = cotangent;
	tmp[10] = -(zFar + zNear) / deltaZ;
	tmp[11] = -1;
	tmp[14] = -2 * zNear * zFar / deltaZ;
	tmp[15] = 0;

	memcpy(m, tmp, sizeof(tmp));
}

// Create sphere vertices and indices
static void create_sphere(float radius, int stacks, int slices)
{
	int i, j;
	int vertex_count = (stacks + 1) * (slices + 1);
	int index_count = stacks * slices * 6;
	
	float* vertices = malloc(vertex_count * 6 * sizeof(float)); // pos(3) + normal(3)
	GLuint* indices = malloc(index_count * sizeof(GLuint));
	
	int vidx = 0;
	for (i = 0; i <= stacks; i++) {
		float phi = M_PI * i / stacks;
		float sin_phi = sinf(phi);
		float cos_phi = cosf(phi);
		
		for (j = 0; j <= slices; j++) {
			float theta = 2.0f * M_PI * j / slices;
			float sin_theta = sinf(theta);
			float cos_theta = cosf(theta);
			
			// Normal
			float nx = sin_phi * cos_theta;
			float ny = cos_phi;
			float nz = sin_phi * sin_theta;
			
			// Position = normal * radius
			vertices[vidx++] = nx * radius;
			vertices[vidx++] = ny * radius;
			vertices[vidx++] = nz * radius;
			vertices[vidx++] = nx;
			vertices[vidx++] = ny;
			vertices[vidx++] = nz;
		}
	}
	
	int iidx = 0;
	for (i = 0; i < stacks; i++) {
		for (j = 0; j < slices; j++) {
			int i0 = i * (slices + 1) + j;
			int i1 = i0 + 1;
			int i2 = (i + 1) * (slices + 1) + j;
			int i3 = i2 + 1;
			
			indices[iidx++] = i0;
			indices[iidx++] = i2;
			indices[iidx++] = i1;
			indices[iidx++] = i1;
			indices[iidx++] = i2;
			indices[iidx++] = i3;
		}
	}
	
	// Create VBO
	glGenBuffers(1, &sphere_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, sphere_vbo);
	glBufferData(GL_ARRAY_BUFFER, vertex_count * 6 * sizeof(float), vertices, GL_STATIC_DRAW);
	
	// Create IBO
	glGenBuffers(1, &sphere_ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphere_ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_count * sizeof(GLuint), indices, GL_STATIC_DRAW);
	
	sphere_vertex_count = vertex_count;
	sphere_index_count = index_count;
	
	free(vertices);
	free(indices);
}

static void draw_sphere(void)
{
	GLfloat model_view[16];
	GLfloat normal_matrix[16];
	GLfloat model_view_projection[16];
	GLfloat transform[16];
	
	identity(transform);
	translate(transform, 0, 0, -5);
	rotate(transform, rot_x, 1, 0, 0);
	rotate(transform, rot_y, 0, 1, 0);
	rotate(transform, rot_z, 0, 0, 1);
	
	memcpy(model_view, transform, sizeof(model_view));
	
	memcpy(model_view_projection, ProjectionMatrix, sizeof(model_view_projection));
	multiply(model_view_projection, model_view);
	memcpy(&uniforms.mvp_mat, model_view_projection, sizeof(mat4));
	
	memcpy(normal_matrix, model_view, sizeof(normal_matrix));
	invert(normal_matrix);
	transpose(normal_matrix);
	memcpy(&uniforms.normal_mat, normal_matrix, sizeof(mat4));
	
	// Red color
	uniforms.material_color.x = 1.0f;
	uniforms.material_color.y = 0.0f;
	uniforms.material_color.z = 0.0f;
	
	glBindBuffer(GL_ARRAY_BUFFER, sphere_vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphere_ibo);
	
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), 0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
	
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	
	glDrawElements(GL_TRIANGLES, sphere_index_count, GL_UNSIGNED_INT, 0);
	
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(0);
}

static void sphere_draw(void)
{
	glClearColor(0.0, 0.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	draw_sphere();
}

static void sphere_idle(void)
{
	static int frames = 0;
	static double tRot0 = -1.0, tRate0 = -1.0;
	double dt, t = SDL_GetTicks() / 1000.0;
	
	if (tRot0 < 0.0)
		tRot0 = t;
	dt = t - tRot0;
	tRot0 = t;
	
	// Update rotation
	rot_x += rot_speed_x * dt;
	rot_y += rot_speed_y * dt;
	rot_z += rot_speed_z * dt;
	
	// Keep angles in [0, 2*PI]
	while (rot_x > 2 * M_PI) rot_x -= 2 * M_PI;
	while (rot_y > 2 * M_PI) rot_y -= 2 * M_PI;
	while (rot_z > 2 * M_PI) rot_z -= 2 * M_PI;
	
	frames++;
	
	if (tRate0 < 0.0)
		tRate0 = t;
	if (t - tRate0 >= 5.0) {
		GLfloat seconds = t - tRate0;
		GLfloat fps = frames / seconds;
		printf("%d frames in %3.1f seconds = %6.3f FPS\n", frames, seconds, fps);
		tRate0 = t;
		frames = 0;
	}
}

void vertex_shader(float* vs_output, vec4* v_attrs, Shader_Builtins* builtins, void* uniforms)
{
	vec3* vs_out = (vec3*)vs_output;
	My_Uniforms* u = uniforms;
	
	vec4 v4 = mult_m4_v4(u->normal_mat, v_attrs[1]);
	vec3 v3 = { v4.x, v4.y, v4.z };
	vec3 N = norm_v3(v3);
	
	const vec3 light_pos = { 5.0, 5.0, 10.0 };
	vec3 L = norm_v3(light_pos);
	
	float tmp = dot_v3s(N, L);
	float diff_intensity = MAX(tmp, 0.0);
	
	vs_out[0] = scale_v3(u->material_color, diff_intensity);
	
	builtins->gl_Position = mult_m4_v4(u->mvp_mat, v_attrs[0]);
}

void fragment_shader(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	vec3 color = ((vec3*)fs_input)[0];
	
	// ARGB format - no swap needed
	builtins->gl_FragColor.x = color.x;
	builtins->gl_FragColor.y = color.y;
	builtins->gl_FragColor.z = color.z;
	builtins->gl_FragColor.w = 1;
}

static void sphere_init(void)
{
	GLuint program;
	
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	
	GLenum smooth[3] = { PGL_SMOOTH3 };
	
	program = pglCreateProgram(vertex_shader, fragment_shader, 3, smooth, GL_FALSE);
	glUseProgram(program);
	pglSetUniform(&uniforms);
	
	perspective(ProjectionMatrix, 60.0, WIDTH / (float)HEIGHT, 0.1, 100.0);
	glViewport(0, 0, (GLint) WIDTH, (GLint) HEIGHT);
	
	// Create sphere with radius 1.5, 20 stacks, 20 slices
	create_sphere(1.5f, 20, 20);
	
	// Initialize random rotation speeds
	srand(SDL_GetTicks());
	rot_speed_x = (rand() % 100 - 50) / 10.0f; // -5 to 5 rad/s
	rot_speed_y = (rand() % 100 - 50) / 10.0f;
	rot_speed_z = (rand() % 100 - 50) / 10.0f;
	
	printf("Rotation speeds: x=%.2f, y=%.2f, z=%.2f rad/s\n", 
	       rot_speed_x, rot_speed_y, rot_speed_z);
}

void cleanup(void)
{
	free_glContext(&the_context);
	SDL_DestroyTexture(tex);
	SDL_DestroyRenderer(ren);
	SDL_DestroyWindow(window);
	SDL_Quit();
}

void setup_context(void)
{
	SDL_SetMainReady();
	if (SDL_Init(SDL_INIT_VIDEO)) {
		printf("SDL_init error: %s\n", SDL_GetError());
		exit(0);
	}
	
	window = SDL_CreateWindow("Red Sphere", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
	                          WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
	if (!window) {
		printf("Failed to create window\n");
		SDL_Quit();
		exit(0);
	}
	
	ren = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
	tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
	
	if (!init_glContext(&the_context, &bbufpix, WIDTH, HEIGHT)) {
		puts("Failed to initialize glContext");
		exit(0);
	}
}

int handle_events(void)
{
	SDL_Event e;
	int sc;
	while (SDL_PollEvent(&e)) {
		if (e.type == SDL_QUIT) {
			return 1;
		} else if (e.type == SDL_KEYDOWN) {
			sc = e.key.keysym.scancode;
			
			switch (sc) {
			case SDL_SCANCODE_ESCAPE:
				return 1;
			case SDL_SCANCODE_P:
				polygon_mode = (polygon_mode + 1) % 3;
				if (polygon_mode == 0)
					glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
				else if (polygon_mode == 1)
					glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				else
					glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
				break;
			case SDL_SCANCODE_R:
				// Randomize rotation speeds
				rot_speed_x = (rand() % 100 - 50) / 10.0f;
				rot_speed_y = (rand() % 100 - 50) / 10.0f;
				rot_speed_z = (rand() % 100 - 50) / 10.0f;
				printf("New rotation speeds: x=%.2f, y=%.2f, z=%.2f rad/s\n", 
				       rot_speed_x, rot_speed_y, rot_speed_z);
				break;
			}
		}
	}
	return 0;
}

int main(int argc, char *argv[])
{
	(void)argc;
	(void)argv;

	printf("Pixel format: %s\n", PGL_PIX_STR);
	printf("RMASK: 0x%08X, GMASK: 0x%08X, BMASK: 0x%08X, AMASK: 0x%08X\n",
	       PGL_RMASK, PGL_GMASK, PGL_BMASK, PGL_AMASK);
	
	setup_context();
	polygon_mode = 2;
	
	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	
	sphere_init();
	
	while (1) {
		if (handle_events())
			break;
		
		sphere_idle();
		sphere_draw();
		
		SDL_UpdateTexture(tex, NULL, bbufpix, WIDTH * sizeof(pix_t));
		SDL_RenderCopy(ren, tex, NULL, NULL);
		SDL_RenderPresent(ren);
	}
	
	cleanup();
	return 0;
}
