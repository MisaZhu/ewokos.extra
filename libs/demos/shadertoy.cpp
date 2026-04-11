
#define PGL_PREFIX_TYPES
#define PORTABLEGL_IMPLEMENTATION
#include "gltools.h"

#include <iostream>
#include <stdio.h>

#define SDL_MAIN_HANDLED
#include <SDL.h>

#define WIDTH 320
#define HEIGHT 240

using namespace std;

SDL_Window* window;
SDL_Renderer* ren;
SDL_Texture* tex;

u32* bbufpix;
glContext the_Context;

float iGlobalTime;

typedef struct My_Uniforms
{
	float globaltime;
	GLuint tex0;
	GLuint tex1;
	GLuint tex2;
	GLuint tex6;
	GLuint tex9;
} My_Uniforms;

#define NUM_TEXTURES 5
GLuint textures[NUM_TEXTURES];

#define NUM_SHADERS 11
GLuint shaders[NUM_SHADERS];

void cleanup();
void setup_context();

void graphing_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);
void graphing_lines_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);
void my_tunnel_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);
void square_tunnel_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);
void deform_tunnel_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);
void the_cave_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);
void tileable_water_caustic_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);
void flame_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);
void running_in_the_night_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);
void voronoise_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);
void iqs_eyeball(float* fs_input, Shader_Builtins* builtins, void* uniforms);

frag_func frag_funcs[NUM_SHADERS] =
{
	graphing_lines_fs,
	graphing_fs,
	my_tunnel_fs,
	running_in_the_night_fs,
	square_tunnel_fs,
	deform_tunnel_fs,
	tileable_water_caustic_fs,
	iqs_eyeball,
	voronoise_fs,
	the_cave_fs,
	flame_fs
};

// Helper functions for C-style vector operations
// Note: make_v2/make_v3/make_v4 are already defined in portablegl.h

inline pgl_vec2 v2_add(pgl_vec2 a, pgl_vec2 b) { return make_v2(a.x + b.x, a.y + b.y); }
inline pgl_vec2 v2_sub(pgl_vec2 a, pgl_vec2 b) { return make_v2(a.x - b.x, a.y - b.y); }
inline pgl_vec2 v2_mul(pgl_vec2 a, pgl_vec2 b) { return make_v2(a.x * b.x, a.y * b.y); }
inline pgl_vec2 v2_scale(pgl_vec2 a, float s) { return make_v2(a.x * s, a.y * s); }
inline float v2_dot(pgl_vec2 a, pgl_vec2 b) { return a.x * b.x + a.y * b.y; }
inline float v2_len(pgl_vec2 a) { return sqrtf(a.x * a.x + a.y * a.y); }
inline pgl_vec2 v2_norm(pgl_vec2 a) { float l = v2_len(a); return make_v2(a.x / l, a.y / l); }

