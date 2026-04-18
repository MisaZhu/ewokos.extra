// Interface for browser window

// std
#include <string>

#include <litehtml.h>

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;
class SDLContainer;


class CBrowserWnd
{
public:
    CBrowserWnd();
    ~CBrowserWnd();

    SDL_Window*   getWindow();
    SDL_Renderer* getRenderer();

    bool runEventLoop();

private:
    bool init();
    bool loadMedia();
    SDL_Texture* loadTexture(std::string path);
    void handleWindowResize(int newWidth, int newHeight);

    std::shared_ptr<SDLContainer> m_container;
    litehtml::document::ptr	      m_doc;
    litehtml::context             m_browser_context;
    
    // Window size
    int m_windowWidth;
    int m_windowHeight;
};