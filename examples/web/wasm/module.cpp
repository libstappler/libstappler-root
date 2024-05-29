/**
 Copyright (c) 2024 Stappler LLC <admin@stappler.dev>

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 **/

#include "stappler.h"
#include <string.h>

struct Component {
	stappler_wasm_webserver_borrow_host_component_t handle;

	static void handleChildInit(Component *c, stappler_wasm_webserver_borrow_host_t) {
		stappler_string_t str;
		str.ptr = (uint8_t *)"handleChildInit";
		str.len = strlen("handleChildInit");
		stappler_wasm_wasm_debug_print(&str);
	}

	static void handleStorageInit(Component *c, stappler_wasm_webserver_borrow_host_t, int32_t handle) {
		stappler_string_t str;
		str.ptr = (uint8_t *)"handleStorageInit";
		str.len = strlen("handleStorageInit");
		stappler_wasm_wasm_debug_print(&str);
	}

	static void handleHeartbeat(Component *c, stappler_wasm_webserver_borrow_host_t) {
		stappler_string_t str;
		str.ptr = (uint8_t *)"handleHeartbeat";
		str.len = strlen("handleHeartbeat");
		stappler_wasm_wasm_debug_print(&str);
	}
};

extern "C"
exports_stappler_wasm_app_own_host_component_t exports_stappler_wasm_app_make_component(
		exports_stappler_wasm_app_borrow_host_t h, exports_stappler_wasm_app_borrow_host_component_info_t i) {
	stappler_wasm_webserver_host_component_data_t data;

	auto comp = new Component;

	data.on_child_init = (uintptr_t)&Component::handleChildInit;
	data.on_storage_init = (uintptr_t)&Component::handleStorageInit;
	data.on_heartbeat = (uintptr_t)&Component::handleHeartbeat;
	data.userdata = (uintptr_t)comp;

	auto c = stappler_wasm_webserver_constructor_host_component(h, i, &data);
	comp->handle = stappler_wasm_webserver_borrow_host_component(c);
	return c;
}