inline pgl_vec3 v3_add(pgl_vec3 a, pgl_vec3 b) { return make_v3(a.x + b.x, a.y + b.y, a.z + b.z); }
inline pgl_vec3 v3_sub(pgl_vec3 a, pgl_vec3 b) { return make_v3(a.x - b.x, a.y - b.y, a.z - b.z); }
inline pgl_vec3 v3_mul(pgl_vec3 a, pgl_vec3 b) { return make_v3(a.x * b.x, a.y * b.y, a.z * b.z); }
inline pgl_vec3 v3_scale(pgl_vec3 a, float s) { return make_v3(a.x * s, a.y * s, a.z * s); }
inline float v3_dot(pgl_vec3 a, pgl_vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline float v3_len(pgl_vec3 a) { return sqrtf(a.x * a.x + a.y * a.y + a.z * a.z); }
inline pgl_vec3 v3_norm(pgl_vec3 a) { float l = v3_len(a); return make_v3(a.x / l, a.y / l, a.z / l); }
inline pgl_vec3 v3_cross(pgl_vec3 a, pgl_vec3 b) {
	return make_v3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}

inline pgl_vec4 v4_add(pgl_vec4 a, pgl_vec4 b) { return make_v4(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w); }
inline pgl_vec4 v4_sub(pgl_vec4 a, pgl_vec4 b) { return make_v4(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w); }
inline pgl_vec4 v4_mul(pgl_vec4 a, pgl_vec4 b) { return make_v4(a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w); }
inline pgl_vec4 v4_scale(pgl_vec4 a, float s) { return make_v4(a.x * s, a.y * s, a.z * s, a.w * s); }
inline float v4_dot(pgl_vec4 a, pgl_vec4 b) { return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w; }

inline float clamp_f(float x, float minVal, float maxVal) {
	if (x < minVal) return minVal;
	if (x > maxVal) return maxVal;
	return x;
}

inline float smoothstep_f(float edge0, float edge1, float x) {
	float t = clamp_f((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
	return t * t * (3.0f - 2.0f * t);
}

inline float mix_f(float x, float y, float a) {
	return x * (1.0f - a) + y * a;
}

inline pgl_vec3 v3_mix(pgl_vec3 x, pgl_vec3 y, float a) {
	return make_v3(mix_f(x.x, y.x, a), mix_f(x.y, y.y, a), mix_f(x.z, y.z, a));
}

inline pgl_vec4 v4_mix(pgl_vec4 x, pgl_vec4 y, float a) {
	return make_v4(mix_f(x.x, y.x, a), mix_f(x.y, y.y, a), mix_f(x.z, y.z, a), mix_f(x.w, y.w, a));
}

inline float fract_f(float x) { return x - floorf(x); }

inline pgl_vec2 v2_fract(pgl_vec2 v) { return make_v2(fract_f(v.x), fract_f(v.y)); }
inline pgl_vec3 v3_fract(pgl_vec3 v) { return make_v3(fract_f(v.x), fract_f(v.y), fract_f(v.z)); }

inline pgl_vec2 v2_floor(pgl_vec2 v) { return make_v2(floorf(v.x), floorf(v.y)); }
inline pgl_vec3 v3_floor(pgl_vec3 v) { return make_v3(floorf(v.x), floorf(v.y), floorf(v.z)); }

inline float v2_length(pgl_vec2 a) { return sqrtf(a.x * a.x + a.y * a.y); }
inline float v3_length(pgl_vec3 a) { return sqrtf(a.x * a.x + a.y * a.y + a.z * a.z); }

int main(int argc, char** argv)
{
	setup_context();

	float points[] =
	{
		-1.0,  1.0, 0,
		-1.0, -1.0, 0,
		 1.0,  1.0, 0,
		 1.0, -1.0, 0
	};

	My_Uniforms the_uniforms;

	glGenTextures(NUM_TEXTURES, textures);
	glBindTexture(GL_TEXTURE_2D, textures[0]);
	if (!load_texture2D("/data/media/textures/tex00.jpg", GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_FALSE, GL_FALSE, NULL, NULL)) {
		printf("failed to load texture\n");
		return 0;
	}
	glBindTexture(GL_TEXTURE_2D, textures[1]);
	if (!load_texture2D("/data/media/textures/tex02.jpg", GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_FALSE, GL_FALSE, NULL, NULL)) {
		printf("failed to load texture\n");
		return 0;
	}
	glBindTexture(GL_TEXTURE_2D, textures[2]);
	if (!load_texture2D("/data/media/textures/tex06.jpg", GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_FALSE, GL_FALSE, NULL, NULL)) {
		printf("failed to load texture\n");
		return 0;
	}
	glBindTexture(GL_TEXTURE_2D, textures[3]);
	if (!load_texture2D("/data/media/textures/tex01.jpg", GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_FALSE, GL_FALSE, NULL, NULL)) {
		printf("failed to load texture\n");
		return 0;
	}
	glBindTexture(GL_TEXTURE_2D, textures[4]);
	if (!load_texture2D("/data/media/textures/tex09.jpg", GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_FALSE, GL_FALSE, NULL, NULL)) {
		printf("failed to load texture\n");
		return 0;
	}

	the_uniforms.tex0 = textures[0];
	the_uniforms.tex2 = textures[1];
	the_uniforms.tex6 = textures[2];
	the_uniforms.tex1 = textures[3];
	the_uniforms.tex9 = textures[4];

	GLuint screen_quad;
	glGenBuffers(1, &screen_quad);
	glBindBuffer(GL_ARRAY_BUFFER, screen_quad);
	glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	for (int i=0; i<NUM_SHADERS; ++i) {
		shaders[i] = pglCreateFragProgram(frag_funcs[i], GL_FALSE);
		glUseProgram(shaders[i]);
		pglSetUniform(&the_uniforms);
	}

	int cur_shader = 0;
	glUseProgram(shaders[cur_shader]);

	SDL_Event event;
	SDL_Keysym keysym;
	bool quit = GL_FALSE;

	unsigned int old_time = 0, new_time=0, counter = 0, start_time = 0;

	while (!quit) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) {
				quit = true;
			} else if (event.type == SDL_KEYDOWN) {
				keysym = event.key.keysym;
				switch (keysym.sym) {
				case SDLK_ESCAPE:
					quit = true;
					break;
				case SDLK_LEFT:
					cur_shader = (cur_shader) ? cur_shader-1 : NUM_SHADERS-1;
					start_time = SDL_GetTicks();
					glUseProgram(shaders[cur_shader]);
					break;
				case SDLK_RIGHT:
					cur_shader = (cur_shader + 1) % NUM_SHADERS;
					start_time = SDL_GetTicks();
					glUseProgram(shaders[cur_shader]);
					break;
				}
			}
		}

		++counter;
		new_time = SDL_GetTicks();
		if (new_time - old_time >= 3000) {
			printf("%f FPS\n", counter*1000.0f/((float)(new_time-old_time)));
			old_time = new_time;
			counter = 0;
		}

		iGlobalTime = (new_time-start_time) / 1000.0f;
		the_uniforms.globaltime = (new_time-start_time) / 1000.0f;

		pglDrawFrame2(frag_funcs[cur_shader], &the_uniforms);

		SDL_UpdateTexture(tex, NULL, bbufpix, WIDTH * sizeof(u32));
		SDL_RenderCopy(ren, tex, NULL, NULL);
		SDL_RenderPresent(ren);
	}

	cleanup();
	return 0;
}

