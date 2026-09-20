/* WidgetWebview - the EwokOS xwin front-end for an embedded ewebview engine.
 *
 * Everything web - litehtml, the mario VM, networking, rasterization, the
 * engine and download threads - lives in libewebview (browser/ewebview). This
 * file is only the glue between that engine's pure-C API and the widget++
 * Widget/Scrollable surface:
 *
 *   ctor/dtor   ewebview_create() with the reference EwokOS port / release the
 *               adopted frame + ewebview_destroy()
 *   onTimer     ewebview_tick(): drains the engine->UI events (listener cbs)
 *   onRepaint   blit the adopted frame (a graph_t*, zero-copy under the EwokOS
 *               port) shifted by the live scroll offset + the build overlay
 *   onResize    ewebview_set_viewport()
 *   onMouse     translate the xwin mouse event into eweb_event_t +
 *               ewebview_post_event(); drag/wheel scroll stay UI-local
 *   onScroll    uiLocalScroll(): clamp, move the live offset, ewebview_scroll()
 *
 * Threading: per <ewebview.h>, every listener callback fires on the UI thread
 * inside ewebview_tick(), so the virtual onTask handlers, onBuildStatus and
 * onHtmlUrlChanged hooks (which touch the embedding app's widgets) all run
 * here. No locks are taken anywhere in this file: the engine never calls the UI thread from its
 * own threads, and the UI thread only queues commands into the engine.
 */

#include "WidgetWebview/WidgetWebview.h"

#include <ewebview.h>          /* the ewebview C API + eweb_listener_t        */
#include <ewebview_port.h>     /* eweb_port_ewokos(), eweb_event_t            */

#include <font/font.h>         /* graph_draw_text_font: the build overlay     */
#include <ewoksys/kernel_tic.h>
#include <ewoksys/klog.h>

#include <stdlib.h>
#include <string.h>
#include <math.h>             /* lroundf/ceilf: logical<->device pixel scaling */

using namespace Ewok;

/* libgloss heap diagnostics (see compat.c). Walks the whole block chain under
 * the malloc lock, so it must never run on a timed path: it is only called
 * from the throttled watchdog in onTimer, and only with XBROWSER_HEAPSTAT=1. */
extern "C" void ewok_heap_stat(uint32_t* blocks, uint32_t* free_blocks,
        uint32_t* used_bytes, uint32_t* free_bytes,
        uint32_t* free_list_len, uint32_t* free_list_max);

static bool heap_stat_enabled() {
    static int enabled = -1;
    if(enabled < 0) {
        const char* v = getenv("XBROWSER_HEAPSTAT");
        enabled = (v != nullptr && v[0] == '1') ? 1 : 0;
    }
    return enabled == 1;
}

/* The EwokOS port wraps each frame in an opaque handle around a device-pixel
 * graph_t (port_ewokos.c). Recover the graph_t* for the zero-copy present via
 * the port's accessor rather than a raw cast. */
static inline graph_t* frameGraph(eweb_surface_t* s) {
    return (graph_t*)eweb_port_ewokos_surface_native(s);
}

/* ==================================================================
 * Lifecycle
 * ================================================================== */

