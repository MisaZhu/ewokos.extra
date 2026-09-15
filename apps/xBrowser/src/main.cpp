// Main entry point using widget++

#include <x++/X.h>
#include <Widget/WidgetWin.h>
#include <Widget/RootWidget.h>
#include <Widget/WidgetX.h>
#include <Widget/EditLine.h>
#include <Widget/Label.h>
#include <Widget/LabelButton.h>
#include <Widget/Scroller.h>
#include <Widget/Container.h>
#include <ewoksys/keydef.h>
#include <vector>
#include <string>

#include "WidgetWebview/WidgetWebview.h"

using namespace Ewok;
class StatusBar: public Label {
protected:
	void onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
		graph_fill_3d(g, r.x, r.y, r.w, r.h, theme->basic.docBGColor, true);
		font_t* font = theme->getFont();
		int y = r.y + (r.h-font_get_height(font, theme->basic.fontSize))/2;
		graph_draw_text_font(g, r.x+4, y, label.c_str(), font, theme->basic.fontSize, theme->basic.docFGColor);
	}
public:
	StatusBar(const std::string& label) : Label(label) {}
};

class BrowserWidget: public WidgetWebview {
public:
    BrowserWidget() : WidgetWebview() {
        statusBar = nullptr;
        addressBar = nullptr;
        m_statusDirty = false;
        m_initialLoadStarted = false;
        m_historyNav = false;
        pthread_mutex_init(&m_statusMutex, NULL);
    }

    ~BrowserWidget() {
        pthread_mutex_destroy(&m_statusMutex);
    }

    void setStatusBar(StatusBar* statusBar) {
        this->statusBar = statusBar;
    }

    void setAddressBar(EditLine* addressBar) {
        this->addressBar = addressBar;
    }

    void setInitialUrl(const std::string& url) {
        m_initialUrl = url;
    }

    /* Toolbar actions. All run on the UI thread (button click / EditLine
     * enter), same as loadHtml itself. */
    void goBack() {
        if(m_history.size() < 2)
            return;
        m_history.pop_back();          /* current page */
        m_historyNav = true;           /* the pop above IS the bookkeeping */
        loadHtml(m_history.back());
    }

    void reload() {
        std::string url = getCurrentUrl();
        if(url.empty())
            url = m_initialUrl;
        if(url.empty())
            return;
        m_historyNav = true;           /* same page: do not push a duplicate */
        loadHtml(url);
    }

    void stop() {
        stopLoading();
        queueStatus("stopped");
    }
protected:
    StatusBar* statusBar;
    EditLine* addressBar;
    std::string m_pendingStatus;
    std::string m_initialUrl;
    pthread_mutex_t m_statusMutex;
    bool m_statusDirty;
    bool m_initialLoadStarted;
    /* Session history: back stack of visited page URLs, top = current page.
     * The webview itself owns no history (see onHtmlUrlChanged in the lib);
     * UI-thread only, so no locking. */
    std::vector<std::string> m_history;
    bool m_historyNav;
    int m_loadRetries = 0;
    int m_retryTicks = 0;
    std::string m_retryUrl;

    void onHtmlUrlChanged(const std::string& url) override {
        /* Fires on the UI thread for every accepted page load, including
         * in-page <a href>/location.href navigations, so link clicks land in
         * the back stack and the address bar follows the real location. */
        if(m_historyNav)
            m_historyNav = false;
        else if(m_history.empty() || m_history.back() != url)
            m_history.push_back(url);
        if(addressBar != nullptr && addressBar->getContent() != url)
            addressBar->setContent(url);
    }

    void queueStatus(const std::string& msg) {
        pthread_mutex_lock(&m_statusMutex);
        m_pendingStatus = msg;
        m_statusDirty = true;
        pthread_mutex_unlock(&m_statusMutex);
    }

    void onTaskStart(const HttpTask& task) override {
        queueStatus("loading " + task.url);
    }
    void onTaskEnd(const HttpTask& task) override {
        queueStatus(task.url + " loaded");
    }
    void onTaskFailed(const HttpTask& task) override {
        /* Boot race: the autostart load can fire before netd's DHCP lease
         * lands, and DNS then fails instantly. Retry a few times on a tick
         * countdown instead of leaving a blank page forever. */
        if(task.type == HttpTask::TASK_HTML && m_loadRetries < 6) {
            m_loadRetries++;
            m_retryUrl = task.url;
            m_retryTicks = 45; /* ~1.5s at the 30Hz window timer */
            queueStatus("retry " + std::to_string(m_loadRetries));
            return;
        }
        queueStatus(task.url + " failed");
    }

    void onTasksEnd() override {
        queueStatus("");
    }

    void onBuildStatus(const std::string& status, int progress) override {
        if(status.empty()) {
            queueStatus("");
            return;
        }
        queueStatus(status + " (" + std::to_string(progress) + "%)");
    }