void graphing_lines_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	pgl_vec2 frag = *(pgl_vec2*)(&builtins->gl_FragCoord);
	frag.x /= WIDTH;
	frag.y /= HEIGHT;
	frag = v2_scale(frag, 2.0f);
	frag = v2_sub(frag, make_v2(1.0f, 1.0f));

	float fragx2 = frag.x * frag.x;
	float x2 = frag.y - fragx2;
	float x3 = frag.y - fragx2 * frag.x + 0.25f * frag.x;
	float x4 = frag.y - fragx2 * fragx2;
	float incr = 2.0f / HEIGHT;

	if (x2 >= -incr && x2 <= incr) {
		*(pgl_vec4*)&builtins->gl_FragColor = make_v4(1, 0, 0, 1);
	} else if (x3 >= -incr && x3 <= incr) {
		*(pgl_vec4*)&builtins->gl_FragColor = make_v4(0, 1, 0, 1);
	} else if (x4 >= -incr && x4 <= incr) {
		*(pgl_vec4*)&builtins->gl_FragColor = make_v4(0, 0, 1, 1);
	} else {
		*(pgl_vec4*)&builtins->gl_FragColor = make_v4(0, 0, 0, 1);
	}
}

void graphing_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	pgl_vec2 frag = *(pgl_vec2*)(&builtins->gl_FragCoord);
	frag.x /= WIDTH;
	frag.y /= HEIGHT;
	frag = v2_scale(frag, 2.0f);
	frag = v2_sub(frag, make_v2(1.0f, 1.0f));

	float fragx2 = frag.x * frag.x;
	float x2 = frag.y - fragx2;

	if (x2 > 0)
		*(pgl_vec4*)&builtins->gl_FragColor = make_v4(x2, 0, 0, 1);
	else
		*(pgl_vec4*)&builtins->gl_FragColor = make_v4(0, 0, 0, 1);
}

void my_tunnel_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	float globaltime = ((My_Uniforms*)uniforms)->globaltime;

	pgl_vec2 frag = *(pgl_vec2*)(&builtins->gl_FragCoord);
	frag.x /= WIDTH;
	frag.y /= HEIGHT;
	frag = v2_scale(frag, 2.0f);
	frag = v2_sub(frag, make_v2(1.0f, 1.0f));

	float r = powf(powf(frag.x, 16.0f) + powf(frag.y, 16.0f), 1.0f/16.0f);
	float wave = (0.5f * sinf(10.0f * globaltime * (1.0f - r)) + 0.5f);

	*(pgl_vec4*)&builtins->gl_FragColor = make_v4(r * wave, 0, r * (1.0f - wave), 1);
}

void square_tunnel_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	float globaltime = ((My_Uniforms*)uniforms)->globaltime;
	GLuint channel0 = ((My_Uniforms*)uniforms)->tex0;

	pgl_vec2 resolution = make_v2(WIDTH, HEIGHT);
	pgl_vec2 frag = *(pgl_vec2*)(&builtins->gl_FragCoord);
	
	pgl_vec2 p = v2_sub(v2_scale(frag, 2.0f), resolution);
	p = v2_scale(p, 1.0f / resolution.y);

	float a = atan2f(p.y, p.x);
	float r = powf(powf(p.x * p.x, 16.0f) + powf(p.y * p.y, 16.0f), 1.0f/32.0f);

	pgl_vec2 uv = make_v2(0.5f/r + 0.5f * globaltime, a / 3.1416f);

	pgl_vec4 tmp = texture2D(channel0, uv.x, uv.y);
	*(pgl_vec4*)&builtins->gl_FragColor = make_v4(tmp.x * r, tmp.y * r, tmp.z * r, 1.0f);
}