WidgetWebview::WidgetWebview()
    : m_view(nullptr)
    , m_jsEnabled(true)
    , m_dpr(1.0f)
    , m_displayCache(nullptr)
    , m_frameScrollX(0)
    , m_frameScrollY(0)
    , m_docW(0)
    , m_docH(0)
    , m_scrollX(0)
    , m_scrollY(0)
    , m_buildProgress(0)
    , m_uiBuildOverlay(false)
    , m_lastStatLogAt(0)
{
    /* HiDPI mechanism, off by default on EwokOS: lay out in logical (CSS) px
     * and rasterise at device resolution. dpr defaults to 1.0 (plain 1x, the
     * port does no scaling); set XBROWSER_DPR (e.g. 2) to opt into HiDPI on a
     * higher-density panel, matching the SDL reference port. */
    const char* dprEnv = getenv("XBROWSER_DPR");
    if(dprEnv != nullptr && dprEnv[0] != '\0') {
        float v = (float)atof(dprEnv);
        if(v >= 1.0f && v <= 4.0f)
            m_dpr = v;
    }

    eweb_port_t port;
    eweb_port_ewokos(&port, nullptr);
    /* Must precede ewebview_create()/set_viewport so the frame pool allocates
     * device-sized (logical*dpr) buffers. */
    eweb_port_ewokos_set_dpr(m_dpr);
    m_view = ewebview_create(&port);
    if(m_view == nullptr) {
        klog("[xBrowser] ewebview_create failed\n");
        return;
    }

    eweb_listener_t lis;
    eweb_listener_init(&lis);
    lis.ud              = this;
    lis.on_frame        = cbFrame;
    lis.on_scroll       = cbScroll;
    lis.on_url          = cbUrl;
    lis.on_status       = cbStatus;
    lis.on_build_status = cbBuildStatus;
    lis.on_dialog       = cbDialog;
    lis.on_task_start   = cbTaskStart;
    lis.on_task_end     = cbTaskEnd;
    lis.on_task_failed  = cbTaskFailed;
    lis.on_tasks_end    = cbTasksEnd;
    ewebview_set_listener(m_view, &lis);
}

WidgetWebview::~WidgetWebview()
{
    /* "程序退出" termination path. The engine owns the documents/VM/frame pool,
     * so ewebview_destroy() stops the threads and tears everything down in the
     * engine's own context. The adopted front buffer is a pooled frame, so hand
     * it back FIRST - it becomes invalid the moment the pool is freed. */
    if(m_view != nullptr) {
        if(m_displayCache != nullptr) {
            ewebview_release_frame(m_view, m_displayCache);
            m_displayCache = nullptr;
        }
        ewebview_destroy(m_view);
        m_view = nullptr;
    }
}

/* ==================================================================
 * Public UI-thread API -> engine commands
 * ================================================================== */

bool WidgetWebview::loadHtml(const std::string& url)
{
    /* Queues a navigation and returns immediately; the engine aborts + cleans
     * up any in-flight page and starts the new load in its own context. */
    if(m_view == nullptr || url.empty())
        return false;
    klog("[xBrowser] queue html: %s\n", url.c_str());
    ewebview_load(m_view, url.c_str());
    return true;
}

void WidgetWebview::stopLoading()
{
    if(m_view != nullptr)
        ewebview_stop(m_view);
}

void WidgetWebview::setDefaultCSS(const std::string& url)
{
    if(m_view != nullptr)
        ewebview_set_default_css(m_view, url.c_str());
}

void WidgetWebview::setJSEnabled(bool enabled)
{
    m_jsEnabled = enabled;
    if(m_view != nullptr)
        ewebview_set_js_enabled(m_view, enabled);
}

/* ==================================================================
 * ewebview listener trampolines (all on the UI thread, from ewebview_tick)
 * ================================================================== */

void WidgetWebview::cbFrame(void* ud, struct eweb_surface* frame,
                            int frameScrollX, int frameScrollY, int docW, int docH)
{
    WidgetWebview* self = (WidgetWebview*)ud;
    if(self != nullptr)
        self->adoptFrame((eweb_surface_t*)frame,
                         frameScrollX, frameScrollY, docW, docH);
}

void WidgetWebview::cbScroll(void* ud, int x, int y, int docW, int docH)
{
    /* Engine-authoritative scroll (page swap -> 0, window.scrollTo, or a script
     * run end). Snap the live offset + geometry and refresh the scrollbar. */
    WidgetWebview* self = (WidgetWebview*)ud;
    if(self == nullptr)
        return;
    self->m_scrollX = x;
    self->m_scrollY = y;
    self->m_docW = docW;
    self->m_docH = docH;
    self->updateScroller();
    self->update();
}

