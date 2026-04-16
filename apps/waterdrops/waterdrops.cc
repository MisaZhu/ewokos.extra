#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include "glcommon/gltools.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <ewoksys/klog.h>

#define WIDTH 1024
#define HEIGHT 768
#define PIX_FORMAT SDL_PIXELFORMAT_ARGB8888
#define GRID_SIZE 120
#define MAX_RIPPLES 10
#define MAX_VERTICES (GRID_SIZE * GRID_SIZE * 6)
#define MAX_DROP_VERTICES 1024

#define sinf pgl_fast_sin
#define cosf pgl_fast_cos
#define sqrtf pgl_fast_sqrt
#define tanf pgl_fast_tan
#define acosf pgl_fast_acos
#define asinf pgl_fast_asin
#define atanf pgl_fast_atan
#define atanf2 pgl_fast_atan2

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Ripple source
typedef struct {
    float x, z;
    float amplitude;
    float frequency;
    float phase;
    float decay;
    int active;
} Ripple;

// Water drop
typedef struct {
    float x, y, z;
    float vy;
    float radius;
    int active;
} WaterDrop;

// Global variables
SDL_Window* window = NULL;
SDL_Renderer* ren = NULL;
SDL_Texture* tex = NULL;
glContext the_Context;
pix_t* bbufpix = NULL;
int screen_width, screen_height;

Ripple ripples[MAX_RIPPLES];
WaterDrop drops[5];
float time_val = 0;
int next_drop_time = 0;
int wireframe_mode = 0;  // 0 = filled, 1 = wireframe
int last_mode_switch_time = 0;  // For automatic mode switching

// Shader program
GLuint water_program = 0;
GLuint drop_program = 0;

// Vertex data for OpenGL
float water_vertices[MAX_VERTICES * 2];  // x, y
float water_colors[MAX_VERTICES * 4];    // r, g, b, a
int water_vertex_count = 0;

float drop_vertices[MAX_DROP_VERTICES * 2];
float drop_colors[MAX_DROP_VERTICES * 4];
int drop_vertex_count = 0;

// Simple vertex shader for water - just pass through position and color
void water_vertex_shader(float* vs_output, vec4* v_attrs, Shader_Builtins* builtins, void* uniforms)
{
    // v_attrs[0] = position (vec4)
    // v_attrs[1] = color (vec4)
    // Pass color to fragment shader
    vec4* vs_out = (vec4*)vs_output;
    vs_out[0] = v_attrs[1];  // Pass color
    
    // Set position
    builtins->gl_Position = v_attrs[0];
}

// Simple fragment shader for water
void water_fragment_shader(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
    vec4 color = ((vec4*)fs_input)[0];
    builtins->gl_FragColor = color;
}

// Vertex shader for drops
void drop_vertex_shader(float* vs_output, vec4* v_attrs, Shader_Builtins* builtins, void* uniforms)
{
    vec4* vs_out = (vec4*)vs_output;
    vs_out[0] = v_attrs[1];  // Pass color
    builtins->gl_Position = v_attrs[0];
}

// Fragment shader for drops
void drop_fragment_shader(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
    vec4 color = ((vec4*)fs_input)[0];
    builtins->gl_FragColor = color;
}

// Initialize SDL and OpenGL context
void setup_context() {
    if (SDL_Init(SDL_INIT_VIDEO)) {
        klog("SDL_Init error: %s\n", SDL_GetError());
        exit(0);
    }

    window = SDL_CreateWindow("Water Drops", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              WIDTH, HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN);
    if (!window) exit(0);

    SDL_GetWindowSize(window, &screen_width, &screen_height);
    klog("Waterdrops: window size %dx%d\n", screen_width, screen_height);

    ren = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (!ren) exit(0);

    tex = SDL_CreateTexture(ren, PIX_FORMAT, SDL_TEXTUREACCESS_STREAMING, screen_width, screen_height);
    if (!tex) exit(0);

    pgl_set_max_vertices(MAX_VERTICES + MAX_DROP_VERTICES);
    if (!init_glContext(&the_Context, &bbufpix, screen_width, screen_height)) {
        puts("Failed to initialize glContext");
        exit(0);
    }
    
    // Create shader programs
    GLenum smooth[4] = { PGL_SMOOTH4 };
    
    water_program = pglCreateProgram(water_vertex_shader, water_fragment_shader, 4, smooth, GL_FALSE);
    if (water_program == 0) {
        klog("Failed to create water shader program\n");
        exit(0);
    }
    
    drop_program = pglCreateProgram(drop_vertex_shader, drop_fragment_shader, 4, smooth, GL_FALSE);
    if (drop_program == 0) {
        klog("Failed to create drop shader program\n");
        exit(0);
    }
}

