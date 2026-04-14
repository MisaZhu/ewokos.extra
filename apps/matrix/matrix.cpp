#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#include "glcommon/gltools.h"
#include "glcommon/gltext.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <vector>

#include <ewoksys/klog.h>

#define WIDTH 640
#define HEIGHT 480
#define FONT_SIZE 14
#define FONT_PATH "/usr/system/fonts/Hack-Regular.ttf"
#define MAX_WIDTH 3840
#define MAX_HEIGHT 2160

#define PIX_FORMAT SDL_PIXELFORMAT_ARGB8888

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

const char* MATRIX_CHARS = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789@#$%^&*()_+-=[]{}|;:,.<>?";

unsigned char* font_buffer = NULL;
stbtt_fontinfo font;
int font_ascent, font_descent, font_linegap;
float font_scale = 0;
#define MAX_CHAR_BITMAP_SIZE 64
static unsigned char char_bitmap_buf[MAX_CHAR_BITMAP_SIZE * MAX_CHAR_BITMAP_SIZE];

int is_font_loaded = 0;

struct Column {
    float y;
    float speed;
    int length;
    std::vector<char> chars;

    Column() {
        reset();
    }

    void reset() {
        y = (float)(rand() % HEIGHT) - HEIGHT;
        speed = 2.0f + (float)(rand() % 100) / 50.0f;
        length = 5 + rand() % 20;
        chars.clear();
        size_t char_count = strlen(MATRIX_CHARS);
        if (char_count == 0) char_count = 1;
        for (int i = 0; i < length; i++) {
            chars.push_back(MATRIX_CHARS[rand() % char_count]);
        }
    }

    void update(float dt) {
        y += speed * dt * 60.0f;
        if (y - length * FONT_SIZE > HEIGHT) {
            reset();
        }
        if (rand() % 10 == 0) {
            size_t char_count = strlen(MATRIX_CHARS);
            if (char_count == 0) char_count = 1;

            int idx = rand() % chars.size();
            chars[idx] = MATRIX_CHARS[rand() % char_count];
        }
    }
};

SDL_Window* window;
SDL_Renderer* ren;
SDL_Texture* tex;
glContext the_Context;
pix_t* bbufpix;

int width, height;
int buf_width, buf_height;

volatile int needs_resize_skip;

std::vector<Column> columns;