    void onTimer(uint32_t timerFPS, uint32_t timerSteps) override {
        WidgetWebview::onTimer(timerFPS, timerSteps);
        (void)timerFPS;
        (void)timerSteps;

        if(!m_initialLoadStarted && !m_initialUrl.empty()) {
            m_initialLoadStarted = true;
            loadHtml(m_initialUrl);
        }

        if(m_retryTicks > 0 && --m_retryTicks == 0 && !m_retryUrl.empty()) {
            std::string url = m_retryUrl;
            m_retryUrl.clear();
            loadHtml(url);
        }

        if(statusBar == nullptr)
            return;

        std::string msg;
        bool dirty = false;
        pthread_mutex_lock(&m_statusMutex);
        if(m_statusDirty) {
            msg = m_pendingStatus;
            m_statusDirty = false;
            dirty = true;
        }
        pthread_mutex_unlock(&m_statusMutex);

        if(dirty) {
            statusBar->setLabel(msg);
        }
    }
};

static void onInputFunc(Widget* wd, uint32_t key, void* arg) {
    WidgetWebview* webview = (WidgetWebview*)arg;
    EditLine* editline = (EditLine*)wd;
    if(webview == NULL || editline->getContent().empty())
        return;
    if(key != KEY_ENTER)
        return;
    std::string full_url = editline->getContent();
    if(full_url.find("://") == std::string::npos) {
        full_url = "https://" + editline->getContent();
        editline->setContent(full_url);
    }

    webview->loadHtml(full_url);
}

/* Toolbar buttons share Widget's generic event func; only the CLICK edge
 * (fires on mouse-up inside the button) triggers the action. */
static void onBackClick(Widget* wd, xevent_t* evt, void* arg) {
    (void)wd;
    if(evt->type != XEVT_MOUSE || evt->state != MOUSE_STATE_CLICK)
        return;
    ((BrowserWidget*)arg)->goBack();
}

static void onStopClick(Widget* wd, xevent_t* evt, void* arg) {
    (void)wd;
    if(evt->type != XEVT_MOUSE || evt->state != MOUSE_STATE_CLICK)
        return;
    ((BrowserWidget*)arg)->stop();
}

static void onReloadClick(Widget* wd, xevent_t* evt, void* arg) {
    (void)wd;
    if(evt->type != XEVT_MOUSE || evt->state != MOUSE_STATE_CLICK)
        return;
    ((BrowserWidget*)arg)->reload();
}

int main(int argc, char* argv[])
{
    X x;
    WidgetWin win;
    RootWidget* root = new RootWidget();
    win.setRoot(root);
    root->setType(Container::VERTICAL);

    /* Toolbar: [ address bar ................ ][back][stop][reload] */
    Container* toolbar = new Container();
    toolbar->setType(Container::HORIZONTAL);
    root->add(toolbar);
    toolbar->fix(0, 24);

    EditLine* editline = new EditLine();
    toolbar->add(editline);
    root->focus(editline);

    LabelButton* backBtn = new LabelButton("<");
    backBtn->fix(26, 0);
    toolbar->add(backBtn);

    LabelButton* stopBtn = new LabelButton("X");
    stopBtn->fix(26, 0);
    toolbar->add(stopBtn);

    LabelButton* reloadBtn = new LabelButton("R");
    reloadBtn->fix(26, 0);
    toolbar->add(reloadBtn);

    // Create a horizontal container for webview and vertical scroller
    Container* c = new Container();
    c->setType(Container::HORIZONTAL);
    root->add(c);

    BrowserWidget* webview = new BrowserWidget();
    c->add(webview);

    editline->setOnInputFunc(onInputFunc, webview);
    webview->setAddressBar(editline);
    backBtn->setEventFunc(onBackClick, webview);
    stopBtn->setEventFunc(onStopClick, webview);
    reloadBtn->setEventFunc(onReloadClick, webview);

    // Add vertical scroller
    Scroller* sr = new Scroller();
    sr->fix(8, 0);
    webview->setScrollerV(sr);
    c->add(sr);

    win.open(&x, -1, -1, -1, 0, 0, "HTML Browser", XWIN_STYLE_NORMAL);
    /* Faster widget loop: the webview's page cache makes routine repaints a
     * cheap blit (full litehtml draws happen only when content changes), so a
     * 30Hz timer - which also halves the loop's sleep quantum - keeps drag
     * and wheel scrolling smooth without burning CPU on idle. */
    win.setTimer(30);

    webview->setDefaultCSS("res://html/default.css");
    std::string initialUrl;
    if(argc > 1) {
        editline->disable();
        std::string argUrl = argv[1];
        if(!argUrl.empty() && argUrl[0] == '/') //local file path, e.g. /data/test.html
            argUrl = "file:/" + argUrl;
        editline->setContent(argUrl);
        initialUrl = argUrl;
    }
    else {
        editline->enable();
        initialUrl = "res://html/default.html";
    }

    StatusBar* statusBar = new StatusBar("");
    webview->setStatusBar(statusBar);
    webview->setInitialUrl(initialUrl);
    if(argc > 2 && std::string(argv[2]) == "nojs")
        webview->setJSEnabled(false);
    root->add(statusBar);

    statusBar->fix(0, 16);

    widgetXRun(&x, &win);
    return 0;
}
