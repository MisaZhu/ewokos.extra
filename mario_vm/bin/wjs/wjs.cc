#include <Widget/WidgetX.h>
#include <Widget/Image.h>
#include <Widget/Label.h>
#include <Widget/LabelButton.h>
#include <Widget/List.h>
#include <Widget/EditLine.h>
#include <Widget/Grid.h>
#include <Widget/Scroller.h>
#include <Widget/Splitter.h>
#include <WidgetEx/LayoutWidget.h>
#include <WidgetEx/FileDialog.h>
#include <WidgetEx/ConfirmDialog.h>

#include <WidgetEx/LayoutWin.h>

#include <x++/X.h>
#include <unistd.h>
#include <font/font.h>

#include "js.h"
#include "mbc.h"
#include "platform.h"
#include "mem.h"

/**
load extra native libs.
*/

#ifdef __cplusplus /* __cplusplus */
extern "C" {
#endif

void reg_natives(vm_t* vm);
bool js_compile(bytecode_t *bc, const char* input);

vm_t* init_js(void) {
	platform_init();
	mem_init();
	vm_t* vm = vm_new(js_compile);
	vm->gc_buffer_size = 1024;
	vm_init(vm, reg_natives, NULL);
	return vm;
}

bool load_wjs(vm_t* vm, const char* fname) {
	if(fname == NULL || fname[0] == 0)
		return false;
	bool res = false;
	if(strstr(fname, ".js") != NULL)
		res = load_js(vm, fname);
	else if(strstr(fname, ".mbc") != NULL) {
		bc_release(&vm->bc);
		res = vm_load_mbc(vm, fname);
	}
	return res;
}

void quit_js(vm_t* vm) {
	vm_close(vm);
	mem_quit();
}

#ifdef __cplusplus /* __cplusplus */
}
#endif

vm_t* _vm;

using namespace Ewok;

static void onMenuItemFunc(MenuItem* it, void* data) {
	klog("onMenuItemFunc: %d\n", it->id);
}

static void onEventFunc(Widget* wd, xevent_t* xev, void* arg) {
	vm_t* vm = _vm;
	
	var_t* evt_arg = var_new_obj(vm, NULL, NULL);
	var_add(evt_arg, "type", var_new_int(vm, xev->type));
	var_add(evt_arg, "state", var_new_int(vm, xev->state));

	var_t* mouse_arg = var_new_obj(vm, NULL, NULL);
	var_add(mouse_arg, "x", var_new_int(vm, xev->value.mouse.x));
	var_add(mouse_arg, "y", var_new_int(vm, xev->value.mouse.y));
	var_add(mouse_arg, "rx", var_new_int(vm, xev->value.mouse.rx));
	var_add(mouse_arg, "ry", var_new_int(vm, xev->value.mouse.ry));
	var_add(evt_arg, "mouse", mouse_arg);

	var_t* im_arg = var_new_obj(vm, NULL, NULL);
	var_add(im_arg, "value", var_new_int(vm, xev->value.im.value));
	var_add(evt_arg, "im", im_arg);

	var_t* args = var_new(vm);
	var_add(args, "widgetID", var_new_int(vm, wd->getID()));
	var_add(args, "widgetName", var_new_str(vm, wd->getName().c_str()));
	var_add(args, "event", evt_arg);

	call_m_func_by_name(_vm, NULL, "_onWidgetEvent", args);
}

int main(int argc, char** argv) {
	if(argc < 3) {
		klog("Usage: %s <xxxx.xw> <xxxx.js>\n", argv[0]);
		return 0;
	}
	
	X x;
	LayoutWin win;
	LayoutWidget* layout = win.getLayoutWidget();
	layout->setMenuItemFunc(onMenuItemFunc);
	layout->setEventFunc(onEventFunc);
	win.loadConfig(argv[1]); // 加载布局文件

	_vm = init_js();
	_m_debug = true;
	load_wjs(_vm, argv[2]);
	vm_run(_vm);

	win.open(&x, 0, -1, -1, 0, 0, argv[1], XWIN_STYLE_NORMAL);
	win.setTimer(16);

	widgetXRun(&x, &win);	
	quit_js(_vm);
	return 0;
}