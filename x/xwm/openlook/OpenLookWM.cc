#include "OpenLookWM.h"
#include <ewoksys/kernel_tic.h>
#include <ewoksys/klog.h>
#include <upng/upng.h>
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
	graph_frame(g, r->x-xwm.theme.frameW, r->y-xwm.theme.frameW, 
			r->w+xwm.theme.frameW*2, r->h+xwm.theme.frameW*2, xwm.theme.frameW, 0x88ffffff, false);
}

void OpenLookWM::drawResize(graph_t* g, xinfo_t* info, grect_t* r, bool top) {
	/*
	(void)info;
	if(!top)
		return;
	uint32_t fg, bg;
	getColor(&fg, &bg, top);

	uint32_t dark, bright;
	graph_get_3d_color(bg, &dark, &bright);

	graph_line(g, 
			r->x + r->w - frameW + 1, r->y,
			r->x + r->w, r->y, dark);
	graph_line(g,
			r->x + r->w - frameW + 1, r->y + 1,
			r->x + r->w, r->y + 1, bright);
	graph_line(g,
			r->x, r->y + r->h - frameW + 1,
			r->x, r->y + r->h, dark);
	graph_line(g,
			r->x + 1, r->y + r->h - frameW + 1,
			r->x + 1, r->y + r->h, bright);
	*/
}

void OpenLookWM::drawFrame(graph_t* graph, xinfo_t* info, bool top) {
	uint32_t fg, bg;
	getColor(&fg, &bg, top);

	int x = info->wsr.x;
	int y = info->wsr.y;
	int w = info->wsr.w;
	int h = info->wsr.h;

	if((info->style & XWIN_STYLE_NO_TITLE) == 0) {
		h += xwm.theme.titleH;
		y -= xwm.theme.titleH;
	}

	graph_fill(graph, x-xwm.theme.frameW, y-xwm.theme.frameW, w+xwm.theme.frameW*2, xwm.theme.frameW, bg);
	graph_fill(graph, x-xwm.theme.frameW, y-xwm.theme.frameW, xwm.theme.frameW, h+xwm.theme.frameW*2, bg);
	graph_fill(graph, x-xwm.theme.frameW, y+h, w+xwm.theme.frameW*2, xwm.theme.frameW, bg);
	graph_fill(graph, x+w, y-xwm.theme.frameW, xwm.theme.frameW, h+xwm.theme.frameW*2, bg);

	graph_fill(graph, x-xwm.theme.frameW, y-xwm.theme.frameW, w+xwm.theme.frameW*2, 2, 0xff000000);
	graph_fill(graph, x-xwm.theme.frameW, y-xwm.theme.frameW, 2, h+xwm.theme.frameW*2, 0xff000000);
	graph_fill(graph, x-xwm.theme.frameW, y+h+xwm.theme.frameW-2, w+xwm.theme.frameW*2, 2, 0xff000000);
	graph_fill(graph, x+w+xwm.theme.frameW-2, y-xwm.theme.frameW, 2, h+xwm.theme.frameW*2, 0xff000000);

	if(frameTLIcon)
		graph_blt_alpha(frameTLIcon, 0, 0, frameTLIcon->w, frameTLIcon->h,
				graph, x-xwm.theme.frameW, y-xwm.theme.frameW, frameTLIcon->w, frameTLIcon->h, 0xff);
	if(frameTRIcon)
		graph_blt_alpha(frameTRIcon, 0, 0, frameTRIcon->w, frameTRIcon->h,
				graph, x+w-frameTRIcon->w+xwm.theme.frameW, y-xwm.theme.frameW, frameTLIcon->w, frameTLIcon->h, 0xff);
	if(frameBLIcon)
		graph_blt_alpha(frameBLIcon, 0, 0, frameBLIcon->w, frameBLIcon->h,
				graph, x-xwm.theme.frameW, y+h-frameBLIcon->h+xwm.theme.frameW, frameBLIcon->w, frameBLIcon->h, 0xff);
	if(frameBRIcon)
		graph_blt_alpha(frameBRIcon, 0, 0, frameBRIcon->w, frameBRIcon->h,
				graph, x+w-frameBRIcon->w+xwm.theme.frameW, y+h-frameBRIcon->h+xwm.theme.frameW, frameBRIcon->w, frameBRIcon->h, 0xff);
	//shadow
	/*if(top) {
		graph_fill(graph, x+w+xwm.theme.frameW, y, xwm.theme.frameW, h+xwm.theme.frameW, 0xaa000000);
		graph_fill(graph, x, y+h+xwm.theme.frameW, w+xwm.theme.frameW*2, xwm.theme.frameW, 0xaa000000);
	}
	*/
}

void OpenLookWM::drawTitle(graph_t* g, xinfo_t* info, grect_t* r, bool top) {
	uint32_t fg, bg;
	getColor(&fg, &bg, top);

	uint32_t dark, bright;
	graph_get_3d_color(bg, &dark, &bright);

	gsize_t sz;
	font_text_size(info->title, font, xwm.theme.fontSize, (uint32_t*)&sz.w, (uint32_t*)&sz.h);
	
	int pw = (r->w-sz.w)/2;
	int ph = (r->h-sz.h)/2;
	graph_fill(g, r->x, r->y, r->w, r->h, bg);
	graph_draw_text_font(g, r->x+pw, r->y+ph, info->title, font, xwm.theme.fontSize, fg);//title

	//graph_line(g, r->x, r->y+r->h-3, r->x+r->w, r->y+r->h-3, dark);
	//graph_line(g, r->x, r->y+r->h-2, r->x+r->w, r->y+r->h-2, bright);
}

OpenLookWM::~OpenLookWM(void) {
}

OpenLookWM::OpenLookWM(void) {
	frameBLIcon = png_image_new(x_get_theme_fname(X_THEME_ROOT, "xwm", "icons/bottom_left.png"));
	frameTLIcon = png_image_new(x_get_theme_fname(X_THEME_ROOT, "xwm", "icons/top_left.png"));
	frameBRIcon = png_image_new(x_get_theme_fname(X_THEME_ROOT, "xwm", "icons/bottom_right.png"));
	frameTRIcon = png_image_new(x_get_theme_fname(X_THEME_ROOT, "xwm", "icons/top_right.png"));
	frameCloseIcon = png_image_new(x_get_theme_fname(X_THEME_ROOT, "xwm", "icons/button_down.png"));
	frameMaxIcon = png_image_new(x_get_theme_fname(X_THEME_ROOT, "xwm", "icons/button_max.png"));
}