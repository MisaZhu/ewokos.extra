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
#define MAX_PARTICLES 5000

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// 粒子结构体 - 点阵拖尾效果
typedef struct {
    float x, y;
    float vx, vy;
    float life;
    float maxLife;
    float r, g, b;
    float brightness;
    int active;
} Particle;

// 烟花火箭
typedef struct {
    float x, y;
    float vx, vy;
    float r, g, b;
    int active;
} Firework;

// 全局变量
SDL_Window* window = NULL;
SDL_Renderer* ren = NULL;
SDL_Texture* tex = NULL;
glContext the_Context;
pix_t* bbufpix = NULL;
int screen_width, screen_height;

Particle particles[MAX_PARTICLES];
int num_particles = 0;

#define MAX_FIREWORKS 5
Firework fireworks[MAX_FIREWORKS];
int next_firework_time = 0;

// 初始化
void setup_context() {
    if (SDL_Init(SDL_INIT_VIDEO)) {
        klog("SDL_Init error: %s\n", SDL_GetError());
        exit(0);
    }

    screen_width = WIDTH;
    screen_height = HEIGHT;

    window = SDL_CreateWindow("Fireworks", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              WIDTH, HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
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

// HSV转RGB
void hsv_to_rgb(float h, float s, float v, float* r, float* g, float* b) {
    int i = (int)(h * 6);
    float f = h * 6 - i;
    float p = v * (1 - s);
    float q = v * (1 - f * s);
    float t = v * (1 - (1 - f) * s);

    switch (i % 6) {
        case 0: *r = v; *g = t; *b = p; break;
        case 1: *r = q; *g = v; *b = p; break;
        case 2: *r = p; *g = v; *b = t; break;
        case 3: *r = p; *g = q; *b = v; break;
        case 4: *r = t; *g = p; *b = v; break;
        case 5: *r = v; *g = p; *b = q; break;
    }
}

// 发射烟花 - 随机高度，支持多个同时存在
void launch_firework() {
    // 找一个空闲的烟花槽
    int slot = -1;
    for (int i = 0; i < MAX_FIREWORKS; i++) {
        if (!fireworks[i].active) {
            slot = i;
            break;
        }
    }
    if (slot < 0) return; // 没有空闲槽位
    
    Firework* fw = &fireworks[slot];
    fw->x = random_float(screen_width * 0.1f, screen_width * 0.9f);
    fw->y = screen_height;
    fw->vx = random_float(-0.5f, 0.5f);
    // 速度根据目标高度调整，越高速度越快
    fw->vy = random_float(-12.0f, -18.0f);
    
    float hue = random_float(0.0f, 1.0f);
    hsv_to_rgb(hue, 1.0f, 1.0f, &fw->r, &fw->g, &fw->b);
    
    fw->active = 1;
}

// 创建爆炸 - 范围更小更自然
void create_explosion(float x, float y, float r, float g, float b) {
    // 检查粒子数组是否已满
    if (num_particles >= MAX_PARTICLES - 500) return;
    
    int streams = 20 + rand() % 12; // 20-32条射线
    
    for (int s = 0; s < streams; s++) {
        float base_angle = (M_PI * 2 * s) / streams;
        // 添加自然随机角度
        float angle_var = random_float(-0.15f, 0.15f);
        
        // 每条射线上的粒子数减少
        int particles_per_stream = 12 + rand() % 8;
        
        for (int i = 0; i < particles_per_stream; i++) {
            if (num_particles >= MAX_PARTICLES) break;
            
            Particle* p = &particles[num_particles++];
            
            float angle = base_angle + angle_var + random_float(-0.08f, 0.08f);
            // 速度减慢，范围更紧凑
            float base_speed = 1.2f + (float)i * 0.25f;
            float speed = base_speed * random_float(0.9f, 1.1f);
            
            p->x = x + random_float(-2.0f, 2.0f); // 起始位置略有随机
            p->y = y + random_float(-2.0f, 2.0f);
            p->vx = cos(angle) * speed;
            p->vy = sin(angle) * speed;
            p->life = 1.0f;
            p->maxLife = random_float(50.0f, 90.0f);
            p->brightness = 1.0f - (float)i / particles_per_stream * 0.25f;
            
            // 颜色渐变
            float fade = 1.0f - (float)i / particles_per_stream * 0.35f;
            p->r = r * fade;
            p->g = g * fade;
            p->b = b * fade;
            
            p->active = 1;
        }
    }
    
    // 中心粒子减少
    int center_particles = 15;
    for (int i = 0; i < center_particles; i++) {
        if (num_particles >= MAX_PARTICLES) break;
        
        Particle* p = &particles[num_particles++];
        
        float angle = random_float(0, M_PI * 2);
        float speed = random_float(0.3f, 1.5f);
        
        p->x = x;
        p->y = y;
        p->vx = cos(angle) * speed;
        p->vy = sin(angle) * speed;
        p->life = 1.0f;
        p->maxLife = random_float(30.0f, 60.0f);
        p->brightness = 1.0f;
        p->r = r;
        p->g = g;
        p->b = b;
        p->active = 1;
    }
}

// 更新烟花 - 随机爆炸高度
void update_fireworks() {
    for (int i = 0; i < MAX_FIREWORKS; i++) {
        Firework* fw = &fireworks[i];
        if (!fw->active) continue;

        fw->x += fw->vx;
        fw->y += fw->vy;
        fw->vy += 0.1f;

        // 随机爆炸高度：屏幕高度的10%-50%（Y轴向下，数值越小越高）
        float explode_height = screen_height * random_float(0.1f, 0.3f);

        if (fw->vy > -1.0f || fw->y < explode_height) {
            create_explosion(fw->x, fw->y, fw->r, fw->g, fw->b);
            fw->active = 0;
        }
    }
}

// 更新粒子 - 增强重力效果
void update_particles() {
    int i = 0;
    while (i < num_particles) {
        Particle* p = &particles[i];
        if (!p->active) {
            i++;
            continue;
        }
        
        p->x += p->vx;
        p->y += p->vy;
        p->vy += 0.12f; // 增强重力
        p->life -= 1.0f / p->maxLife;
        
        // 空气阻力
        p->vx *= 0.985f;
        p->vy *= 0.985f;
        
        if (p->life <= 0) {
            p->active = 0;
            particles[i] = particles[--num_particles];
        } else {
            i++;
        }
    }
}

// 绘制像素
void set_pixel(int x, int y, uint32_t color) {
    if (x >= 0 && x < screen_width && y >= 0 && y < screen_height) {
        bbufpix[y * screen_width + x] = color;
    }
}

// 绘制带透明度的像素
void blend_pixel(int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    if (x < 0 || x >= screen_width || y < 0 || y >= screen_height) return;
    
    long idx = (long)y * screen_width + x;
    uint32_t dst = bbufpix[idx];
    
    uint8_t dstR = (dst >> 16) & 0xFF;
    uint8_t dstG = (dst >> 8) & 0xFF;
    uint8_t dstB = dst & 0xFF;
    
    uint8_t outR = (r * a + dstR * (255 - a)) / 255;
    uint8_t outG = (g * a + dstG * (255 - a)) / 255;
    uint8_t outB = (b * a + dstB * (255 - a)) / 255;
    
    bbufpix[idx] = 0xFF000000 | (outR << 16) | (outG << 8) | outB;
}

// 绘制发光点 - 更大更亮
void draw_glow_pixel(int x, int y, uint8_t r, uint8_t g, uint8_t b, float alpha) {
    if (alpha <= 0) return;
    
    // 提高亮度
    uint8_t bright_r = (uint8_t)fminf(255, r * 1.4f);
    uint8_t bright_g = (uint8_t)fminf(255, g * 1.4f);
    uint8_t bright_b = (uint8_t)fminf(255, b * 1.4f);
    
    uint8_t a = (uint8_t)(alpha * 255);
    
    // 中心点 - 更大
    blend_pixel(x, y, bright_r, bright_g, bright_b, a);
    blend_pixel(x+1, y, bright_r, bright_g, bright_b, a * 0.8f);
    blend_pixel(x-1, y, bright_r, bright_g, bright_b, a * 0.8f);
    blend_pixel(x, y+1, bright_r, bright_g, bright_b, a * 0.8f);
    blend_pixel(x, y-1, bright_r, bright_g, bright_b, a * 0.8f);
    
    // 周围发光 - 更大范围
    uint8_t glow_a = a / 2;
    blend_pixel(x+2, y, bright_r, bright_g, bright_b, glow_a);
    blend_pixel(x-2, y, bright_r, bright_g, bright_b, glow_a);
    blend_pixel(x, y+2, bright_r, bright_g, bright_b, glow_a);
    blend_pixel(x, y-2, bright_r, bright_g, bright_b, glow_a);
    blend_pixel(x+1, y+1, bright_r, bright_g, bright_b, glow_a * 0.7f);
    blend_pixel(x-1, y+1, bright_r, bright_g, bright_b, glow_a * 0.7f);
    blend_pixel(x+1, y-1, bright_r, bright_g, bright_b, glow_a * 0.7f);
    blend_pixel(x-1, y-1, bright_r, bright_g, bright_b, glow_a * 0.7f);
}

// 绘制点阵拖尾效果 - 更长的拖影
void draw_dotted_trail(float x0, float y0, float x1, float y1,
                       uint8_t r, uint8_t g, uint8_t b, float alpha) {
    float dx = x1 - x0;
    float dy = y1 - y0;
    float dist = sqrtf(dx * dx + dy * dy);
    
    if (dist < 1.0f) {
        draw_glow_pixel((int)x0, (int)y0, r, g, b, alpha);
        return;
    }
    
    // 点阵间隔 - 3-4像素一个点，颗粒更大
    float dot_spacing = 3.5f;
    int num_dots = (int)(dist / dot_spacing);
    if (num_dots < 1) num_dots = 1;
    if (num_dots > 30) num_dots = 30; // 限制最大点数
    
    for (int i = 0; i <= num_dots; i++) {
        float t = (float)i / num_dots;
        float x = x0 + dx * t;
        float y = y0 + dy * t;
        
        // 头部亮，尾部淡，但尾部更亮（拖影明显）
        float dot_alpha = alpha * (0.6f + 0.4f * (1.0f - t));
        if (dot_alpha > 0.05f) {
            draw_glow_pixel((int)x, (int)y, r, g, b, dot_alpha);
        }
    }
}

// 绘制所有粒子 - 点阵烟花效果，拖影更明显
void draw_particles() {
    if (!bbufpix) return;
    
    for (int i = 0; i < num_particles; i++) {
        Particle* p = &particles[i];
        if (!p->active) continue;
        
        // 计算拖尾 - 适中的拖影
        float speed = sqrtf(p->vx * p->vx + p->vy * p->vy);
        // 速度适中，拖尾不要过长
        float tail_len = speed * 3.5f + fabsf(p->vy) * 1.5f;
        if (tail_len > 25.0f) tail_len = 25.0f;
        if (tail_len < 5.0f) tail_len = 5.0f;
        
        float x0 = p->x;
        float y0 = p->y;
        // 拖尾方向与速度相反，但向下偏移（重力效果）
        float x1 = p->x - p->vx * tail_len * 0.4f;
        float y1 = p->y - p->vy * tail_len * 0.4f + tail_len * 0.3f;
        
        uint8_t r = (uint8_t)(p->r * 255 * p->brightness);
        uint8_t g = (uint8_t)(p->g * 255 * p->brightness);
        uint8_t b = (uint8_t)(p->b * 255 * p->brightness);
        
        // 绘制点阵拖尾
        draw_dotted_trail(x0, y0, x1, y1, r, g, b, p->life);
    }
}

// 绘制所有火箭
void draw_fireworks() {
    if (!bbufpix) return;

    for (int i = 0; i < MAX_FIREWORKS; i++) {
        Firework* fw = &fireworks[i];
        if (!fw->active) continue;

        int px = (int)fw->x;
        int py = (int)fw->y;

        uint8_t r = (uint8_t)(fw->r * 255);
        uint8_t g = (uint8_t)(fw->g * 255);
        uint8_t b = (uint8_t)(fw->b * 255);

        // 火箭头部发光
        for (int dy = -2; dy <= 2; dy++) {
            for (int dx = -2; dx <= 2; dx++) {
                int dist = dx*dx + dy*dy;
                if (dist <= 4) {
                    uint8_t alpha = 255 - dist * 40;
                    blend_pixel(px + dx, py + dy, r, g, b, alpha);
                }
            }
        }

        // 火箭点阵拖尾
        for (int j = 1; j <= 12; j++) {
            float t = j * 0.8f;
            int tx = (int)(fw->x - fw->vx * t);
            int ty = (int)(fw->y - fw->vy * t);
            uint8_t alpha = (uint8_t)(200 - j * 15);
            draw_glow_pixel(tx, ty, r, g, b, alpha / 255.0f);
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
            // 处理窗口关闭事件（xwin关闭时）
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

    // 随机种子初始化发射时间
    next_firework_time = rand() % 30 + 10;

    int frame_count = 0;

    while (1) {
        if (handle_events()) break;

        // 随机发射时机，不用等上一个结束
        if (frame_count >= next_firework_time) {
            launch_firework();
            // 下一次发射时间：10-60帧后（约0.16-1秒）
            next_firework_time = frame_count + rand() % 50 + 10;
        }

        update_fireworks();
        update_particles();

        // 清屏 - 纯黑背景
        if (bbufpix) {
            long total_pixels = (long)screen_width * screen_height;
            for (long i = 0; i < total_pixels; ++i) {
                bbufpix[i] = 0xFF000000;
            }

            draw_fireworks();
            draw_particles();

            SDL_UpdateTexture(tex, NULL, bbufpix, screen_width * sizeof(pix_t));
        }

        SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
        SDL_RenderClear(ren);
        SDL_RenderCopy(ren, tex, NULL, NULL);
        SDL_RenderPresent(ren);

        frame_count++;
        SDL_Delay(16);
    }

    cleanup();
    return 0;
}
