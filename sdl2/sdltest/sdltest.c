#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
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

    /*
    IMG_Init(IMG_INIT_PNG);
    SDL_Surface* img = IMG_Load("/usr/system/images/logos/apple.png");
    if(img != NULL)
       SDL_FreeSurface(img);

    Mix_Init(MIX_INIT_MP3);
    Mix_Music* mix = Mix_LoadMUS("/data/test/test.mp3");
    klog("mix: %x\n", mix);
    if(mix != NULL)
       Mix_FreeMusic(mix);
       */

    SDL_Rect rect = { 10, 10, 100, 100 };
    int quit = 0;
    while (!quit) {
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderFillRect(renderer, &rect);

        SDL_RenderPresent(renderer);

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    quit = 1;
                    break;
                case SDL_KEYDOWN:
                    if (event.key.keysym.sym == SDLK_ESCAPE) {
                        quit = 1;
                    }
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    klog("down, x: %d, y: %d\n", event.button.x, event.button.y);
                    rect.x = event.button.x;
                    rect.y = event.button.y;
                    break;
            }
        }
        usleep(30000);
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
