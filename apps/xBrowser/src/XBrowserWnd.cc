// Impl

#include "XBrowserWnd.h"
#include "XContainer.h"

// std
#include <iostream>
#include <fstream>
#include <string>
#include <stdio.h>

static std::string readFileContents(const char* filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return "";
    }
    std::string contents;
    char c;
    while (file.get(c)) {
        contents += c;
    }
    return contents;
}

XBrowserWnd::XBrowserWnd()
    : m_windowWidth(640)
    , m_windowHeight(480)
    , m_doc(nullptr)
    , m_xwin(nullptr)
    , m_g(nullptr)
{
    m_container = std::make_shared<XContainer>(&m_browser_context, this);
}

XBrowserWnd::~XBrowserWnd()
{
    m_doc.reset();
    m_container.reset();
}

void XBrowserWnd::setXWin(xwin_t* xwin)
{
    m_xwin = xwin;
}

void XBrowserWnd::repaint(graph_t* g)
{
    if (g == NULL)
        return;

    m_g = g;
    m_container->setGraph(g);
    litehtml::position pos(0, 0, m_windowWidth, m_windowHeight);

    litehtml::background_paint bg;
    bg.clip_box = pos;
    bg.color = litehtml::web_color(0xff, 0xff, 0xff);
    m_container->draw_background(0, bg);

    if (m_doc) {
        m_doc->draw((litehtml::uint_ptr)g, 0, 0, &pos);
    }
}

bool XBrowserWnd::loadMedia()
{
    return loadHtmlContent();
}

bool XBrowserWnd::loadHtmlContent()
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

    return m_doc != nullptr;
}

void XBrowserWnd::handleWindowResize(int newWidth, int newHeight)
{
    m_windowWidth = newWidth;
    m_windowHeight = newHeight;
    m_container->set_client_size(newWidth, newHeight);

    if (m_doc) {
        m_doc->render(m_windowWidth);
    }
}
