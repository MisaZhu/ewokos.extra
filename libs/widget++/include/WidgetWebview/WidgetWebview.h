#pragma once

/* WidgetWebview - a widget++ window over an embedded ewebview engine.
 *
 * All web machinery (litehtml documents, the mario JS VM, networking, the
 * render pipeline and its engine/download threads) lives in the platform-
 * independent ewebview library (browser/ewebview), driven through its pure-C
 * API <ewebview.h>. This widget is only the EwokOS xwin front-end:
 *
 *   - it creates the engine with the reference EwokOS port (eweb_port_ewokos:
 *     graph_t surfaces, font_t text, tinyhttpsc/vfs net, kernel_tic clock);
 *   - onTimer() pumps ewebview_tick(), which delivers every engine event
 *     (frames, scroll clamps, url/status/title/dialog, task lifecycle) as a
 *     listener callback on the UI thread;
 *   - onRepaint() blits the latest adopted frame - shifted by the live scroll
 *     offset - into the window graph_t (a frame IS a graph_t* under the EwokOS
 *     port, so this is a zero-copy handoff);
 *   - onMouse()/onScroll()/onResize() translate xwin input into
 *     ewebview_post_event() / ewebview_scroll() / ewebview_set_viewport().
 *
 * Threading: exactly as documented in <ewebview.h> - the engine thread owns
 * the documents/VM/bitmaps, download workers fetch subresources, and the UI
 * thread (this widget) never blocks on a fetch/parse; every call into the
 * engine just queues a command.
 */

#include <Widget/Scrollable.h>
#include <graph/graph.h>
#include <stdint.h>
#include <string>

/* Opaque ewebview handles (definitions live in <ewebview.h>/<ewebview_port.h>,
 * included by the .cc; forward-declared here to keep this public header - which
 * is installed into the SDK include dir - light). */
struct ewebview;
struct eweb_surface;

/* Sub-resource task description handed to the onTask* hooks below (mirrors the
 * EWEB_TASK_* kinds of the ewebview listener). */
struct HttpTask {
    static const int TASK_HTML   = 0;
    static const int TASK_CSS    = 1;
    static const int TASK_IMAGE  = 2;
    std::string url;
    int type;
    bool loading;
};

namespace Ewok {

class WidgetWebview : public Scrollable {
public:
    WidgetWebview();
    virtual ~WidgetWebview();

    /* Begin loading `url`. UI-THREAD entry point (toolbar, address bar, initial
     * load): queues a navigation command with the engine and returns
     * immediately, so the window never blocks on a fetch/parse. The engine
     * aborts + cleans up any in-flight page and starts the new load in its own
     * context. */
    bool loadHtml(const std::string& url);
    /* Stop button (UI thread). The engine aborts the in-flight build/fetches
     * and keeps whatever is already on screen. */
    void stopLoading();
    /* Master stylesheet URL (e.g. "res://html/default.css"), loaded before the
     * first page. URL resolution (res://, file://, https://) is the engine's
     * job now - pass the string through as-is. */
    void setDefaultCSS(const std::string& url);

    /* Toggle JavaScript execution. Enabled by default; disabling makes the
     * engine free the VM and drop <script> bodies on the next load. */
    void setJSEnabled(bool enabled);
    bool isJSEnabled() const { return m_jsEnabled; }

    /* URL of the page currently on screen (empty before the first load).
     * UI-thread mirror refreshed by the on_url listener callback. */
    const std::string& getCurrentUrl() const { return m_uiCurrentUrl; }

protected:
    virtual void onRepaint(graph_t* g, XTheme* theme, const grect_t& r) override;
    virtual void onResize() override;
    virtual void setAttr(const string& attr, json_var_t*value) override;
    virtual void onTimer(uint32_t timerFPS, uint32_t timerSteps) override;

    virtual bool onScroll(int step, bool horizontal) override;
    virtual void updateScroller() override;
    virtual bool onMouse(xevent_t* ev) override;

