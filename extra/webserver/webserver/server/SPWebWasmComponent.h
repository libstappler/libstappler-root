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

#ifndef EXTRA_WEBSERVER_WEBSERVER_SERVER_SPWEBWASMCOMPONENT_H_
#define EXTRA_WEBSERVER_WEBSERVER_SERVER_SPWEBWASMCOMPONENT_H_

#include "SPWebHostComponent.h"

#if MODULE_STAPPLER_WASM
#include "SPWasm.h"

namespace STAPPLER_VERSIONIZED stappler::web {

class SP_PUBLIC WasmComponent : public HostComponent {
public:
	struct WasmData {
		uint32_t childInitCallback;
		uint32_t storageInitCallback;
		uint32_t heartbeatCallback;
		uint32_t userdata;
	};

	static WasmComponent *load(const Host &serv, const HostComponentInfo &, wasm::Module *);

	WasmComponent(const Host &serv, const HostComponentInfo &, wasm::ExecEnv *, WasmData &&);

	virtual void handleChildInit(const Host &) override;
	virtual void handleStorageInit(const Host &, const db::Adapter &) override;
	virtual void initTransaction(db::Transaction &) override;
	virtual void handleHeartbeat(const Host &) override;

protected:
	Rc<wasm::ExecEnv> _env;
	WasmData _wasmData;
	Mutex _mutex;
};

}

#endif

#endif /* EXTRA_WEBSERVER_WEBSERVER_SERVER_SPWEBWASMCOMPONENT_H_ */
