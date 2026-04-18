// Main entry point using widget++

#include <x++/X.h>
#include <Widget/WidgetWin.h>
#include <Widget/RootWidget.h>
#include <Widget/WidgetX.h>
#include <Widget/EditLine.h>

#include "WidgetWebview/WidgetWebview.h"

using namespace Ewok;

int main(int argc, char* argv[])
{
    X x;
    WidgetWin win;
    RootWidget* root = new RootWidget();
    win.setRoot(root);
    root->setType(Container::VERTICAL);

    EditLine* editline = new EditLine();
    root->add(editline);
    editline->fix(0, 24);
    root->focus(editline);

    WidgetWebview* webview = new WidgetWebview();
    root->add(webview);

    if(argc > 1) {
        editline->disable();
        editline->setContent(argv[1]);
        webview->loadCSS(X::getResFullName("css/default.css"));
        webview->loadHtml(argv[1]);
    }
    else {
        editline->enable();
    }

    win.open(&x, -1, -1, -1, 0, 0, "HTML Browser", XWIN_STYLE_NORMAL);
    widgetXRun(&x, &win);

    return 0;
}
