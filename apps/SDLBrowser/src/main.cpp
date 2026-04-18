// Main

// std
#include <stdio.h>
#include <string>
#include <memory>

// SDL
#include <SDL.h>

// Browser
#include "CBrowserWnd.h"

int main(int argc, char* args[])
{
    //Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        // SDL could not initialize
    }
    else {
        std::shared_ptr<CBrowserWnd> pBrowserWnd = std::make_shared<CBrowserWnd>();
        pBrowserWnd->runEventLoop();
    }

    //Quit SDL subsystems
    SDL_Quit();

    return 0;
}
