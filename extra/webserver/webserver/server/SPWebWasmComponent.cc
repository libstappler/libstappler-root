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

WasmComponent *WasmComponent::load(const Host &serv, const HostComponentInfo &info, wasm::Module *mod) {
	auto inst = Rc<wasm::ModuleInstance>::create(mod);
	if (!inst) {
		return nullptr;
	}

	auto env = Rc<wasm::ExecEnv>::create(inst);

	auto sym = inst->lookup(info.symbol);
	if (!sym) {
		return nullptr;
	}

	WasmComponent *result = nullptr;

	auto hostHandle = inst->addHandle(&serv);
	auto infoHandle = inst->addHandle(&info);

	wasm_val_t args[2] = {
		wasm::MakeValue(hostHandle),
		wasm::MakeValue(infoHandle),
	};

	auto idx = sym->call1(env, makeSpanView(args, 2));
	if (idx.kind == WASM_I32 && idx.of.i32 > 0) {
		result = inst->getObject<WasmComponent>(idx.of.i32);
	}

	inst->removeHandle(hostHandle);
	inst->removeHandle(infoHandle);

	return result;
}

WasmComponent::WasmComponent(const Host &serv, const HostComponentInfo &info, wasm::ExecEnv *env, WasmData &&data)
: HostComponent(serv, info), _env(env), _wasmData(move(data))  {

}

void WasmComponent::handleChildInit(const Host &host) {
	HostComponent::handleChildInit(host);

	if (_wasmData.childInitCallback != 0) {
		std::unique_lock lock(_mutex);
		auto hostHandle = _env->getInstance()->addHandle(&host);

		uint32_t args[2] = { _wasmData.userdata, hostHandle };
		_env->callIndirect(_wasmData.childInitCallback, 2, args);

		_env->getInstance()->removeHandle(hostHandle);
	}
}

void WasmComponent::handleStorageInit(const Host &host, const db::Adapter &a) {
	HostComponent::handleStorageInit(host, a);

	if (_wasmData.storageInitCallback != 0) {
		std::unique_lock lock(_mutex);
		auto hostHandle = _env->getInstance()->addHandle(&host);
		auto adapterHandle = _env->getInstance()->addHandle(&a);

		uint32_t args[3] = { _wasmData.userdata, hostHandle, adapterHandle };
		_env->callIndirect(_wasmData.storageInitCallback, 3, args);

		_env->getInstance()->removeHandle(adapterHandle);
		_env->getInstance()->removeHandle(hostHandle);
	}
}

void WasmComponent::initTransaction(db::Transaction &t) {
	HostComponent::initTransaction(t);
}

void WasmComponent::handleHeartbeat(const Host &host) {
	HostComponent::handleHeartbeat(host);

	if (_wasmData.heartbeatCallback != 0) {
		std::unique_lock lock(_mutex);
		auto hostHandle = _env->getInstance()->addHandle(&host);

		uint32_t args[2] = { _wasmData.userdata, hostHandle };
		_env->callIndirect(_wasmData.heartbeatCallback, 2, args);

		_env->getInstance()->removeHandle(hostHandle);
	}
}


}
#endif