void deform_tunnel_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	float globaltime = ((My_Uniforms*)uniforms)->globaltime;
	GLuint channel0 = ((My_Uniforms*)uniforms)->tex2;
	GLuint channel1 = ((My_Uniforms*)uniforms)->tex0;

	pgl_vec2 resolution = make_v2(WIDTH, HEIGHT);
	pgl_vec2 frag = *(pgl_vec2*)(&builtins->gl_FragCoord);

	pgl_vec2 p = v2_sub(v2_scale(v2_scale(frag, 2.0f), 1.0f/resolution.x), make_v2(1.0f, 1.0f));
	p.y = p.y * (resolution.y / resolution.x);
	
	float r = powf(powf(p.x * p.x, 16.0f) + powf(p.y * p.y, 16.0f), 1.0f/32.0f);
	
	pgl_vec2 uv;
	uv.x = 0.5f * globaltime + 0.5f / r;
	uv.y = atan2f(p.y, p.x) / 3.1416f;

	float h = sinf(32.0f * uv.y);
	uv.x += 0.85f * smoothstep_f(-0.1f, 0.1f, h);
	
	pgl_vec4 ch1_ = texture2D(channel1, 2.0f * uv.x, 2.0f * uv.y);
	pgl_vec4 ch0_ = texture2D(channel0, uv.x, uv.y);

	pgl_vec3 ch0 = make_v3(ch0_.x, ch0_.y, ch0_.z);
	pgl_vec3 ch1 = make_v3(ch1_.x, ch1_.y, ch1_.z);
	float aa = smoothstep_f(0.9f, 1.1f, fabsf(p.x / p.y));
	pgl_vec3 col = v3_mix(ch1, ch0, aa);

	r *= 1.0f - 0.3f * (smoothstep_f(0.0f, 0.3f, h) - smoothstep_f(0.3f, 0.96f, h));

	*(pgl_vec4*)&builtins->gl_FragColor = make_v4(col.x * r * r * 1.2f, col.y * r * r * 1.2f, col.z * r * r * 1.2f, 1.0f);
}

void tileable_water_caustic_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
#define TAU 6.28318530718f
#define MAX_ITER 5

	pgl_vec2 iResolution = make_v2(WIDTH, HEIGHT);
	pgl_vec2 gl_FragCoord = *(pgl_vec2*)(&builtins->gl_FragCoord);

	float time = iGlobalTime * 0.5f + 23.0f;
	pgl_vec2 sp;
	sp.x = gl_FragCoord.x / iResolution.x;
	sp.y = gl_FragCoord.y / iResolution.y;

	pgl_vec2 p = v2_sub(v2_scale(sp, TAU), make_v2(250.0f, 250.0f));
	pgl_vec2 i = p;
	float c = 1.0f;
	float inten = 0.005f;

	for (int n = 0; n < MAX_ITER; n++)
	{
		float t = time * (1.0f - (3.5f / (float)(n+1)));
		i.x = p.x + cosf(t - i.x) + sinf(t + i.y);
		i.y = p.y + sinf(t - i.y) + cosf(t + i.x);
		pgl_vec2 len_vec = make_v2(
			p.x / (sinf(i.x + t) / inten),
			p.y / (cosf(i.y + t) / inten)
		);
		c += 1.0f / v2_length(len_vec);
	}
	c /= (float)MAX_ITER;
	c = 1.2f - powf(c, 1.2f);
	
	float colour_val = powf(fabsf(c), 6.0f);
	pgl_vec3 colour = make_v3(colour_val, colour_val, colour_val);
	
	pgl_vec3 final_col = v3_add(colour, make_v3(0.0f, 0.35f, 0.5f));
	final_col.x = clamp_f(final_col.x, 0.0f, 1.0f);
	final_col.y = clamp_f(final_col.y, 0.0f, 1.0f);
	final_col.z = clamp_f(final_col.z, 0.0f, 1.0f);

    *(pgl_vec4*)&builtins->gl_FragColor = make_v4(final_col.x, final_col.y, final_col.z, 1.0f);
}

