#include <SDL.h>
#include <SDL_image.h>
#include <ewoksys/klog.h>
#include <x/x.h>

int main() {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *window = SDL_CreateWindow("Test", 10, 10, 640, 480, SDL_WINDOW_SHOWN);

    // 创建渲染器
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (renderer == NULL) {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // 定义矩形
    SDL_Rect rect = { 10, 10, 100, 100 };

    // 设置渲染器的绘制颜色（这里设置为红色）
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);

    // 绘制矩形
    SDL_RenderFillRect(renderer, &rect);

    // 呈现渲染器的内容到窗口
    SDL_RenderPresent(renderer);


    x_t* x = (x_t*)SDL_GetVideoDriverData();
    while(!x->terminated) {
        sleep(1);
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
