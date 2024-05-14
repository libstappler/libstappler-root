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

#include "Test.h"
#include "SPWasm.h"

namespace STAPPLER_VERSIONIZED stappler::app::test {

struct WesmTest : Test {
	WesmTest() : Test("WesmTest") { }

	virtual bool run() override {
		using namespace stappler::wasm;

		auto mempool = memory::pool::create();
		memory::pool::push(mempool);

		auto mod = Rc<Module>::create("stappler:wasm/app", FilePath(filesystem::currentDir<Interface>("stappler-build/host/wasm/clang/debug/app.wasm")));
		if (!mod) {
			return -1;
		}

		auto inst = Rc<ModuleInstance>::create(mod);
		if (!inst) {
			return -1;
		}

		auto env = Rc<ExecEnv>::create(inst);
		if (!env) {
			return -1;
		}

		auto fn = inst->lookup("run");
		auto res = fn->call1(env);
		if (res.of.i32) {
			printf("GREAT! PASS ALL CHKs\n");
		}

		memory::pool::pop();

		return true;
	}
} _WesmTest;

}