void running_in_the_night_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	pgl_vec2 iResolution = make_v2(WIDTH, HEIGHT);
	pgl_vec2 gl_FragCoord = *(pgl_vec2*)(&builtins->gl_FragCoord);
	GLuint iChannel0 = ((My_Uniforms*)uniforms)->tex2;

	float time = 45.0f * 3.14159f / 180.0f + cosf(iGlobalTime * 12.0f) / 40.0f;
	float time1 = 45.0f * 3.14159f / 180.0f + cosf(iGlobalTime * 22.0f) / 30.0f;
	
	pgl_vec2 uv = v2_sub(v2_scale(gl_FragCoord, 2.0f / iResolution.x), make_v2(1.0f, 1.0f));
	uv.y = uv.y * (iResolution.y / iResolution.x);
	
	if (uv.y < 0.0f) {
		pgl_vec2 tex;
		pgl_vec2 rot = make_v2(cosf(time), sinf(time));
		pgl_vec2 mat;
		mat.x = (uv.x * rot.x + (uv.y - 1.0f) * rot.y);
		mat.y = ((uv.y - 1.0f) * rot.x - uv.x * rot.y);
		tex.x = mat.x * time1 / uv.y + iGlobalTime * 2.0f;
		tex.y = mat.y * time1 / uv.y + iGlobalTime * 2.0f;

		pgl_vec4 tmp = texture2D(iChannel0, tex.x * 2.0f, tex.y * 2.0f);
		*(pgl_vec4*)&builtins->gl_FragColor = make_v4(tmp.x * (-uv.y), tmp.y * (-uv.y), tmp.z * (-uv.y), tmp.w * (-uv.y));
	} else {
		*(pgl_vec4*)&builtins->gl_FragColor = make_v4(0, 0, 0, 0);
	}
}

pgl_vec3 hash3(pgl_vec2 p)
{
    pgl_vec3 q = make_v3(
		v2_dot(p, make_v2(127.1f, 311.7f)),
		v2_dot(p, make_v2(269.5f, 183.3f)),
		v2_dot(p, make_v2(419.2f, 371.9f))
	);
	return v3_fract(v3_scale(make_v3(sinf(q.x), sinf(q.y), sinf(q.z)), 43758.5453f));
}

float iqnoise(pgl_vec2 x, float u, float v)
{
    pgl_vec2 p = v2_floor(x);
    pgl_vec2 f = v2_fract(x);

	float k = 1.0f + 63.0f * powf(1.0f - v, 4.0f);

	float va = 0.0f;
	float wt = 0.0f;
    for (int j = -2; j <= 2; j++)
    for (int i = -2; i <= 2; i++)
    {
        pgl_vec2 g = make_v2((float)i, (float)j);
		pgl_vec3 o = v3_mul(hash3(v2_add(p, g)), make_v3(u, u, 1.0f));
		pgl_vec2 r = v2_add(v2_sub(g, f), make_v2(o.x, o.y));
		float d = v2_dot(r, r);
		float ww = powf(1.0f - smoothstep_f(0.0f, 1.414f, sqrtf(d)), k);
		va += o.z * ww;
		wt += ww;
    }

    return va / wt;
}

void voronoise_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	pgl_vec2 iResolution = make_v2(WIDTH, HEIGHT);
	pgl_vec2 gl_FragCoord = *(pgl_vec2*)(&builtins->gl_FragCoord);

	pgl_vec2 uv;
	uv.x = gl_FragCoord.x / iResolution.x;
	uv.y = gl_FragCoord.y / iResolution.x;

    pgl_vec2 p;
	p.x = 0.5f - 0.5f * sinf(iGlobalTime * 1.01f);
	p.y = 0.5f - 0.5f * sinf(iGlobalTime * 1.71f);

	float t1 = 3.0f - 2.0f * p.x;
	float t2 = 3.0f - 2.0f * p.y;
	p.x = p.x * p.x * t1;
	p.y = p.y * p.y * t2;
	p.x = p.x * p.x * (3.0f - 2.0f * p.x);
	p.y = p.y * p.y * (3.0f - 2.0f * p.y);
	p.x = p.x * p.x * (3.0f - 2.0f * p.x);
	p.y = p.y * p.y * (3.0f - 2.0f * p.y);

	float f = iqnoise(v2_scale(uv, 24.0f), p.x, p.y);

	*(pgl_vec4*)&builtins->gl_FragColor = make_v4(f, f, f, 1.0f);
}

float noise(pgl_vec3 p)
{
	pgl_vec3 i = v3_floor(p);
	pgl_vec3 v1 = make_v3(1.0f, 57.0f, 21.0f);
	pgl_vec3 v2 = make_v3(0.0f, 57.0f, 21.0f);
	pgl_vec3 v3 = make_v3(78.0f, 78.0f, 78.0f);
	pgl_vec4 a = v4_add(make_v4(v3_dot(i, v1), 0, 0, 0), make_v4(v2.x, v2.y, v2.z, v3.x));
	
	pgl_vec3 f = v3_scale(v3_sub(p, i), acosf(-1.0f));
	f.x = cosf(f.x) * (-0.5f) + 0.5f;
	f.y = cosf(f.y) * (-0.5f) + 0.5f;
	f.z = cosf(f.z) * (-0.5f) + 0.5f;
	
	// Simplified mix operations
	float a1 = sinf(cosf(a.x) * a.x);
	float a2 = sinf(cosf(a.y) * a.y);
	float a3 = sinf(cosf(1.0f + a.z) * (1.0f + a.z));
	float a4 = sinf(cosf(1.0f + a.w) * (1.0f + a.w));
	
	float m1 = mix_f(a1, a3, f.x);
	float m2 = mix_f(a2, a4, f.x);
	
	return mix_f(m1, m2, f.z);
}

