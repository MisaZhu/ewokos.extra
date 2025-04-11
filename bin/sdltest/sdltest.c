#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <SDL2/SDL_ttf.h>
#include <ewoksys/klog.h>
#include <ewoksys/keydef.h>
#include <x/x.h>
#include <stdio.h>
#include <unistd.h>

// 定时器回调函数
Uint32 timer_callback(Uint32 interval, void *param) {
    static uint32_t index = 0;
    printf("Timer triggered! Interval: %u ms, %d\n", interval, index++);
    return interval; // 返回下一次触发的时间间隔
}

int main() {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);

    if (TTF_Init() == -1) {
        printf("TTF 无法初始化! TTF_Error: %s\n", TTF_GetError());
        SDL_Quit();
        return 0;
    }

    // 加载字体
    TTF_Font* font = TTF_OpenFont("/usr/system/fonts/system-cn.ttf", 32);
    if (font == NULL) {
        printf("Failed to load font! SDL_ttf Error: %s\n", TTF_GetError());
        return 1;
    }

    // 创建定时器，每 2000 毫秒（2 秒）触发一次
    SDL_TimerID timer_id = SDL_AddTimer(2000, timer_callback, NULL);
    if (timer_id == 0) {
        printf("Failed to add timer! SDL_Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("SDL_Test", 10, 10, 640, 480, SDL_WINDOW_SHOWN);

    // 创建渲染器
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (renderer == NULL) {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // 文本内容和颜色
    const char* text = "Hello, SDL2_ttf!触发一次";
    SDL_Color textColor = {255, 0, 0, 255};

    // 创建文本表面
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, text, textColor);
    if (textSurface == NULL) {
        printf("Unable to render text surface! SDL_ttf Error: %s\n", TTF_GetError());
        return 1;
    }

    // 创建纹理
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    if (textTexture == NULL) {
        printf("Unable to create texture from rendered text! SDL Error: %s\n", SDL_GetError());
        return 1;
    }
    // 释放表面
    SDL_FreeSurface(textSurface);

    // 文本矩形
    SDL_Rect textRect;
    textRect.x = 100;
    textRect.y = 40;
    textRect.w = textSurface->w;
    textRect.h = textSurface->h;

    // 渲染文本
    SDL_RenderCopy(renderer, textTexture, NULL, &textRect);

    IMG_Init(IMG_INIT_PNG);
    SDL_Surface* img = IMG_Load("/usr/system/images/logos/ewok.png");
    SDL_Texture* texture = NULL;
    SDL_Rect imgRect = { 0, 0, 0, 0 };
    if(img != NULL) {
        imgRect.w = img->w;
        imgRect.h = img->h;
        texture = SDL_CreateTextureFromSurface(renderer, img);
        SDL_FreeSurface(img);
    }

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, &imgRect);
    SDL_RenderPresent(renderer);
    
    Mix_Init(MIX_INIT_OGG);
    Mix_Music* mix = Mix_LoadMUS("/data/test/test.ogg");
    if(mix != NULL)
       Mix_FreeMusic(mix);

    int quit = 0;
    while (!quit) {
        bool update = false;
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    quit = 1;
                    break;
                case SDL_KEYDOWN:
                    if (event.key.keysym.sym == KEY_UP) {
                        imgRect.y -= 10;
                        update = true;
                    }
                    else if (event.key.keysym.sym == KEY_DOWN) {
                        imgRect.y += 10;
                        update = true;
                    }
                    else if (event.key.keysym.sym == KEY_LEFT) {
                        imgRect.x -= 10;
                        update = true;
                    }
                    else if (event.key.keysym.sym == KEY_RIGHT) {
                        imgRect.x += 10;
                        update = true;
                    }
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    imgRect.x = event.button.x - imgRect.w/2;
                    imgRect.y = event.button.y - imgRect.h/2;
                    update = true;
                    break;
            }

            if(update) {
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                SDL_RenderClear(renderer);
                SDL_RenderCopy(renderer, texture, NULL, &imgRect);
                roundedBoxColor(renderer, 100, 100, 200, 200, 10, 0x8800FF00);
                SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
                SDL_RenderPresent(renderer);
                update = false;
            }
        }
        usleep(30000);
    }

    if(texture != NULL)
       SDL_DestroyTexture(texture);

    if(textTexture != NULL)
        SDL_DestroyTexture(textTexture);

    if(font != NULL)
        TTF_CloseFont(font);

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