void WidgetWebview::cbUrl(void* ud, const char* url)
{
    /* The visible page's URL changed (address-bar load or an in-page
     * navigation). Mirror it for getCurrentUrl() and fire the hook. */
    WidgetWebview* self = (WidgetWebview*)ud;
    if(self == nullptr)
        return;
    self->m_uiCurrentUrl = (url != nullptr) ? url : "";
    self->onHtmlUrlChanged(self->m_uiCurrentUrl);
}

void WidgetWebview::cbStatus(void* ud, const char* text, int progress)
{
    WidgetWebview* self = (WidgetWebview*)ud;
    if(self != nullptr)
        self->onBuildStatus((text != nullptr) ? text : "", progress);
}

void WidgetWebview::cbBuildStatus(void* ud, const char* text, int progress, bool overlay)
{
    /* Build overlay mirrors: text, progress and the veil flag. */
    WidgetWebview* self = (WidgetWebview*)ud;
    if(self == nullptr)
        return;
    self->m_buildStatus = (text != nullptr) ? text : "";
    self->m_buildProgress = progress;
    self->m_uiBuildOverlay = overlay;
    self->onBuildStatus(self->m_buildStatus, progress);
    self->update();
}

void WidgetWebview::cbDialog(void* ud, const char* text)
{
    /* alert()/confirm()/prompt() text, surfaced non-blocking (the engine never
     * waits on a modal; confirm answers "cancel", prompt answers null). */
    (void)ud;
    klog("[xBrowser] dialog: %s\n", (text != nullptr) ? text : "");
}

void WidgetWebview::cbTaskStart(void* ud, const char* url, int type)
{
    WidgetWebview* self = (WidgetWebview*)ud;
    if(self == nullptr)
        return;
    HttpTask task;
    task.url = (url != nullptr) ? url : "";
    task.type = type;
    task.loading = true;
    self->onTaskStart(task);
}

void WidgetWebview::cbTaskEnd(void* ud, const char* url, int type)
{
    WidgetWebview* self = (WidgetWebview*)ud;
    if(self == nullptr)
        return;
    HttpTask task;
    task.url = (url != nullptr) ? url : "";
    task.type = type;
    task.loading = false;
    self->onTaskEnd(task);
}

void WidgetWebview::cbTaskFailed(void* ud, const char* url, int type)
{
    WidgetWebview* self = (WidgetWebview*)ud;
    if(self == nullptr)
        return;
    HttpTask task;
    task.url = (url != nullptr) ? url : "";
    task.type = type;
    task.loading = false;
    self->onTaskFailed(task);
}

void WidgetWebview::cbTasksEnd(void* ud)
{
    WidgetWebview* self = (WidgetWebview*)ud;
    if(self != nullptr)
        self->onTasksEnd();
}

/* ==================================================================
 * Frame adoption + UI-local scrolling
 * ================================================================== */

void WidgetWebview::adoptFrame(eweb_surface_t* buf, int renderX, int renderY, int docW, int docH)
{
    /* UI-THREAD ONLY (from cbFrame). Take ownership of the engine-rendered
     * frame as the new front buffer and hand the previous one back to the
     * engine's pool (it is recycled, or freed when a resize changed its size).
     * Records the offset/geometry the frame was rendered at so onRepaint blits
     * it aligned to the live scroll and updateScroller sizes the scrollbar. */
    if(buf == nullptr || m_view == nullptr)
        return;
    eweb_surface_t* old = m_displayCache;
    m_displayCache = buf;
    m_frameScrollX = renderX;
    m_frameScrollY = renderY;
    m_docW = docW;
    m_docH = docH;
    if(old != nullptr)
        ewebview_release_frame(m_view, old);

    updateScroller();
    update();
}

