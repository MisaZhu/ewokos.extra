// Main entry point using xwin C API

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ewoksys/proc.h>
#include <x/x.h>
#include <x/xwin.h>
#include <graph/graph.h>
#include "XBrowserWnd.h"

static XBrowserWnd* g_browser = NULL;

static void on_repaint(xwin_t* xwin, graph_t* g)
{
    (void)xwin;
    if (g_browser && g) {
        g_browser->repaint(g);
    }
}

static void on_resize(xwin_t* xwin)
{
    (void)xwin;
    if (g_browser && xwin && xwin->xinfo) {
        g_browser->handleWindowResize(xwin->xinfo->winr.w, xwin->xinfo->winr.h);
    }
}

static void loop(void* p)
{
    xwin_t* xwin = (xwin_t*)p;
    xwin_repaint(xwin);
    proc_usleep(16000);
}

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    x_t x;
    x_init(&x, NULL);

    xwin_t* xwin = xwin_open(&x, -1, 32, 32, 640, 480, "HTML Browser", XWIN_STYLE_NORMAL);
    if (!xwin) {
        printf("Failed to open window\n");
        return 1;
    }

    g_browser = new XBrowserWnd();
    g_browser->setXWin(xwin);
    g_browser->loadMedia();

    xwin->on_repaint = on_repaint;
    xwin->on_resize = on_resize;
    x.on_loop = loop;

    xwin_set_visible(xwin, true);

    x_run(&x, xwin);

    delete g_browser;
    return 0;
}