void setup_context() {
    if (SDL_Init(SDL_INIT_VIDEO)) {
        klog("SDL_Init error: %s\n", SDL_GetError());
        exit(0);
    }

    width = WIDTH;
    height = HEIGHT;

    window = SDL_CreateWindow("Matrix Rain", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!window) exit(0);

    ren = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (!ren) exit(0);

    tex = SDL_CreateTexture(ren, PIX_FORMAT, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
    if (!tex) exit(0);

    pgl_set_max_vertices(PGL_SMALL_MAX_VERTICES);
    if (!init_glContext(&the_Context, &bbufpix, WIDTH, HEIGHT)) {
        puts("Failed to initialize glContext");
        exit(0);
    }

    buf_width = WIDTH;
    buf_height = HEIGHT;
    needs_resize_skip = 0;
}

void cleanup() {
    if (font_buffer) free(font_buffer);
    free_glContext(&the_Context);
    if (tex) SDL_DestroyTexture(tex);
    if (ren) SDL_DestroyRenderer(ren);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
}

int handle_events();

int load_font() {
    FILE* f = fopen(FONT_PATH, "rb");
    if (!f) return 0;

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (fsize <= 0 || fsize > 10 * 1024 * 1024) {
        fclose(f);
        return 0;
    }

    font_buffer = (unsigned char*)malloc(fsize);
    if (!font_buffer) {
        fclose(f);
        return 0;
    }

    fread(font_buffer, 1, fsize, f);
    fclose(f);

    if (!stbtt_InitFont(&font, font_buffer, 0)) {
        free(font_buffer);
        font_buffer = NULL;
        return 0;
    }

    stbtt_GetFontVMetrics(&font, &font_ascent, &font_descent, &font_linegap);
    font_scale = stbtt_ScaleForPixelHeight(&font, FONT_SIZE);
    is_font_loaded = 1;
    klog("Font loaded OK\n");

    return 1;
}

int main(int argc, char** argv) {
    (void)argc; (void)argv;
    srand((unsigned int)time(NULL));

    setup_context();

    load_font();

    int num_columns = width / FONT_SIZE;
    if (num_columns <= 0) num_columns = 1;
    columns.resize(num_columns);

    for (size_t i = 0; i < columns.size(); i++) {
        columns[i].y = (float)(rand() % (HEIGHT + HEIGHT)) - HEIGHT;
    }

    unsigned int last_time = SDL_GetTicks();
    int frame_count = 0;
    unsigned int fps_time = 0;

    while (1) {
        unsigned int current_time = SDL_GetTicks();
        float dt = (current_time - last_time) / 1000.0f;
        if (dt > 0.5f) dt = 0.016f;
        last_time = current_time;

        klog("handle_events\n");
        if (handle_events()) break;
        klog("handle_events done\n");


        frame_count++;
        if (current_time - fps_time > 3000) {
            klog("%d FPS (%dx%d)\n", frame_count * 1000 / (current_time - fps_time), width, height);
            frame_count = 0;
            fps_time = current_time;
        }

        if (needs_resize_skip > 0) {
            needs_resize_skip--;
            SDL_Delay(16);
            continue;
        }

        long total_pixels = (long)width * height;
        for (long i = 0; i < total_pixels; ++i) {
            bbufpix[i] = 0xFF000000;
        }
        klog("update columns\n");

        if (is_font_loaded) {
            for (size_t ci = 0; ci < columns.size(); ci++) {
                columns[ci].update(dt);

                Column& col = columns[ci];
                int col_x = (int)(ci * FONT_SIZE);
                if (col_x >= width) continue;

                for (int j = 0; j < col.length; j++) {
                    int char_y = (int)(col.y - j * FONT_SIZE);
                    if (char_y < -FONT_SIZE || char_y >= height + FONT_SIZE) continue;

                    char c = col.chars[j];

                    int advance, lsb, x0, y0, x1, y1;
                    stbtt_GetCodepointHMetrics(&font, c, &advance, &lsb);
                    stbtt_GetCodepointBitmapBox(&font, c, font_scale, font_scale, &x0, &y0, &x1, &y1);

                    int w = x1 - x0;
                    int h = y1 - y0;
                    if (w <= 0 || h <= 0) continue;
                    if (w > MAX_CHAR_BITMAP_SIZE || h > MAX_CHAR_BITMAP_SIZE) continue;

                    stbtt_MakeCodepointBitmap(&font, char_bitmap_buf, w, h, w, font_scale, font_scale, c);

                    int baseline_y = char_y + (int)(font_ascent * font_scale) + y0;

                    float brightness = 1.0f - (float)j / col.length;
                    uint32_t color;
                    if (j == 0) {
                        color = 0xFFFFFFFF;
                    } else {
                        unsigned char g_val = (unsigned char)(brightness * 255);
                        unsigned char r_val = (unsigned char)(brightness * 50);
                        color = 0xFF000000 | ((uint32_t)r_val << 16) | ((uint32_t)g_val << 8);
                    }

                    for (int row = 0; row < h; row++) {
                        int py = baseline_y + row;
                        if (py < 0 || py >= height) continue;

                        for (int col_idx = 0; col_idx < w; col_idx++) {
                            int px = col_x + col_idx;
                            if (px < 0 || px >= width) continue;

                            unsigned char alpha = char_bitmap_buf[row * w + col_idx];
                            if (alpha > 0) {
                                long idx = ((long)py * width) + px;
                                bbufpix[idx] = color;
                            }
                        }
                    }
                }
            }
        }
        klog("render columns\n");



        SDL_UpdateTexture(tex, NULL, bbufpix, width * sizeof(pix_t));
        SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
        SDL_RenderFillRect(ren, NULL);
        SDL_RenderCopy(ren, tex, NULL, NULL);
        SDL_RenderPresent(ren);
    }

    cleanup();
    return 0;
}

int handle_events() {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_KEYDOWN:
            if (event.key.keysym.sym == SDLK_ESCAPE) return 1;
            break;

        case SDL_WINDOWEVENT:
            if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                klog("window resized\n");
                width = event.window.data1;
                height = event.window.data2;
                klog("width: %d, height: %d\n", width, height);

                ResizeFramebuffer(width, height);
                klog("ResizeFramebuffer done\n");
                bbufpix = (pix_t*)GetBackBuffer();
                klog("GetBackBuffer done\n");
                glViewport(0, 0, width, height);
                klog("glViewport done\n");
                SDL_DestroyTexture(tex);
                klog("SDL_DestroyTexture done\n");
                tex = SDL_CreateTexture(ren, PIX_FORMAT, SDL_TEXTUREACCESS_STREAMING, width, height);
                needs_resize_skip = 0;
                klog("SDL_CreateTexture done\n");
            
                int num_cols = width / FONT_SIZE;
                if (num_cols <= 0) num_cols = 1;
                klog("width=%d, num_cols=%d\n", width, num_cols);
                if (num_cols > 10000) {
                    klog("ERROR: num_cols too large, skipping resize\n");
                    needs_resize_skip = 0;
                    break;
                }
                klog("num_cols: %d\n", num_cols);
                columns.resize(num_cols);
                klog("columns resize done\n");
                for (size_t k = 0; k < columns.size(); k++) {
                    klog("columns[%d].reset()\n", k);
                    columns[k].reset();
                    klog("columns[%d].reset() done\n", k);
                }
                klog("columns reset\n");
            }
            break;

        case SDL_QUIT:
            return 1;
        }
    }

    return 0;
}
