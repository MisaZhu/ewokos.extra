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
#define GRID_SIZE 80
#define MAX_RIPPLES 10

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
float grid[GRID_SIZE][GRID_SIZE];
float time_val = 0;
int next_ripple_time = 0;
int next_drop_time = 0;

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

    pgl_set_max_vertices(PGL_SMALL_MAX_VERTICES);
    if (!init_glContext(&the_Context, &bbufpix, screen_width, screen_height)) {
        puts("Failed to initialize glContext");
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
            ripples[i].amplitude = random_float(0.15f, 0.3f);
            ripples[i].frequency = random_float(8.0f, 15.0f);
            ripples[i].phase = 0;
            ripples[i].decay = random_float(0.98f, 0.995f);
            ripples[i].active = 1;
            return;
        }
    }
}

// Create a water drop
void create_drop() {
    for (int i = 0; i < 5; i++) {
        if (!drops[i].active) {
            drops[i].x = random_float(-0.5f, 0.5f);
            drops[i].y = -2.5f; // Start from top of screen
            drops[i].z = random_float(-0.5f, 0.5f);
            drops[i].vy = 0;
            drops[i].radius = 0.04f; // Half size
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
            ripples[i].amplitude = random_float(0.2f, 0.35f);
            ripples[i].frequency = random_float(10.0f, 18.0f);
            ripples[i].phase = 0;
            ripples[i].decay = random_float(0.98f, 0.995f);
            ripples[i].active = 1;
            return;
        }
    }
}

// Update water drops
void update_drops() {
    for (int i = 0; i < 5; i++) {
        if (!drops[i].active) continue;

        drops[i].vy += 0.008f; // Slower gravity for gentle fall
        if (drops[i].vy > 0.08f) drops[i].vy = 0.08f; // Terminal velocity
        drops[i].y += drops[i].vy; // Increasing Y means falling down

        // Hit water surface at y=0
        if (drops[i].y >= 0) {
            create_ripple_at(drops[i].x, drops[i].z);
            drops[i].active = 0;
        }
    }
}

// Update ripples
void update_ripples() {
    for (int i = 0; i < MAX_RIPPLES; i++) {
        if (!ripples[i].active) continue;

        ripples[i].phase += 0.15f;
        ripples[i].amplitude *= ripples[i].decay;

        if (ripples[i].amplitude < 0.01f) {
            ripples[i].active = 0;
        }
    }

    time_val += 0.02f;
}

// Calculate water surface height at position
float get_water_height(float x, float z) {
    float height = 0;

    for (int i = 0; i < MAX_RIPPLES; i++) {
        if (!ripples[i].active) continue;

        float dx = x - ripples[i].x;
        float dz = z - ripples[i].z;
        float dist = sqrtf(dx * dx + dz * dz);

        // Wave formula: amplitude * sin(frequency * distance + phase) * attenuation
        float wave = ripples[i].amplitude * sinf(ripples[i].frequency * dist - ripples[i].phase);
        wave *= expf(-dist * 1.5f); // Distance attenuation

        height += wave;
    }

    return height;
}

// Forward declarations
void draw_line(int x0, int y0, int x1, int y1, uint8_t r0, uint8_t g0, uint8_t b0, uint8_t r1, uint8_t g1, uint8_t b1);
void project_3d(float x, float y, float z, float rot_x, float fov, float view_dist, int* sx, int* sy);
void create_ripple_at(float x, float z);

// Draw pixel
void set_pixel(int x, int y, uint8_t r, uint8_t g, uint8_t b) {
    if (x >= 0 && x < screen_width && y >= 0 && y < screen_height) {
        bbufpix[y * screen_width + x] = 0xFF000000 | (r << 16) | (g << 8) | b;
    }
}