void WidgetWebview::uiLocalScroll(int newX, int newY)
{
    /* UI-THREAD ONLY (wheel/drag, via onScroll). Clamp the requested offset to
     * the last-known document geometry and move the live UI offset, so
     * onRepaint blits the cached frame shifted by (frameScroll - scroll) and
     * the page tracks the gesture immediately. Then ewebview_scroll() so the
     * engine re-renders the newly exposed content at the new offset and fires
     * the page's scroll handlers there. */
    /* m_docW/H and m_scroll* are LOGICAL px; the visible page is the device
     * area scaled down by dpr. Clamp in logical units to match. */
    int viewW = (int)ceilf((float)area.w / m_dpr);
    int viewH = (int)ceilf((float)area.h / m_dpr);
    int maxX = m_docW - viewW; if (maxX < 0) maxX = 0;
    int maxY = m_docH - viewH; if (maxY < 0) maxY = 0;
    if (newX < 0) newX = 0; else if (newX > maxX) newX = maxX;
    if (newY < 0) newY = 0; else if (newY > maxY) newY = maxY;
    if (newX == m_scrollX && newY == m_scrollY)
        return;
    m_scrollX = newX;
    m_scrollY = newY;

    if(m_view != nullptr)
        ewebview_scroll(m_view, newX, newY);

    updateScroller();
    update();
}

/* ==================================================================
 * Widget overrides
 * ================================================================== */

void WidgetWebview::onTimer(uint32_t timerFPS, uint32_t timerSteps)
{
    (void)timerFPS;
    (void)timerSteps;

    /* UI-THREAD ONLY. Everything heavy runs on the engine thread; this tick
     * only drains the engine->UI events (adopting frames, refreshing the
     * status/url/scrollbar and firing the virtual hooks - which touch the
     * embedding app's widgets, so they MUST run here, on the UI thread). */
    if(m_view != nullptr)
        ewebview_tick(m_view);

    /* Red-line watchdog: the UI tick must stay trivial so xwin events are
     * served promptly. Opt-in heap snapshot via XBROWSER_HEAPSTAT=1. */
    uint64_t now = kernel_tic_ms(0);
    if (now - m_lastStatLogAt >= 1000) {
        m_lastStatLogAt = now;
        if (heap_stat_enabled()) {
            uint32_t blocks = 0;
            uint32_t free_blocks = 0;
            uint32_t used_bytes = 0;
            uint32_t free_bytes = 0;
            uint32_t flist_len = 0;
            uint32_t flist_max = 0;
            ewok_heap_stat(&blocks, &free_blocks, &used_bytes, &free_bytes,
                &flist_len, &flist_max);
            klog("[xBrowser] ui watchdog: heap blocks=%u free=%u used=%u KB holes=%u KB flist=%u/%u flmax=%u\n",
                blocks, free_blocks, used_bytes / 1024, free_bytes / 1024,
                flist_len, free_blocks, flist_max);
        }
    }
}

