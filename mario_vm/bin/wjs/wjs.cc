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

#include <tinyjson/tinyjson.h>

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
	vm_t* vm = _vm;
	var_t* args = var_new(vm);
	var_add(args, "menuID", var_new_int(vm, it->id));
	call_m_func_by_name(_vm, NULL, "_onMenuItemEvent", args);
	var_unref(args);
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
	var_unref(args);
}

static int doargs(int argc, char* argv[]) {
	int c = 0;
	while (c != -1) {
		c = getopt (argc, argv, "d");
		if(c == -1)
			break;

		switch (c) {
		case 'd':
			_m_debug = true;
			break;
		case '?':
			return -1;
		default:
			c = -1;
			break;
		}
	}
	return optind;
}

static bool loadWJS(const string& wjs_fname, string& layout_fname, string& js_fname) {
    json_var_t* conf_var = json_parse_file(wjs_fname.c_str());
    if(conf_var == NULL)
        return false;
		
	layout_fname = json_get_str(conf_var, "layout");
	js_fname = json_get_str(conf_var, "js");
    
    json_var_unref(conf_var);
    return true;
}

//============= Widget natives =================================

#define CLS_WJS "WJS"
#define CLS_Widget "Widget"
#define CLS_XEvent "XEvent"

void free_none(void* data) {
}

var_t* native_wjs_getByName(vm_t* vm, var_t* env, void* data) {
	LayoutWidget* layout = (LayoutWidget*)data;
	const char* name = get_str(env, "name");
	Widget* wd = layout->getChild(name);
	if(wd == NULL)
		return NULL;

	var_t* var_wd = new_obj(vm, CLS_Widget, 0);
	var_wd->value = wd;
	var_wd->free_func = free_none;
	return var_wd;
}

var_t* native_wjs_getByID(vm_t* vm, var_t* env, void* data) {
	LayoutWidget* layout = (LayoutWidget*)data;
	uint32_t id = get_int(env, "id");
	Widget* wd = layout->getChild(id);
	if(wd == NULL)
		return NULL;

	var_t* var_wd = new_obj(vm, CLS_Widget, 0);
	var_wd->value = wd;
	var_wd->free_func = free_none;
	return var_wd;
}

var_t* native_widget_get(vm_t* vm, var_t* env, void* data) {
	Widget* wd = (Widget*)get_raw(env, THIS);
	if(wd == NULL)
		return NULL;

	string name = get_str(env, "name");
	json_var_t* value = wd->get(name);	
	string json_str = "";
	if(value != NULL) {
		str_t* str = str_new("");
		if(str == NULL)
			return NULL;
		json_var_to_json_str(value, str, 0);
		json_var_unref(value);
		json_str = str->cstr;
		str_free(str);
	}

	var_t* ret = var_new_str(vm, json_str.c_str());
	return ret;
}

var_t* native_widget_set(vm_t* vm, var_t* env, void* data) {
	Widget* wd = (Widget*)get_raw(env, THIS);
	if(wd == NULL)
		return NULL;

	string name = get_str(env, "name");
	var_t* value = get_obj(env, "value");

	if(value != NULL) {
		mstr_t* str = mstr_new("");
		if(str == NULL)
			return NULL;

		var_to_json_str(value, str, 0);
		json_var_t* json_value = json_parse_str(str->cstr);
		mstr_free(str);

		if(json_value == NULL)
			return NULL;

		wd->set(name, json_value);
		json_var_unref(json_value);
	}

	return NULL;
}

#ifdef __cplusplus /* __cplusplus */
extern "C" {
#endif

void reg_native_widget(vm_t* vm, LayoutWidget* layout) {
	var_t* cls = vm_new_class(vm, CLS_WJS);
	vm_reg_static(vm, cls, "getWidgetByName(name)", native_wjs_getByName, layout); 
	vm_reg_static(vm, cls, "getWidgetByID(id)", native_wjs_getByID, layout); 

	cls = vm_new_class(vm, CLS_XEvent);
	vm_reg_var(vm, cls, "MOUSE", var_new_int(vm, XEVT_MOUSE), true);
	vm_reg_var(vm, cls, "IM", var_new_int(vm, XEVT_IM), true);
	vm_reg_var(vm, cls, "MOUSE_MOVE", var_new_int(vm, MOUSE_STATE_MOVE), true);
	vm_reg_var(vm, cls, "MOUSE_DOWN", var_new_int(vm, MOUSE_STATE_DOWN), true);
	vm_reg_var(vm, cls, "MOUSE_UP", var_new_int(vm, MOUSE_STATE_UP), true);
	vm_reg_var(vm, cls, "MOUSE_DRAG", var_new_int(vm, MOUSE_STATE_DRAG), true);
	vm_reg_var(vm, cls, "MOUSE_CLICK", var_new_int(vm, MOUSE_STATE_CLICK), true);

	cls = vm_new_class(vm, CLS_Widget);
	vm_reg_native(vm, cls, "get(name)", native_widget_get, layout); 
	vm_reg_native(vm, cls, "set(name, value)", native_widget_set, layout); 
}

#ifdef __cplusplus /* __cplusplus */
}
#endif

int main(int argc, char** argv) {
	int argind = doargs(argc, argv);
	if(argind < 0) {
		return -1;
	}

	const char* wsj_fname = "";
	if(argind < argc) {
		wsj_fname = argv[argind];
		argind++;
	}

	if(wsj_fname[0] == 0) {
		klog("Usage: %s <xxxx.wjs>\n", argv[0]);
		return -1;
	}

	string layout_fname, js_fname;
	if(!loadWJS(wsj_fname, layout_fname, js_fname)) {
		return -1;
	}

	X x;
	LayoutWin win;
	LayoutWidget* layout = win.getLayoutWidget();

	_vm = init_js();
	reg_native_widget(_vm, layout);
	load_wjs(_vm, js_fname.c_str());
	vm_run(_vm);

	layout->setMenuItemFunc(onMenuItemFunc);
	layout->setEventFunc(onEventFunc);

	win.loadConfig(layout_fname.c_str()); // 加载布局文件
	win.open(&x, 0, -1, -1, 0, 0, argv[1], XWIN_STYLE_NORMAL);
	win.setTimer(16);

	widgetXRun(&x, &win);	
	quit_js(_vm);
	return 0;
}