void cleanup() {
    free_glContext(&the_Context);
    if (tex) {
        SDL_DestroyTexture(tex);
        tex = NULL;
    }
    if (ren) {
        SDL_DestroyRenderer(ren);
        ren = NULL;
    }
    if (window) {
        SDL_DestroyWindow(window);
        window = NULL;
    }
    SDL_Quit();
}

float random_float(float min, float max) {
    return min + (float)rand() / RAND_MAX * (max - min);
}

// Create a ripple at random position
void create_ripple() {
    for (int i = 0; i < MAX_RIPPLES; i++) {
        if (!ripples[i].active) {
            ripples[i].x = random_float(-0.8f, 0.8f);
            ripples[i].z = random_float(-0.8f, 0.8f);
            ripples[i].amplitude = random_float(0.03f, 0.06f);  // Slightly larger initial amplitude
            ripples[i].frequency = random_float(12.0f, 20.0f);  // Higher frequency for denser ripples
            ripples[i].phase = 0;
            ripples[i].decay = random_float(0.92f, 0.96f);      // Faster decay for quicker fade
            ripples[i].active = 1;
            return;
        }
    }
}

// Create a water drop
void create_drop() {
    for (int i = 0; i < 5; i++) {
        if (!drops[i].active) {
            drops[i].x = random_float(-0.9f, 0.9f);
            drops[i].y = 2.5f;  // Start from top of screen
            drops[i].z = random_float(-0.9f, 0.9f);
            drops[i].vy = 0;
            drops[i].radius = 0.04f;
            drops[i].active = 1;
            return;
        }
    }
}

// Create ripple at specific position
void create_ripple_at(float x, float z) {
    for (int i = 0; i < MAX_RIPPLES; i++) {
        if (!ripples[i].active) {
            ripples[i].x = x;
            ripples[i].z = z;
            ripples[i].amplitude = random_float(0.04f, 0.08f);  // Slightly larger initial amplitude
            ripples[i].frequency = random_float(15.0f, 25.0f);  // Higher frequency for denser ripples
            ripples[i].phase = 0;
            ripples[i].decay = random_float(0.92f, 0.96f);      // Faster decay for quicker fade
            ripples[i].active = 1;
            return;
        }
    }
}

// Update water drops
void update_drops() {
    for (int i = 0; i < 5; i++) {
        if (!drops[i].active) continue;

        drops[i].vy += 0.015f;  // Stronger gravity for faster fall
        if (drops[i].vy > 0.15f) drops[i].vy = 0.15f;  // Higher terminal velocity
        drops[i].y -= drops[i].vy;  // Move downward (decrease y)

        if (drops[i].y <= 0) {  // Hit water surface
            create_ripple_at(drops[i].x, drops[i].z);
            drops[i].active = 0;
        }
    }
}

// Update ripples
void update_ripples() {
    for (int i = 0; i < MAX_RIPPLES; i++) {
        if (!ripples[i].active) continue;

        ripples[i].phase += 0.5f;  // Even faster wave propagation for spreading effect
        // Use smoother衰减 curve
        ripples[i].amplitude *= ripples[i].decay;

        // Smoother fade out when amplitude gets small
        if (ripples[i].amplitude < 0.01f) {
            ripples[i].amplitude *= 0.9f;  // Extra smooth衰减 for final fade
            if (ripples[i].amplitude < 0.002f) {
                ripples[i].active = 0;
            }
        }
    }

    time_val += 0.05f;  // Faster time progression
}