// 3D projection function
void project_3d(float x, float y, float z, float rot_x, float fov, float view_dist, int* sx, int* sy) {
    float cos_rx = cosf(rot_x);
    float sin_rx = sinf(rot_x);

    float py = y * cos_rx - z * sin_rx;
    float pz = y * sin_rx + z * cos_rx;

    float scale = fov / (view_dist + pz);
    *sx = (int)(screen_width / 2 + x * scale * screen_width);
    *sy = (int)(screen_height / 2 + py * scale * screen_height);
}

// Draw 3D water surface grid
void draw_water() {
    if (!bbufpix) return;

    // Clear screen to black
    for (int i = 0; i < screen_width * screen_height; i++) {
        bbufpix[i] = 0xFF000000;
    }

    // 3D projection parameters
    float fov = 0.8f;
    float view_dist = 3.0f;
    float rot_x = 0.5f; // Viewing angle

    float cos_rx = cosf(rot_x);
    float sin_rx = sinf(rot_x);

    // Draw grid lines
    for (int i = 0; i < GRID_SIZE - 1; i++) {
        for (int j = 0; j < GRID_SIZE - 1; j++) {
            float x1 = (i - GRID_SIZE / 2) * 0.05f;
            float z1 = (j - GRID_SIZE / 2) * 0.05f;
            float x2 = (i + 1 - GRID_SIZE / 2) * 0.05f;
            float z2 = (j + 1 - GRID_SIZE / 2) * 0.05f;

            float y1 = get_water_height(x1, z1);
            float y2 = get_water_height(x2, z1);
            float y3 = get_water_height(x1, z2);

            // 3D rotation and projection
            float py1 = y1 * cos_rx - z1 * sin_rx;
            float pz1 = y1 * sin_rx + z1 * cos_rx;
            float py2 = y2 * cos_rx - z1 * sin_rx;
            float pz2 = y2 * sin_rx + z1 * cos_rx;
            float py3 = y3 * cos_rx - z2 * sin_rx;
            float pz3 = y3 * sin_rx + z2 * cos_rx;

            // Perspective projection
            float scale1 = fov / (view_dist + pz1);
            float scale2 = fov / (view_dist + pz2);
            float scale3 = fov / (view_dist + pz3);

            int sx1 = (int)(screen_width / 2 + x1 * scale1 * screen_width);
            int sy1 = (int)(screen_height / 2 + py1 * scale1 * screen_height);
            int sx2 = (int)(screen_width / 2 + x2 * scale2 * screen_width);
            int sy2 = (int)(screen_height / 2 + py2 * scale2 * screen_height);
            int sx3 = (int)(screen_width / 2 + x1 * scale3 * screen_width);
            int sy3 = (int)(screen_height / 2 + py3 * scale3 * screen_height);

            // Calculate color brightness based on height
            float brightness1 = 0.3f + (y1 + 0.3f) * 0.8f;
            float brightness2 = 0.3f + (y2 + 0.3f) * 0.8f;
            float brightness3 = 0.3f + (y3 + 0.3f) * 0.8f;

            if (brightness1 < 0.1f) brightness1 = 0.1f;
            if (brightness1 > 1.0f) brightness1 = 1.0f;
            if (brightness2 < 0.1f) brightness2 = 0.1f;
            if (brightness2 > 1.0f) brightness2 = 1.0f;
            if (brightness3 < 0.1f) brightness3 = 0.1f;
            if (brightness3 > 1.0f) brightness3 = 1.0f;

            uint8_t r1 = (uint8_t)(10 * brightness1);
            uint8_t g1 = (uint8_t)(30 * brightness1);
            uint8_t b1 = (uint8_t)(100 + 155 * brightness1);

            uint8_t r2 = (uint8_t)(10 * brightness2);
            uint8_t g2 = (uint8_t)(30 * brightness2);
            uint8_t b2 = (uint8_t)(100 + 155 * brightness2);

            // Draw line segments
            draw_line(sx1, sy1, sx2, sy2, r1, g1, b1, r2, g2, b2);
            draw_line(sx1, sy1, sx3, sy3, r1, g1, b1, r2, g2, b2);
        }
    }
}