float sphere(pgl_vec3 p, pgl_vec4 spr)
{
	return v3_length(v3_sub(make_v3(spr.x, spr.y, spr.z), p)) - spr.w;
}

float flame_func(pgl_vec3 p)
{
	pgl_vec3 scaled_p = make_v3(p.x, p.y * 0.5f, p.z);
	pgl_vec4 spr = make_v4(0.0f, -1.0f, 0.0f, 1.0f);
	float d = sphere(scaled_p, spr);
	return d + (noise(v3_add(p, make_v3(0.0f, iGlobalTime * 2.0f, 0.0f))) + noise(v3_scale(p, 3.0f)) * 0.5f) * 0.25f * p.y;
}

float scene(pgl_vec3 p)
{
	return fminf(100.0f - v3_length(p), fabsf(flame_func(p)));
}

pgl_vec4 raymarch(pgl_vec3 org, pgl_vec3 dir)
{
	float d = 0.0f, glow = 0.0f, eps = 0.02f;
	pgl_vec3 p = org;
	bool glowed = GL_FALSE;

	for(int i = 0; i < 64; i++)
	{
		d = scene(p) + eps;
		p = v3_add(p, v3_scale(dir, d));
		if (d > eps)
		{
			if(flame_func(p) < 0.0f)
				glowed = true;
			if(glowed)
       			glow = (float)i / 64.0f;
		}
	}
	return make_v4(p.x, p.y, p.z, glow);
}

void flame_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	pgl_vec2 iResolution = make_v2(WIDTH, HEIGHT);
	pgl_vec2 gl_FragCoord = *(pgl_vec2*)(&builtins->gl_FragCoord);

	pgl_vec2 v;
	v.x = -1.0f + 2.0f * gl_FragCoord.x / iResolution.x;
	v.y = -1.0f + 2.0f * gl_FragCoord.y / iResolution.y;
	v.x *= iResolution.x / iResolution.y;

	pgl_vec3 org = make_v3(0.0f, -2.0f, 4.0f);
	pgl_vec3 dir = v3_norm(make_v3(v.x * 1.6f, -v.y, -1.5f));

	pgl_vec4 p = raymarch(org, dir);
	float glow = p.w;

	pgl_vec4 col1 = make_v4(1.0f, 0.5f, 0.1f, 1.0f);
	pgl_vec4 col2 = make_v4(0.1f, 0.5f, 1.0f, 1.0f);
	pgl_vec4 col = v4_mix(col1, col2, p.y * 0.02f + 0.4f);

    *(pgl_vec4*)&builtins->gl_FragColor = v4_mix(make_v4(0, 0, 0, 0), col, powf(glow * 2.0f, 4.0f));
}

// Constants for the cave shader
const pgl_vec2 cama = {-2.6943f, 3.0483f};
const pgl_vec2 camb = {0.2516f, 0.1749f};
const pgl_vec2 camc = {-3.7902f, 2.4478f};
const pgl_vec2 camd = {0.0865f, -0.1664f};

const pgl_vec2 lighta = {1.4301f, 4.0985f};
const pgl_vec2 lightb = {-0.1276f, 0.2347f};
const pgl_vec2 lightc = {-2.2655f, 1.5066f};
const pgl_vec2 lightd = {-0.1284f, 0.0731f};

inline pgl_vec2 Position(float z, pgl_vec2 a, pgl_vec2 b, pgl_vec2 c, pgl_vec2 d)
{
	return v2_add(v2_scale(make_v2(sinf(z * a.x), sinf(z * a.y)), b.x), 
	              v2_scale(make_v2(cosf(z * c.x), cosf(z * c.y)), d.x));
}

inline pgl_vec3 Position3D(float time, pgl_vec2 a, pgl_vec2 b, pgl_vec2 c, pgl_vec2 d)
{
	pgl_vec2 pos = Position(time, a, b, c, d);
	return make_v3(pos.x, pos.y, time);
}

inline float Distance_(pgl_vec3 p, pgl_vec2 a, pgl_vec2 b, pgl_vec2 c, pgl_vec2 d, pgl_vec2 e, float r)
{
	pgl_vec2 pos = Position(p.z, a, b, c, d);
	float radius = fmaxf(5.0f, r + sinf(p.z * e.x) * e.y) / 10000.0f;
	pgl_vec2 diff = v2_sub(make_v2(p.x, p.y), pos);
	return radius / v2_dot(diff, diff);
}