// Calculate water surface height at position
float get_water_height(float x, float z) {
    float height = 0;

    for (int i = 0; i < MAX_RIPPLES; i++) {
        if (!ripples[i].active) continue;

        float dx = x - ripples[i].x;
        float dz = z - ripples[i].z;
        float dist = sqrtf(dx * dx + dz * dz);

        // Wave with distance-based phase for spreading effect
        // The wave travels outward from center: phase increases with distance
        float wave = ripples[i].amplitude * sinf(ripples[i].frequency * dist - ripples[i].phase * 0.5f);
        wave *= expf(-dist * 1.2f);  // Distance attenuation

        height += wave;
    }

    return height;
}

// Add vertex to water buffer
void add_water_vertex(float x, float y, float r, float g, float b, float a) {
    if (water_vertex_count >= MAX_VERTICES) return;
    
    int idx = water_vertex_count * 2;
    water_vertices[idx] = x;
    water_vertices[idx + 1] = y;
    
    int cidx = water_vertex_count * 4;
    water_colors[cidx] = r;
    water_colors[cidx + 1] = g;
    water_colors[cidx + 2] = b;
    water_colors[cidx + 3] = a;
    
    water_vertex_count++;
}

// Add vertex to drop buffer
void add_drop_vertex(float x, float y, float r, float g, float b, float a) {
    if (drop_vertex_count >= MAX_DROP_VERTICES) return;
    
    int idx = drop_vertex_count * 2;
    drop_vertices[idx] = x;
    drop_vertices[idx + 1] = y;
    
    int cidx = drop_vertex_count * 4;
    drop_colors[cidx] = r;
    drop_colors[cidx + 1] = g;
    drop_colors[cidx + 2] = b;
    drop_colors[cidx + 3] = a;
    
    drop_vertex_count++;
}

// Project a 3D point to screen coordinates
void project_point(float x, float y, float z, float cos_a, float sin_a, 
                   float camera_distance, float near_plane, float fov_scale, float aspect,
                   float* sx, float* sy) {
    // Transform to camera space
    float cy = y * cos_a - z * sin_a;
    float cz = y * sin_a + z * cos_a + camera_distance;
    
    // Perspective projection
    float scale = fov_scale / (cz + near_plane);
    
    // Screen coordinates
    *sx = x * scale / aspect;
    *sy = -cy * scale;  // Invert Y
}