void WidgetWebview::onRepaint(graph_t* g, XTheme* theme, const grect_t& r)
{
    if (g == NULL)
        return;

    /* UI-THREAD ONLY: blit the latest frame the engine rendered. The buffer was
     * rendered at m_frameScroll*, but the live scroll offset (m_scroll*) may
     * have moved ahead during a fast wheel/drag, so blit it shifted by the
     * difference: already-rendered content tracks the scroll immediately and
     * only the newly-exposed edge stays white until the engine's refill frame
     * is adopted. */
    graph_fill_rect(g, r.x, r.y, r.w, r.h, 0xFFFFFFFF);

    graph_t* frame = frameGraph(m_displayCache);
    bool has_frame = (frame != nullptr && frame->buffer != nullptr);
    if (has_frame) {
        /* The frame is a DEVICE-pixel buffer (~area size), blitted 1:1 for a
         * crisp result. The scroll offsets are LOGICAL, so scale the render-vs-
         * live delta up by dpr to shift the device buffer by the right amount. */
        int dx = r.x + (int)lroundf((float)(m_frameScrollX - m_scrollX) * m_dpr);
        int dy = r.y + (int)lroundf((float)(m_frameScrollY - m_scrollY) * m_dpr);
        graph_set_clip(g, r.x, r.y, r.w, r.h);
        graph_blt(frame, 0, 0, frame->w, frame->h, g, dx, dy, frame->w, frame->h);
        graph_unset_clip(g);
    }

    /* The build overlay ("building...", progress bar) covers the page while a
     * real build - not a post-swap script run - is in flight. All three fields
     * are UI-thread mirrors written by cbBuildStatus. */
    if (m_uiBuildOverlay) {
        font_t* font = theme ? theme->getFont() : nullptr;
        uint32_t fg = theme ? theme->basic.docFGColor : 0xFF000000;
        uint32_t bg = 0xFFE8E8E8;
        int build_progress = m_buildProgress;
        int box_w = has_frame ? (r.w / 3) : 260;
        if (box_w < 220) box_w = 220;
        int box_h = 48;
        int box_x = r.x + (r.w - box_w) / 2;
        int box_y = r.y + (r.h - box_h) / 2;
        graph_fill_rect(g, box_x, box_y, box_w, box_h, bg);
        graph_rect(g, box_x, box_y, box_w, box_h, 0xFF808080);
        int bar_x = box_x + 8;
        int bar_y = box_y + box_h - 14;
        int bar_w = box_w - 16;
        graph_rect(g, bar_x, bar_y, bar_w, 8, 0xFF909090);
        int fill_w = (bar_w - 2) * build_progress / 100;
        if (fill_w < 0) fill_w = 0;
        graph_fill_rect(g, bar_x + 1, bar_y + 1, fill_w, 6, 0xFF4A90E2);
        if (font != NULL && !m_buildStatus.empty()) {
            int text_y = box_y + 8;
            graph_draw_text_font(g, box_x + 8, text_y, m_buildStatus.c_str(), font,
                theme ? theme->basic.fontSize : 16, fg);
        }
    }
}

void WidgetWebview::onResize()
{
    /* UI-THREAD ONLY: the window changed size. Hand the new size to the engine
     * (re-layout, re-render, window.onresize all happen there); here we only
     * refresh the UI-side drag step and scrollbar geometry.
     *
     * The engine works in LOGICAL (CSS) px, so report the viewport as the
     * device area scaled down by dpr; the port allocates the frame at
     * logical*dpr == device px. ceilf keeps the device buffer >= the widget. */
    int viewW = (int)ceilf((float)area.w / m_dpr);
    int viewH = (int)ceilf((float)area.h / m_dpr);
    if(viewW < 1) viewW = 1;
    if(viewH < 1) viewH = 1;
    dragStep = viewH / 4;   /* LOGICAL-px quantum (scroll offsets are logical) */

    if(m_view != nullptr)
        ewebview_set_viewport(m_view, viewW, viewH);

    updateScroller();
}

bool WidgetWebview::onScroll(int step, bool horizontal)
{
    /* UI-THREAD ONLY: wheel/drag scroll. Move the live UI offset (clamped to
     * the last-known document geometry), which makes onRepaint blit the cached
     * frame shifted so the page tracks the gesture immediately, and queue an
     * engine scroll (via uiLocalScroll) so it re-renders the exposed content
     * and fires the page's scroll handlers there. */
    int oldX = m_scrollX;
    int oldY = m_scrollY;
    int newX = m_scrollX;
    int newY = m_scrollY;
    if (horizontal)
        newX -= step * dragStep;
    else
        newY -= step * dragStep;
    uiLocalScroll(newX, newY);
    return (m_scrollX != oldX || m_scrollY != oldY);
}

void WidgetWebview::updateScroller()
{
    /* UI-THREAD ONLY: set the scrollbar geometry from the last-known document
     * size (reported by the engine with each frame / scroll clamp) and the live
     * UI offset. No document access - the engine is authoritative. All three
     * inputs are LOGICAL px (doc size, offset, and the device area / dpr). */
    int viewW = (int)ceilf((float)area.w / m_dpr);
    int viewH = (int)ceilf((float)area.h / m_dpr);
    setScrollerInfo(m_docW, m_scrollX, viewW, true);
    setScrollerInfo(m_docH, m_scrollY, viewH, false);
}

