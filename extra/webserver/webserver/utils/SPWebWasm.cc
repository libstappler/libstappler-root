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

#include "SPWebWasmComponent.h"

#if MODULE_STAPPLER_WASM

namespace STAPPLER_VERSIONIZED stappler::web {

static uint32_t stappler_wasm_webserver_constructor_host_component(wasm_exec_env_t exec_env, uint32_t hostHandle, uint32_t infoHandle,
		uint32_t onChildInit, uint32_t onStorageInit, uint32_t onHeartbeat, uint32_t userdata) {
	auto env = wasm::ExecEnv::get(exec_env);
	auto inst = env->getInstance();
	auto host = inst->getObject<Host>(hostHandle);
	if (!host) {
		log::error("wasm::Runtime", __FUNCTION__, ": invalid host handle");
		return wasm::ModuleInstance::InvalidHandle;
	}

	auto info = inst->getObject<HostComponentInfo>(infoHandle);
	if (!host) {
		log::error("wasm::Runtime", __FUNCTION__, ": invalid host info handle");
		return wasm::ModuleInstance::InvalidHandle;
	}

	auto c = new WasmComponent(*host, *info, env, WasmComponent::WasmData{onChildInit, onStorageInit, onHeartbeat, userdata});
	return inst->addHandle(c);
}

static NativeSymbol stapper_wen_symbols[] = {
	NativeSymbol{"[constructor]host-component", (void *)&stappler_wasm_webserver_constructor_host_component, "(iiiiii)i", NULL},
};

static wasm::NativeModule s_wasmModule("stappler:wasm/webserver", stapper_wen_symbols, sizeof(stapper_wen_symbols) / sizeof(NativeSymbol));

}

#endif