// Build water mesh using OpenGL
void build_water_mesh() {
    water_vertex_count = 0;
    
    // Camera/view parameters - higher angle from above
    float camera_distance = 3.0f;
    float view_angle = 0.8f;  // More tilt for higher viewing angle
    
    // Screen aspect ratio correction
    float aspect = (float)screen_width / screen_height;
    
    // Precompute rotation
    float cos_a = cosf(view_angle);
    float sin_a = sinf(view_angle);
    
    // Projection parameters
    float near_plane = 0.1f;
    float fov_scale = 1.8f;
    
    // Grid parameters
    float grid_scale = 0.05f;
    float grid_offset = GRID_SIZE * grid_scale * 0.5f;
    
    for (int i = 0; i < GRID_SIZE - 1; i++) {
        for (int j = 0; j < GRID_SIZE - 1; j++) {
            // Grid coordinates (x, z plane)
            float x1 = i * grid_scale - grid_offset;
            float z1 = j * grid_scale - grid_offset;
            float x2 = (i + 1) * grid_scale - grid_offset;
            float z2 = (j + 1) * grid_scale - grid_offset;
            
            // Get water height at each corner
            float y1 = get_water_height(x1, z1);
            float y2 = get_water_height(x2, z1);
            float y3 = get_water_height(x1, z2);
            float y4 = get_water_height(x2, z2);
            
            // Project all 4 vertices
            float sx1, sy1, sx2, sy2, sx3, sy3, sx4, sy4;
            project_point(x1, y1, z1, cos_a, sin_a, camera_distance, near_plane, fov_scale, aspect, &sx1, &sy1);
            project_point(x2, y2, z1, cos_a, sin_a, camera_distance, near_plane, fov_scale, aspect, &sx2, &sy2);
            project_point(x1, y3, z2, cos_a, sin_a, camera_distance, near_plane, fov_scale, aspect, &sx3, &sy3);
            project_point(x2, y4, z2, cos_a, sin_a, camera_distance, near_plane, fov_scale, aspect, &sx4, &sy4);
            
            // Calculate brightness per vertex for ripple shading effect - maximum contrast
            // y > 0 means peak (up), should be brighter; y < 0 means valley (down), should be darker
            float brightness1 = 0.5f + y1 * 5.0f;
            float brightness2 = 0.5f + y2 * 5.0f;
            float brightness3 = 0.5f + y3 * 5.0f;
            float brightness4 = 0.5f + y4 * 5.0f;

            if (brightness1 < 0.0f) brightness1 = 0.0f;
            if (brightness1 > 1.0f) brightness1 = 1.0f;
            if (brightness2 < 0.0f) brightness2 = 0.0f;
            if (brightness2 > 1.0f) brightness2 = 1.0f;
            if (brightness3 < 0.0f) brightness3 = 0.0f;
            if (brightness3 > 1.0f) brightness3 = 1.0f;
            if (brightness4 < 0.0f) brightness4 = 0.0f;
            if (brightness4 > 1.0f) brightness4 = 1.0f;

            // Deep blue water color with maximum contrast - peaks bright, valleys dark
            float r1 = 0.0f, g1 = 0.0f + 0.5f * brightness1, b1 = 0.1f + 0.8f * brightness1;
            float r2 = 0.0f, g2 = 0.0f + 0.5f * brightness2, b2 = 0.1f + 0.8f * brightness2;
            float r3 = 0.0f, g3 = 0.0f + 0.5f * brightness3, b3 = 0.1f + 0.8f * brightness3;
            float r4 = 0.0f, g4 = 0.0f + 0.5f * brightness4, b4 = 0.1f + 0.8f * brightness4;
            
            // Triangle 1: (x1,z1), (x2,z1), (x2,z2)
            add_water_vertex(sx1, sy1, r1, g1, b1, 1.0f);
            add_water_vertex(sx2, sy2, r2, g2, b2, 1.0f);
            add_water_vertex(sx4, sy4, r4, g4, b4, 1.0f);
            
            // Triangle 2: (x1,z1), (x2,z2), (x1,z2)
            add_water_vertex(sx1, sy1, r1, g1, b1, 1.0f);
            add_water_vertex(sx4, sy4, r4, g4, b4, 1.0f);
            add_water_vertex(sx3, sy3, r3, g3, b3, 1.0f);
        }
    }
}

// Draw water using OpenGL with shader-based API
void draw_water_gl() {
    build_water_mesh();
    
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Use water shader program
    glUseProgram(water_program);
    
    // Set up vertex attributes
    glEnableVertexAttribArray(0);  // Position attribute
    glEnableVertexAttribArray(1);  // Color attribute
    
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, water_vertices);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, water_colors);
    
    // Draw filled triangles or wireframe lines based on mode
    if (wireframe_mode) {
        glDrawArrays(GL_LINES, 0, water_vertex_count);
    } else {
        glDrawArrays(GL_TRIANGLES, 0, water_vertex_count);
    }

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);

    glDisable(GL_BLEND);
}