float Dist2D(pgl_vec3 pos)
{
	float d = 0.0f;

	d += Distance_(pos, cama, camb, camc, camd, make_v2(2.1913f, 15.4634f), 70.0000f);
	d += Distance_(pos, lighta, lightb, lightc, lightd, make_v2(0.3814f, 12.7206f), 17.0590f);
	
	return d;
}

inline pgl_vec3 nmap(pgl_vec2 t, GLuint tx, float str)
{
	float d = 1.0f / 1024.0f;

	float xy = texture2D(tx, t.x, t.y).x;
	float x2 = texture2D(tx, t.x + d, t.y).x;
	float y2 = texture2D(tx, t.x, t.y + d).x;

	float s = (1.0f - str) * 1.2f;
	s *= s;
	s *= s;

	return v3_norm(make_v3(x2 - xy, y2 - xy, s / 8.0f));
}

void the_cave_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	float globaltime = ((My_Uniforms*)uniforms)->globaltime;
	GLuint iChannel0 = ((My_Uniforms*)uniforms)->tex6;
	GLuint iChannel1 = ((My_Uniforms*)uniforms)->tex1;
	GLuint iChannel2 = ((My_Uniforms*)uniforms)->tex9;

	pgl_vec2 iResolution = make_v2(WIDTH, HEIGHT);
	pgl_vec2 gl_FragCoord = *(pgl_vec2*)(&builtins->gl_FragCoord);

	float time = globaltime / 12.0f + 291.0f;

	pgl_vec2 p1 = Position(time + 0.05f, cama, camb, camc, camd);
	pgl_vec3 Pos = Position3D(time, cama, camb, camc, camd);
	pgl_vec3 oPos = Pos;

	pgl_vec3 CamDir = v3_norm(make_v3(p1.x - Pos.x, -p1.y + Pos.y, 0.1f));
	pgl_vec3 CamRight = v3_norm(v3_cross(CamDir, make_v3(0, 1, 0)));
	pgl_vec3 CamUp = v3_norm(v3_cross(CamRight, CamDir));

	pgl_vec2 uv = v2_sub(v2_scale(gl_FragCoord, 2.0f / iResolution.x), make_v2(1.0f, 1.0f));
	uv.y = uv.y * (iResolution.y / iResolution.x);
	float aspect = iResolution.x / iResolution.y;

	pgl_vec3 Dir = v3_norm(make_v3(uv.x * aspect, uv.y, 1.0f));
	// Apply camera rotation (simplified)
	Dir = v3_add(v3_add(v3_scale(CamRight, Dir.x), v3_scale(CamUp, Dir.y)), v3_scale(CamDir, Dir.z));

	float fade = 0.0f;
	const float numit = 75.0f;
	const float threshold = 1.20f;
	const float scale = 1.5f;

	pgl_vec3 Posm1 = Pos;

	for (float x = 0.0f; x < numit; x++)
	{
		if (Dist2D(Pos) < threshold)
		{
			fade = 1.0f - x / numit;
			break;
		}
		Posm1 = Pos;
		Pos = v3_add(Pos, v3_scale(Dir, scale / numit));
	}

	for (int x = 0; x < 6; x++)
	{
		pgl_vec3 p2 = v3_scale(v3_add(Posm1, Pos), 0.5f);
		if (Dist2D(p2) < threshold)
			Pos = p2;
		else
			Posm1 = p2;
	}

	// Simplified lighting
	pgl_vec3 lp = Position3D(time + 0.5f, cama, camb, camc, camd);
	pgl_vec3 ld = v3_sub(lp, Pos);
	float lv = 1.0f;

	const float ShadowIT = 15.0f;
	for (float x = 1.0f; x < ShadowIT; x++) {
		if (Dist2D(v3_add(Pos, v3_scale(ld, x / ShadowIT))) < threshold) {
			lv = 0.0f;
			break;
		}
	}

	pgl_vec3 tuv = make_v3(Pos.x * 3.0f, Pos.y * 3.0f, Pos.z * 1.5f);
	float nms = 0.19f;
	
	pgl_vec4 tx_ = texture2D(iChannel0, tuv.y, tuv.z);
	pgl_vec4 ty_ = texture2D(iChannel1, tuv.x, tuv.z);
	pgl_vec4 tz_ = texture2D(iChannel2, tuv.x, tuv.y);
	
	pgl_vec4 col = make_v4(
		(tx_.x + ty_.x + tz_.x) * 0.33f,
		(tx_.y + ty_.y + tz_.y) * 0.33f,
		(tx_.z + ty_.z + tz_.z) * 0.33f,
		1.0f
	);

	pgl_vec4 diff = make_v4(lv * 1.2f + 0.2f, lv * 1.2f + 0.2f, lv * 1.2f + 0.2f, 1.0f);
	
	float ff = fminf(1.0f, fade * 10.0f);
	*(pgl_vec4*)&builtins->gl_FragColor = make_v4(
		col.x * diff.x * ff,
		col.y * diff.y * ff,
		col.z * diff.z * ff,
		1.0f
	);
}