// Draw water drop mesh model
void draw_drops() {
    if (!bbufpix) return;

    float fov = 0.8f;
    float view_dist = 3.0f;
    float rot_x = 0.5f;

    for (int d = 0; d < 5; d++) {
        if (!drops[d].active) continue;

        WaterDrop* drop = &drops[d];
        float cx = drop->x;
        float cy = drop->y;
        float cz = drop->z;
        float r = drop->radius;

        // Draw water drop mesh using latitude/longitude grid
        int lat_lines = 8;
        int lon_lines = 12;

        for (int i = 0; i < lat_lines; i++) {
            for (int j = 0; j < lon_lines; j++) {
                float phi1 = (float)j / lon_lines * 2 * M_PI;
                float phi2 = (float)(j + 1) / lon_lines * 2 * M_PI;

                // Calculate points - teardrop shape (rounded top, pointed bottom)
                // Use modified spherical coordinates for teardrop shape
                float t1 = (float)i / lat_lines;
                float t2 = (float)(i + 1) / lat_lines;

                // Teardrop profile: wider at top, tapering to point at bottom
                float profile1 = sinf(t1 * M_PI) * (1.0f - t1 * 0.3f);
                float profile2 = sinf(t2 * M_PI) * (1.0f - t2 * 0.3f);

                // Vertical position - stretch to make elongated
                float y_scale = 2.0f;
                float y1 = cy + r * y_scale * cosf(t1 * M_PI);
                float y2 = cy + r * y_scale * cosf(t1 * M_PI); // Same latitude
                float y3 = cy + r * y_scale * cosf(t2 * M_PI);

                float x1 = cx + r * profile1 * cosf(phi1);
                float z1 = cz + r * profile1 * sinf(phi1);

                float x2 = cx + r * profile1 * cosf(phi2);
                float z2 = cz + r * profile1 * sinf(phi2);

                float x3 = cx + r * profile2 * cosf(phi1);
                float z3 = cz + r * profile2 * sinf(phi1);

                int sx1, sy1, sx2, sy2, sx3, sy3;
                project_3d(x1, y1, z1, rot_x, fov, view_dist, &sx1, &sy1);
                project_3d(x2, y2, z2, rot_x, fov, view_dist, &sx2, &sy2);
                project_3d(x3, y3, z3, rot_x, fov, view_dist, &sx3, &sy3);

                // Water drop color - blue
                uint8_t r_col = 80;
                uint8_t g_col = 150;
                uint8_t b_col = 255;

                draw_line(sx1, sy1, sx2, sy2, r_col, g_col, b_col, r_col, g_col, b_col);
                draw_line(sx1, sy1, sx3, sy3, r_col, g_col, b_col, r_col, g_col, b_col);
            }
        }
    }
}

// Draw line using Bresenham's algorithm
void draw_line(int x0, int y0, int x1, int y1, uint8_t r0, uint8_t g0, uint8_t b0, uint8_t r1, uint8_t g1, uint8_t b1) {
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    while (1) {
        // Interpolate color
        float t = (dx > dy) ? (float)abs(x0 - x1) / dx : (float)abs(y0 - y1) / dy;
        if (t < 0) t = 0;
        if (t > 1) t = 1;

        uint8_t r = (uint8_t)(r0 * (1 - t) + r1 * t);
        uint8_t g = (uint8_t)(g0 * (1 - t) + g1 * t);
        uint8_t b = (uint8_t)(b0 * (1 - t) + b1 * t);

        set_pixel(x0, y0, r, g, b);

        if (x0 == x1 && y0 == y1) break;

        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

int handle_events() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_KEYDOWN:
            if (event.key.keysym.sym == SDLK_ESCAPE) return 1;
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

    next_drop_time = 60; // First drop after 1 second

    int frame_count = 0;

    while (1) {
        if (handle_events()) break;

        if (frame_count >= next_drop_time) {
            create_drop();
            next_drop_time = frame_count + 120; // One drop per second
        }

        update_ripples();
        update_drops();

        if (bbufpix) {
            draw_water();
            draw_drops();
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