// Build drop mesh
void build_drop_mesh() {
    drop_vertex_count = 0;
    
    // Same camera parameters as water mesh
    float camera_distance = 4.0f;
    float view_angle = 0.6f;
    float aspect = (float)screen_width / screen_height;

    float cos_a = cosf(view_angle);
    float sin_a = sinf(view_angle);

    float near_plane = 0.1f;
    float fov_scale = 1.8f;
    
    for (int d = 0; d < 5; d++) {
        if (!drops[d].active) continue;
        
        WaterDrop* drop = &drops[d];
        float cx = drop->x;
        float cy = drop->y;
        float cz = drop->z;
        float r = drop->radius;
        
        int lat_lines = 6;
        int lon_lines = 8;
        
        float dr = 0.3f;
        float dg = 0.6f;
        float db = 1.0f;
        float da = 0.8f;
        
        for (int i = 0; i < lat_lines; i++) {
            float t1 = (float)i / lat_lines;
            
            float profile1 = sinf(t1 * M_PI) * (1.0f - t1 * 0.3f);
            
            float y_scale = 2.0f;
            float y1 = cy + r * y_scale * cosf(t1 * M_PI);
            
            for (int j = 0; j < lon_lines; j++) {
                float phi1 = (float)j / lon_lines * 2 * M_PI;
                float phi2 = (float)(j + 1) / lon_lines * 2 * M_PI;
                
                float x1 = cx + r * profile1 * cosf(phi1);
                float z1 = cz + r * profile1 * sinf(phi1);
                float x2 = cx + r * profile1 * cosf(phi2);
                float z2 = cz + r * profile1 * sinf(phi2);
                
                // Transform to camera space
                float py1 = y1 * cos_a - z1 * sin_a;
                float pz1 = y1 * sin_a + z1 * cos_a + camera_distance;
                float py2 = y1 * cos_a - z2 * sin_a;
                float pz2 = y1 * sin_a + z2 * cos_a + camera_distance;
                
                // Perspective projection
                float scale1 = fov_scale / (pz1 + near_plane);
                float scale2 = fov_scale / (pz2 + near_plane);
                
                // Screen coordinates with aspect correction
                float sx1 = x1 * scale1 / aspect;
                float sy1 = py1 * scale1;
                float sx2 = x2 * scale2 / aspect;
                float sy2 = py2 * scale2;
                
                add_drop_vertex(sx1, sy1, dr, dg, db, da);
                add_drop_vertex(sx2, sy2, dr, dg, db, da);
            }
        }
    }
}

// Draw water drop using OpenGL with shader-based API
void draw_drops_gl() {
    build_drop_mesh();
    
    if (drop_vertex_count == 0) return;
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Use drop shader program
    glUseProgram(drop_program);
    
    // Set up vertex attributes
    glEnableVertexAttribArray(0);  // Position attribute
    glEnableVertexAttribArray(1);  // Color attribute
    
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, drop_vertices);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, drop_colors);
    
    glDrawArrays(GL_LINES, 0, drop_vertex_count);
    
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    
    glDisable(GL_BLEND);
}

int handle_events() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_KEYDOWN:
            if (event.key.keysym.sym == SDLK_ESCAPE) return 1;
            if (event.key.keysym.sym == SDLK_a) {
                wireframe_mode = !wireframe_mode;  // Toggle wireframe mode
            }
            break;
        case SDL_MOUSEBUTTONDOWN:
            return 1;
        case SDL_WINDOWEVENT:
            if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
                return 1;
            }
            break;
        case SDL_QUIT:
            return 1;
        }
    }
    return 0;
}

int main(int argc, char** argv) {
    (void)argc; (void)argv;
    srand((unsigned int)time(NULL));

    setup_context();

    next_drop_time = 0;
    int frame_count = 0;

    while (1) {
        if (handle_events()) break;

        // Auto switch mode every 2 seconds (assuming 60fps, 120 frames)
        if (frame_count - last_mode_switch_time >= 120) {
            wireframe_mode = !wireframe_mode;
            last_mode_switch_time = frame_count;
        }

        if (frame_count >= next_drop_time) {
            // Create a single drop at a time for more natural rain effect
            create_drop();
            // Random interval between drops for natural distribution
            next_drop_time = frame_count + 5 + rand() % 25;  // Random interval 5-30 frames
        }

        update_ripples();
        update_drops();

        if (bbufpix) {
            draw_water_gl();
            draw_drops_gl();
            SDL_UpdateTexture(tex, NULL, bbufpix, screen_width * sizeof(pix_t));
        }

        SDL_RenderClear(ren);
        SDL_RenderCopy(ren, tex, NULL, NULL);
        SDL_RenderPresent(ren);

        frame_count++;
        SDL_Delay(16);
    }

    cleanup();
    return 0;
}
