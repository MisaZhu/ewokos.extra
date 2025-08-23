#include "native_UniObject.h"
#include "UniObject/UniObject.h"
#include <tinyjson/tinyjson.h>
#include <string>

using namespace std;
using namespace Ewok;

#define CLS_UniObject "UniObject"

static string json_var_to_cxxstr(json_var_t* var) {
	str_t* str = str_new("");
	json_var_to_json_str(var, str, 0);
	string ret = str->cstr;
	str_free(str);
	return ret;
}

static string var_to_cxxstr(var_t* var) {
	mstr_t* str = mstr_new("");
	var_to_json_str(var, str, 0);
	string ret = str->cstr;
	mstr_free(str);
	return ret;
}

var_t* native_UniObject_get(vm_t* vm, var_t* env, void* data) {
	UniObject* obj = (UniObject*)get_raw(env, THIS);
	if(obj == NULL)
		return NULL;

	string name = get_str(env, "name");
	json_var_t* value = obj->get(name);	
	string json_str = "";
	if(value != NULL) {
		json_str = json_var_to_cxxstr(value);
		json_var_unref(value);
	}

	var_t* ret = var_new_str(vm, json_str.c_str());
	return ret;
}

var_t* native_UniObject_set(vm_t* vm, var_t* env, void* data) {
	UniObject* obj = (UniObject*)get_raw(env, THIS);
	if(obj == NULL)
		return NULL;

	string name = get_str(env, "name");
	var_t* value = get_obj(env, "value");

	json_var_t* json_value = NULL;

	if(value != NULL) {
		string str = var_to_cxxstr(value);
		json_value = json_parse_str(str.c_str());
	}

	if(json_value != NULL) {
		obj->set(name, json_value);
		json_var_unref(json_value);
	}

	return NULL;
}

#ifdef __cplusplus /* __cplusplus */
extern "C" {
#endif

void reg_native_UniObject(vm_t* vm) {
	var_t* cls = vm_new_class(vm, CLS_UniObject);
	vm_reg_native(vm, cls, "get(name)", native_UniObject_get, NULL); 
	vm_reg_native(vm, cls, "set(name, value)", native_UniObject_set, NULL); 
}

#ifdef __cplusplus /* __cplusplus */
}
#endif
