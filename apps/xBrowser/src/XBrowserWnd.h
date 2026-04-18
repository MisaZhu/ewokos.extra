// Interface for browser window using xwin

#pragma once

#include <litehtml.h>
#include <x/xwin.h>
#include <memory>

class XContainer;

class XBrowserWnd {
public:
    XBrowserWnd();
    ~XBrowserWnd();

    void setXWin(xwin_t* xwin);
    void repaint(graph_t* g);
    void handleWindowResize(int newWidth, int newHeight);
    bool loadMedia();

private:
    bool loadHtmlContent();

    std::shared_ptr<XContainer> m_container;
    litehtml::document::ptr       m_doc;
    litehtml::context            m_browser_context;

    int m_windowWidth;
    int m_windowHeight;
    xwin_t* m_xwin;
    graph_t* m_g;
};