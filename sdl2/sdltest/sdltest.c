#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <ewoksys/klog.h>
#include <ewoksys/keydef.h>
#include <x/x.h>
#include <stdio.h>
#include <unistd.h>

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
    /*
    Mix_Init(MIX_INIT_MP3);
    Mix_Music* mix = Mix_LoadMUS("/data/test/test.mp3");
    klog("mix: %x\n", mix);
    if(mix != NULL)
       Mix_FreeMusic(mix);
       */

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
                SDL_RenderPresent(renderer);
                update = false;
            }
        }
        usleep(30000);
    }

    if(texture != NULL)
       SDL_DestroyTexture(texture);

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
