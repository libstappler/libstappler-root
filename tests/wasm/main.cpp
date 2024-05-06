/**
Copyright (c) 2025 Stappler LLC <admin@stappler.dev>

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

#include "SPCommon.h"
#include "SPMemory.h"
#include "SPTime.h"
#include "SPData.h"
#include "SPString.h"
#include "SPWasm.h"

static constexpr auto HELP_STRING(
R"HelpString(sptest <options> <test-name|all>
Options are one of:
    -v (--verbose)
    -h (--help))HelpString");


namespace stappler::wasm {

#if MODULE_STAPPLER_DATA
int parseOptionSwitch(Value &ret, char c, const char *str) {
	if (c == 'h') {
		ret.setBool(true, "help");
	} else if (c == 'v') {
		ret.setBool(true, "verbose");
	}
	return 1;
}

int parseOptionString(Value &ret, const StringView &str, int argc, const char * argv[]) {
	if (str == "help") {
		ret.setBool(true, "help");
	} else if (str == "verbose") {
		ret.setBool(true, "verbose");
	} else if (str == "gencbor") {
		ret.setBool(true, "gencbor");
	}
	return 1;
}
#endif

SP_EXTERN_C int main(int argc, const char *argv[]) {
#if MODULE_STAPPLER_DATA
	auto opts = data::parseCommandLineOptions<Interface, Value>(argc, argv,
			&parseOptionSwitch, &parseOptionString);
	if (opts.first.getBool("help")) {
		std::cout << HELP_STRING << "\n";
		return 0;
	}

	if (opts.first.getBool("verbose")) {
#if MODULE_STAPPLER_FILESYSTEM
		std::cout << " Current work dir: " << stappler::filesystem::currentDir<Interface>() << "\n";
		std::cout << " Documents dir: " << stappler::filesystem::documentsPathReadOnly<Interface>() << "\n";
		std::cout << " Cache dir: " << stappler::filesystem::cachesPathReadOnly<Interface>() << "\n";
		std::cout << " Writable dir: " << stappler::filesystem::writablePathReadOnly<Interface>() << "\n";
#endif
		std::cout << " Options: " << stappler::data::EncodeFormat::Pretty << opts.first << "\n";
		std::cout << " Arguments: \n";
		for (auto &it : opts.second) {
			std::cout << "\t" << it << "\n";
		}
	}
#endif

	auto mempool = memory::pool::create();
	memory::pool::context memCtx(mempool);

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

#if 0
	/* lookup function instance */
	if (!(wasm_cmp_externref_ptr = wasm_runtime_lookup_function(wasm_module_inst, "cmp-externref", NULL))) {
		printf("%s\n", "lookup function cmp-externref failed");
		return fail();
	}

	if (!(wasm_get_externref_ptr = wasm_runtime_lookup_function(wasm_module_inst, "get-externref", NULL))) {
		printf("%s\n", "lookup function get-externref failed");
		return fail();
	}

	if (!(wasm_set_externref_ptr = wasm_runtime_lookup_function(wasm_module_inst, "set-externref", NULL))) {
		printf("%s\n", "lookup function set-externref failed");
		return fail();
	}

	/* test with NULL */
	if (!set_and_cmp(exec_env, wasm_module_inst, 0, 0)
			|| !set_and_cmp(exec_env, wasm_module_inst, 1, big_number + 1)
			|| !set_and_cmp(exec_env, wasm_module_inst, 2, big_number + 2)
			|| !set_and_cmp(exec_env, wasm_module_inst, 3, big_number + 3)) {
		return fail();
	}
#endif

	return 0;
}

}
