// Impl

#include "CBrowserWnd.h"
#include "SDLContainer.h"

// std
#include <iostream>
#include <fstream>
#include <string>
#include <stdio.h>

// SDL
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>

// EwokOS uses UTF-8 by default, no need for WideChar conversion
#if defined(__EWOKOS__)
// litehtml on non-Windows platforms uses char/tchar_t directly
#endif

//Screen dimension constants
const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

//The window we'll be rendering to
SDL_Window* gWindow = NULL;

//The window renderer
SDL_Renderer* gRenderer = NULL;

//Current displayed texture
SDL_Texture* gTexture = NULL;

CBrowserWnd::CBrowserWnd()
    : m_windowWidth(SCREEN_WIDTH)
    , m_windowHeight(SCREEN_HEIGHT)
{
	// Create window
	if (!init()) {
		// Failed to initialize
	}
	else {
		// Load media
		if (!loadMedia()) {
			// Failed to load media
		}
	}

    m_container = std::make_shared<SDLContainer>(&m_browser_context, gRenderer);
}


CBrowserWnd::~CBrowserWnd()
{
    m_doc.reset();
    m_container.reset();
    SDL_DestroyTexture(gTexture);
    gTexture = NULL;
    SDL_DestroyRenderer(gRenderer);
    SDL_DestroyWindow(gWindow);
    gWindow = NULL;
    gRenderer = NULL;
    TTF_Quit();
    IMG_Quit();
}


SDL_Window* CBrowserWnd::getWindow()
{
	return gWindow;
}


SDL_Renderer* CBrowserWnd::getRenderer()
{
	return gRenderer;
}


// Helper function to read file contents
static std::string readFileContents(const char* filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return "";
    }
    // Use a simple loop to read file contents
    std::string contents;
    char c;
    while (file.get(c)) {
        contents += c;
    }
    return contents;
}

void CBrowserWnd::handleWindowResize(int newWidth, int newHeight)
{
    m_windowWidth = newWidth;
    m_windowHeight = newHeight;
    m_container->set_client_size(newWidth, newHeight);

    if (m_doc) {
        m_doc->render(m_windowWidth);
    }
}

bool CBrowserWnd::runEventLoop()
{
    {
        std::string strContents = readFileContents("/data/html/master.css");
        if (!strContents.empty()) {
            m_browser_context.load_master_stylesheet(strContents.c_str());
        }
    }

    {
        std::string strContents = readFileContents("/data/html/test.html");
        if (!strContents.empty()) {
            m_doc = litehtml::document::createFromString(strContents.c_str(), m_container.get(), &m_browser_context);
            if (m_doc) {
                m_doc->render(m_windowWidth);
            }
        }
    }

    bool quit = false;
    SDL_Event e;

	while (!quit) {
		while (SDL_PollEvent(&e) != 0) {
			switch (e.type) {
			case SDL_QUIT:
				quit = true;
				break;
				
			case SDL_WINDOWEVENT:
				if (e.window.event == SDL_WINDOWEVENT_RESIZED) {
					handleWindowResize(e.window.data1, e.window.data2);
				}
				break;
			}
		}

		SDL_RenderClear(gRenderer);

        if (m_doc) {
            const litehtml::position pos(0, 0, m_windowWidth, m_windowHeight);
            m_doc->draw(0, 0, 0, &pos);
        }

		SDL_RenderPresent(gRenderer);
		SDL_Delay(20);
	}

	return true;
}


bool CBrowserWnd::init()
{
	// Initialization flag
	bool success = true;

	// Set texture filtering to linear
	if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1")) {
		// Warning: Linear texture filtering not enabled
	}

	// Create window
	gWindow = SDL_CreateWindow("SDL Tutorial", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
			SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
	if (gWindow == NULL) {
		success = false;
	}
	else {
		// Create renderer for window
		gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_ACCELERATED);
		if (gRenderer == NULL)
		{
			success = false;
		}
		else
		{
			//Initialize renderer color
			SDL_SetRenderDrawColor(gRenderer, 0xFF, 0xFF, 0xFF, 0xFF);

			//Initialize PNG loading
			int imgFlags = IMG_INIT_PNG;
			if (!(IMG_Init(imgFlags) & imgFlags))
			{
				success = false;
			}
		}
	}

	return success;
}

bool CBrowserWnd::loadMedia()
{
	//Loading success flag
	bool success = true;

	//Load PNG texture
	gTexture = loadTexture("/data/html/texture.png");
	if (gTexture == NULL)
	{
		success = false;
	}

	return success;
}


SDL_Texture* CBrowserWnd::loadTexture(std::string path)
{
	//The final texture
	SDL_Texture* newTexture = NULL;

	//Load image at specified path
	SDL_Surface* loadedSurface = IMG_Load(path.c_str());
	if (loadedSurface == NULL) {
		std::cout << "Unable to load image %s! SDL_image Error: " << path.c_str() << ", " << IMG_GetError();
	}
	else
	{
		//Create texture from surface pixels
		newTexture = SDL_CreateTextureFromSurface(gRenderer, loadedSurface);

		//Get rid of old loaded surface
		SDL_FreeSurface(loadedSurface);
	}

	return newTexture;
}
