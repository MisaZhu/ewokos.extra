#include "OpenLookWM.h"
#include <ewoksys/kernel_tic.h>
#include <ewoksys/vfs.h>
#include <ewoksys/klog.h>
#include <graph/graph_image.h>
#include <x/x.h>
#include <stdlib.h>
#include <string.h>

using namespace Ewok;

void OpenLookWM::drawMin(graph_t* g, xinfo_t* info, grect_t* r, bool top) {
	(void)info;
}

void OpenLookWM::drawMax(graph_t* g, xinfo_t* info, grect_t* r, bool top) {
	(void)info;
	if(frameMaxIcon == NULL)
		return;

	graph_blt_alpha(frameMaxIcon, 0, 0, frameMaxIcon->w, frameMaxIcon->h,
			g, r->x + (r->w-frameMaxIcon->w)/2,
			r->y + (r->h-frameMaxIcon->h)/2, frameMaxIcon->w, frameMaxIcon->h, 0xff);
}

void OpenLookWM::drawClose(graph_t* g, xinfo_t* info, grect_t* r, bool top) {
	(void)info;
	if(frameCloseIcon == NULL)
		return;
	graph_blt_alpha(frameCloseIcon, 0, 0, frameCloseIcon->w, frameCloseIcon->h,
			g, r->x + (r->w-frameCloseIcon->w)/2,
			r->y + (r->h-frameCloseIcon->h)/2, frameCloseIcon->w, frameCloseIcon->h, 0xff);
}

void OpenLookWM::drawDragFrame(graph_t* g, grect_t* r) {
	int wd = xwm.theme.frameW;
	if(wd <= 0)
		wd = 2;
	graph_frame(g, r->x-wd, r->y-wd, 
			r->w+wd*2, r->h+wd*2, wd, 0x88ffffff, false);
}

void OpenLookWM::drawResize(graph_t* g, xinfo_t* info, grect_t* r, bool top) {
}

void OpenLookWM::drawFrame(graph_t* desktop_g, graph_t* frame_g, graph_t* ws_g, xinfo_t* info, grect_t* r, bool top) {
	uint32_t fg, bg;
	getColor(&fg, &bg, top);

	int x = r->x;
	int y = r->y;
	int w = r->w;
	int h = r->h;

	graph_fill_rect(frame_g, x, y, w, xwm.theme.frameW, bg);
	graph_fill_rect(frame_g, x, y, xwm.theme.frameW, h, bg);
	graph_fill_rect(frame_g, x, y+h-xwm.theme.frameW, w, xwm.theme.frameW, bg);
	graph_fill_rect(frame_g, x+w-xwm.theme.frameW, y, xwm.theme.frameW, h, bg);

	graph_fill_rect(frame_g, x, y, w, 2, 0xff000000);
	graph_fill_rect(frame_g, x, y, 2, h, 0xff000000);
	graph_fill_rect(frame_g, x, y+h-2, w, 2, 0xff000000);
	graph_fill_rect(frame_g, x+w-2, y, 2, h, 0xff000000);

	if(frameTLIcon)
		graph_blt_alpha(frameTLIcon, 0, 0, frameTLIcon->w, frameTLIcon->h,
			frame_g, x, y, frameTLIcon->w, frameTLIcon->h, 0xff);
	if(frameTRIcon)
		graph_blt_alpha(frameTRIcon, 0, 0, frameTRIcon->w, frameTRIcon->h,
			frame_g, x+w-frameTRIcon->w, y, frameTLIcon->w, frameTLIcon->h, 0xff);
	if(frameBLIcon)
		graph_blt_alpha(frameBLIcon, 0, 0, frameBLIcon->w, frameBLIcon->h,
			frame_g, x, y+h-frameBLIcon->h, frameBLIcon->w, frameBLIcon->h, 0xff);
	if(frameBRIcon)
		graph_blt_alpha(frameBRIcon, 0, 0, frameBRIcon->w, frameBRIcon->h,
			frame_g, x+w-frameBRIcon->w, y+h-frameBRIcon->h, frameBRIcon->w, frameBRIcon->h, 0xff);
	//shadow
	/*if(top) {
		graph_fill_rect(graph, x+w+xwm.theme.frameW, y, xwm.theme.frameW, h+xwm.theme.frameW, 0xaa000000);
		graph_fill_rect(graph, x, y+h+xwm.theme.frameW, w+xwm.theme.frameW*2, xwm.theme.frameW, 0xaa000000);
	}
	*/
}

void OpenLookWM::drawTitle(graph_t* desktop_g, graph_t* g, xinfo_t* info, grect_t* r, bool top) {
	uint32_t fg, bg;
	getColor(&fg, &bg, top);

	uint32_t dark, bright;
	graph_get_3d_color(bg, &dark, &bright);

	gsize_t sz;
	font_text_size(info->title, font, xwm.theme.fontSize, (uint32_t*)&sz.w, (uint32_t*)&sz.h);
	
	int pw = (r->w-sz.w)/2;
	int ph = (r->h-sz.h)/2;
	graph_fill_rect(g, r->x, r->y, r->w, r->h, bg);
	graph_draw_text_font(g, r->x+pw, r->y+ph, info->title, font, xwm.theme.fontSize, fg);//title

	//graph_line(g, r->x, r->y+r->h-3, r->x+r->w, r->y+r->h-3, dark);
	//graph_line(g, r->x, r->y+r->h-2, r->x+r->w, r->y+r->h-2, bright);
}

void OpenLookWM::onLoadTheme(void) {
	char fname[FS_FULL_NAME_MAX+1] = {0};
	x_get_theme_fname(X_THEME_ROOT, "xwm", "icons/bottom_left.png", fname, FS_FULL_NAME_MAX);
	frameBLIcon = graph_image_new(fname);
	x_get_theme_fname(X_THEME_ROOT, "xwm", "icons/top_left.png", fname, FS_FULL_NAME_MAX);
	frameTLIcon = graph_image_new(fname);
	x_get_theme_fname(X_THEME_ROOT, "xwm", "icons/bottom_right.png", fname, FS_FULL_NAME_MAX);
	frameBRIcon = graph_image_new(fname);
	x_get_theme_fname(X_THEME_ROOT, "xwm", "icons/top_right.png", fname, FS_FULL_NAME_MAX);
	frameTRIcon = graph_image_new(fname);
	x_get_theme_fname(X_THEME_ROOT, "xwm", "icons/button_down.png", fname, FS_FULL_NAME_MAX);
	frameCloseIcon = graph_image_new(fname);
	x_get_theme_fname(X_THEME_ROOT, "xwm", "icons/button_max.png", fname, FS_FULL_NAME_MAX);
	frameMaxIcon = graph_image_new(fname);
}

OpenLookWM::~OpenLookWM(void) {
}

OpenLookWM::OpenLookWM(void) {
}