    /* App hooks, fired on the UI thread from onTimer's ewebview_tick() drain.
     * The embedding app owns its widgets and the session history, so these are
     * the only places it should touch them in reaction to page events. */
    virtual void onTaskStart(const HttpTask& task) {}
    virtual void onTaskEnd(const HttpTask& task) {}
    virtual void onTaskFailed(const HttpTask& task) {}
    virtual void onTasksEnd() {}
    virtual void onBuildStatus(const std::string& status, int progress) {}
    /* The visible page's URL just changed: fired for BOTH address-bar loads and
     * in-page navigations (<a href>, location.href). Record history here. */
    virtual void onHtmlUrlChanged(const std::string& url) { (void)url; }

private:
    /* ewebview listener trampolines. All fire on the UI thread inside
     * ewebview_tick() (see the threading contract in <ewebview.h>). */
    static void cbFrame(void* ud, struct eweb_surface* frame,
                        int frameScrollX, int frameScrollY, int docW, int docH);
    static void cbScroll(void* ud, int x, int y, int docW, int docH);
    static void cbUrl(void* ud, const char* url);
    static void cbStatus(void* ud, const char* text, int progress);
    static void cbBuildStatus(void* ud, const char* text, int progress, bool overlay);
    static void cbDialog(void* ud, const char* text);
    static void cbTaskStart(void* ud, const char* url, int type);
    static void cbTaskEnd(void* ud, const char* url, int type);
    static void cbTaskFailed(void* ud, const char* url, int type);
    static void cbTasksEnd(void* ud);

    /* Take ownership of an engine-rendered frame as the new m_displayCache,
     * handing the previous one back to the engine's frame pool. Records the
     * offset/geometry it was rendered at so onRepaint can blit it aligned to
     * the live scroll. UI-thread only (called from cbFrame). */
    void adoptFrame(struct eweb_surface* buf, int renderX, int renderY, int docW, int docH);
    /* Wheel/drag scroll: clamp the new offset to the last-known geometry, move
     * the live UI offset so the next repaint shifts the cached frame
     * immediately, then ewebview_scroll() so the engine re-renders the exposed
     * strip and fires the page's scroll handlers. UI-thread only. */
    void uiLocalScroll(int newX, int newY);

    struct ewebview* m_view;
    bool        m_jsEnabled;        // UI-side mirror (the C API has no getter)

    /* HiDPI: device pixels per logical (CSS) pixel handed to the EwokOS port
     * (eweb_port_ewokos_set_dpr). The engine lays out / reports scroll + doc
     * geometry in LOGICAL px and rasterises each frame at device resolution;
     * this widget reports a logical viewport (area / m_dpr), blits the device-
     * sized frame 1:1, and converts mouse/scroll between the two. Defaults to
     * 1.0 (plain 1x, no scaling) unless XBROWSER_DPR overrides it. */
    float       m_dpr;

    /* UI front buffer: the last frame adopted from the engine, an opaque
     * eweb_surface handle (its device-pixel graph_t* is recovered via
     * eweb_port_ewokos_surface_native for the zero-copy blit in onRepaint).
     * m_docW/m_docH is the last document geometry the engine reported (LOGICAL
     * px), used to clamp UI-local scrolling and size the scrollbar. */
    struct eweb_surface* m_displayCache;
    int         m_frameScrollX;
    int         m_frameScrollY;
    int         m_docW;
    int         m_docH;
    int         m_scrollX;
    int         m_scrollY;

    std::string m_uiCurrentUrl;     // getCurrentUrl() mirror (on_url)
    std::string m_buildStatus;      // build-overlay text (on_build_status)
    int         m_buildProgress;
    bool        m_uiBuildOverlay;   // veil the page while a real build runs

    // Throttles the once-per-second tick/heap watchdog log in onTimer.
    uint64_t    m_lastStatLogAt;
};

}