bool WidgetWebview::onMouse(xevent_t* ev)
{
    /* UI-THREAD ONLY. Two independent jobs:
     *   1. forward the gesture - translated to an eweb_event_t carrying the
     *      widget-relative client coords the engine cannot compute itself
     *      (getInsidePos walks UI-only widget geometry) - so the engine
     *      dispatches the DOM mouse events and follows <a href> clicks there.
     *      Asynchronous: the UI never blocks on the page's verdict, so a heavy
     *      handler cannot freeze the window.
     *   2. do the visual side here (drag-scroll via Scrollable, wheel-scroll)
     *      so the page tracks the gesture immediately.
     * Wheel gestures and drags are not DOM events and are not forwarded (the
     * wheel is turned into a scroll right here). */
    if (ev == nullptr)
        return false;

    if (m_view != nullptr && ev->type == XEVT_MOUSE) {
        int state = -1;
        switch (ev->state) {
        case MOUSE_STATE_MOVE:         state = EWEB_MOUSE_MOVE; break;
        case MOUSE_STATE_DOWN:         state = EWEB_MOUSE_DOWN; break;
        case MOUSE_STATE_UP:           state = EWEB_MOUSE_UP; break;
        case MOUSE_STATE_CLICK:        state = EWEB_MOUSE_CLICK; break;
        case MOUSE_STATE_DOUBLE_CLICK: state = EWEB_MOUSE_DOUBLE_CLICK; break;
        default: break;   /* DRAG and anything else: no DOM event */
        }
        int32_t btn = ev->value.mouse.button;
        if (state == EWEB_MOUSE_MOVE &&
            (btn == MOUSE_BUTTON_SCROLL_UP || btn == MOUSE_BUTTON_SCROLL_DOWN ||
             btn == MOUSE_BUTTON_SCROLL_LEFT || btn == MOUSE_BUTTON_SCROLL_RIGHT))
            state = -1;   /* the wheel is a scroll request, handled below */
        if (state >= 0) {
            gpos_t ip = getInsidePos(ev->value.mouse.x, ev->value.mouse.y);
            eweb_event_t fev;
            fev.mouse_state = state;
            fev.button = (btn == MOUSE_BUTTON_LEFT)  ? EWEB_BUTTON_LEFT :
                         (btn == MOUSE_BUTTON_MID)   ? EWEB_BUTTON_MIDDLE :
                         (btn == MOUSE_BUTTON_RIGHT) ? EWEB_BUTTON_RIGHT :
                                                       EWEB_BUTTON_NONE;
            /* getInsidePos returns DEVICE px; the engine hit-tests in LOGICAL
             * (CSS) px, so scale the client coordinates down by dpr. */
            fev.cx = (int)lroundf((float)ip.x / m_dpr);
            fev.cy = (int)lroundf((float)ip.y / m_dpr);
            fev.wheel = 0;
            ewebview_post_event(m_view, &fev);
        }
    }

    // Drag scrolling (moves m_scrollX/Y through onScroll -> uiLocalScroll).
    bool handled = Scrollable::onMouse(ev);
    if (handled)
        return true;

    // Mouse wheel scrolling.
    if (ev->state == MOUSE_STATE_MOVE) {
        if (ev->value.mouse.button == MOUSE_BUTTON_SCROLL_UP) {
            scroll(-1, false);
            return true;
        }
        else if (ev->value.mouse.button == MOUSE_BUTTON_SCROLL_DOWN) {
            scroll(1, false);
            return true;
        }
    }
    return false;
}

void WidgetWebview::setAttr(const string& attr, json_var_t*value) {
    Scrollable::setAttr(attr, value);
    if(attr == "url") {
        const char* url = json_var_get_str(value);
        loadHtml(url);
    }
}