float eye_hash(float n)
{
	float x = sinf(n) * 43758.5453123f;
	return x - floorf(x);
}

float eye_noise(pgl_vec2 x)
{
	pgl_vec2 p = v2_floor(x);
	pgl_vec2 f = v2_sub(x, p);
	f = v2_mul(f, v2_mul(f, v2_sub(make_v2(3.0f, 3.0f), v2_scale(f, 2.0f))));

	float n = p.x + p.y * 57.0f;

	return mix_f(
		mix_f(eye_hash(n + 0.0f), eye_hash(n + 1.0f), f.x),
		mix_f(eye_hash(n + 57.0f), eye_hash(n + 58.0f), f.x),
		f.y
	);
}

float fbm(pgl_vec2 p)
{
	// Simplified fbm
	float f = 0.0f;
	f += 0.5000f * eye_noise(p); 
	f += 0.2500f * eye_noise(v2_scale(p, 2.0f));
	f += 0.1250f * eye_noise(v2_scale(p, 4.0f));
	f += 0.0625f * eye_noise(v2_scale(p, 8.0f));
	return f / 0.9375f;
}

void iqs_eyeball(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	float time = ((My_Uniforms*)uniforms)->globaltime;

	pgl_vec2 uv;
	uv.x = (*(pgl_vec2*)(&builtins->gl_FragCoord)).x / WIDTH;
	uv.y = (*(pgl_vec2*)(&builtins->gl_FragCoord)).y / HEIGHT;
	
	pgl_vec2 p;
	p.x = -1.0f + 2.0f * uv.x;
	p.y = -1.0f + 2.0f * uv.y;
	p.x *= WIDTH / (float)HEIGHT;

	float r = sqrtf(v2_dot(p, p));
	float a = atan2f(p.y, p.x);

	pgl_vec3 bg_col = make_v3(1.0f, 1.0f, 1.0f);
	pgl_vec3 col = bg_col;

	float ss = 0.5f + 0.5f * sinf(time);
	float anim = 1.0f + 0.1f * ss * clamp_f(1.0f - r, 0.0f, 1.0f);
	r *= anim;

	if (r < 0.8f) {
		col = make_v3(0.0f, 0.3f, 0.4f);

		float f = fbm(v2_scale(p, 5.0f));
		col = v3_mix(col, make_v3(0.2f, 0.5f, 0.4f), f);

		f = 1.0f - smoothstep_f(0.2f, 0.5f, r);
		col = v3_mix(col, make_v3(0.9f, 0.6f, 0.2f), f);

		a += 0.05f * fbm(v2_scale(p, 20.0f));

		f = smoothstep_f(0.3f, 1.0f, fbm(make_v2(10.0f * r, 20.0f * a)));
		col = v3_mix(col, make_v3(1.0f, 1.0f, 1.0f), f);

		f = smoothstep_f(0.4f, 0.9f, fbm(make_v2(10.0f * r, 15.0f * a)));
		col = v3_scale(col, 1.0f - 0.5f * f);

		f = smoothstep_f(0.6f, 0.8f, r);
		col = v3_scale(col, 1.0f - 0.5f * f);

		f = smoothstep_f(0.2f, 0.25f, r);
		col = v3_scale(col, f);

		pgl_vec2 pp = v2_sub(p, make_v2(0.24f, 0.2f));
		f = 1.0f - smoothstep_f(0.0f, 0.3f, sqrtf(v2_dot(pp, pp)));
		col = v3_add(col, v3_scale(make_v3(1.0f, 0.9f, 0.8f), f * 0.8f));

		f = smoothstep_f(0.75f, 0.8f, r);
		col = v3_mix(col, bg_col, f);
	}

	*(pgl_vec4*)&builtins->gl_FragColor = make_v4(col.x, col.y, col.z, 1.0f);
}

void setup_context()
{
	SDL_SetMainReady();
	if (SDL_Init(SDL_INIT_VIDEO)) {
		cout << "SDL_Init error: " << SDL_GetError() << "\n";
		exit(0);
	}

	window = SDL_CreateWindow("Shadertoy", 100, 100, WIDTH, HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
	if (!window) {
		cerr << "Failed to create window\n";
		SDL_Quit();
		exit(0);
	}

	ren = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
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
	SDL_DestroyWindow(window);
	SDL_Quit();
}
