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

#include "wamr/wasm_export.h"

static constexpr auto HELP_STRING(
R"HelpString(sptest <options> <test-name|all>
Options are one of:
    -v (--verbose)
    -h (--help))HelpString");


namespace stappler::wasm {

using namespace stappler::mem_std;

static uintptr_t global_objects[10] = { 0 };

int32_t local_cmp_externref(wasm_exec_env_t exec_env, uintptr_t externref_a, uintptr_t externref_b) {
	return externref_a == externref_b;
}

int32_t local_chk_externref(wasm_exec_env_t exec_env, int32_t index, uintptr_t externref) {
	return externref == global_objects[index];
}

/* clang-format off */
static NativeSymbol native_symbols[] = {
		{ "native-cmp-externref", (void *)&local_cmp_externref, "(rr)i", NULL },
		{ "native-chk-externref", (void *)&local_chk_externref, "(ir)i", NULL }, };
/* clang-format on */

static inline void local_set_externref(int32_t index, uintptr_t externref) {
	global_objects[index] = externref;
}

static WASMFunctionInstanceCommon *wasm_set_externref_ptr;
static WASMFunctionInstanceCommon *wasm_get_externref_ptr;
static WASMFunctionInstanceCommon *wasm_cmp_externref_ptr;

static bool wasm_set_externref(wasm_exec_env_t exec_env, wasm_module_inst_t inst, int32_t index, uintptr_t externref) {
	union {
		uintptr_t val;
		uint32_t parts[2];
	} u;
	uint32_t argv[3] = { 0 };

	if (!exec_env || !wasm_set_externref_ptr) {
		return false;
	}

	u.val = externref;
	argv[0] = index;
	argv[1] = u.parts[0];
	argv[2] = u.parts[1];
	if (!wasm_runtime_call_wasm(exec_env, wasm_set_externref_ptr, 2, argv)) {
		const char *exception;
		if ((exception = wasm_runtime_get_exception(inst))) {
			printf("Exception: %s\n", exception);
		}
		return false;
	}

	return true;
}

static bool wasm_get_externref(wasm_exec_env_t exec_env, wasm_module_inst_t inst, int32_t index, uintptr_t *ret_externref) {
	wasm_val_t results[1] = { 0 };

	if (!exec_env || !wasm_get_externref_ptr || !ret_externref) {
		return false;
	}

	if (!wasm_runtime_call_wasm_v(exec_env, wasm_get_externref_ptr, 1, results, 1, index)) {
		const char *exception;
		if ((exception = wasm_runtime_get_exception(inst))) {
			printf("Exception: %s\n", exception);
		}
		return false;
	}

	if (WASM_ANYREF != results[0].kind) {
		return false;
	}

	*ret_externref = results[0].of.foreign;
	return true;
}

static bool wasm_cmp_externref(wasm_exec_env_t exec_env, wasm_module_inst_t inst, int32_t index, uintptr_t externref, int32_t *ret_result) {
	wasm_val_t results[1] = { 0 };
	wasm_val_t arguments[2];
	arguments[0].kind = WASM_I32;
	arguments[0].of.i32 = index;

	arguments[1].kind = WASM_ANYREF;
	arguments[1].of.foreign = externref;

	if (!exec_env || !wasm_cmp_externref_ptr || !ret_result) {
		return false;
	}

	if (!wasm_runtime_call_wasm_a(exec_env, wasm_cmp_externref_ptr, 1, results, 2, arguments)) {
		const char *exception;
		if ((exception = wasm_runtime_get_exception(inst))) {
			printf("Exception: %s\n", exception);
		}
		return false;
	}

	if (results[0].kind != WASM_I32) {
		return false;
	}

	*ret_result = results[0].of.i32;
	return true;
}

static bool set_and_cmp(wasm_exec_env_t exec_env, wasm_module_inst_t inst, int32_t i, uintptr_t externref) {
	int32_t cmp_result = 0;
	uintptr_t wasm_externref = 0;

	wasm_set_externref(exec_env, inst, i, externref);
	local_set_externref(i, externref);

	wasm_get_externref(exec_env, inst, i, &wasm_externref);
	if (!local_chk_externref(exec_env, i, wasm_externref)) {
		printf("#%d, In host language world Wasm Externref 0x%lx Vs. Native "
				"Externref 0x%lx FAILED\n", i, wasm_externref, externref);
		return false;
	}

	if (!wasm_cmp_externref(exec_env, inst, i, global_objects[i], &cmp_result) || !cmp_result) {
		printf("#%d, In Wasm world Native Externref 0x%lx Vs, Wasm Externref "
				"FAILED\n", i, global_objects[i]);
		return false;
	}

	return true;
}


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

	auto wasmFile = filesystem::readIntoMemory<Interface>(filesystem::currentDir<Interface>("hello.wasm"));

	if (wasmFile.empty()) {
		return -1;
	}

	uint32_t stack_size = 16 * 1024, heap_size = 16 * 1024;
	wasm_module_t wasm_module = NULL;
	wasm_module_inst_t wasm_module_inst = NULL;
	wasm_exec_env_t exec_env = NULL;
	RuntimeInitArgs init_args;
	char error_buf[128] = { 0 };
	const uint64_t big_number = 0x123456789abc;

	auto fail = [&] () -> int {
		/* destroy exec env */
		if (exec_env) {
			wasm_runtime_destroy_exec_env(exec_env);
		}

		/* destroy the module instance */
		if (wasm_module_inst) {
			wasm_runtime_deinstantiate(wasm_module_inst);
		}

		/* unload the module */
		if (wasm_module) {
			wasm_runtime_unload(wasm_module);
		}

		/* destroy runtime environment */
		wasm_runtime_destroy();
		return -1;
	};

	memset(&init_args, 0, sizeof(RuntimeInitArgs));

	init_args.mem_alloc_type = Alloc_With_Allocator;
	init_args.mem_alloc_option.allocator.malloc_func = (void *)&malloc;
	init_args.mem_alloc_option.allocator.realloc_func = (void *)&realloc;
	init_args.mem_alloc_option.allocator.free_func = (void *)&free;

	init_args.n_native_symbols = sizeof(native_symbols) / sizeof(NativeSymbol);
	init_args.native_module_name = "env";
	init_args.native_symbols = native_symbols;

	/* initialize runtime environment */
	if (!wasm_runtime_full_init(&init_args)) {
		printf("Init runtime environment failed.\n");
		return -1;
	}

	/* load WASM module */
	if (!(wasm_module = wasm_runtime_load(wasmFile.data(), wasmFile.size(), error_buf, sizeof(error_buf)))) {
		printf("%s\n", error_buf);
		return fail();
	}

	/* instantiate the module */
	if (!(wasm_module_inst = wasm_runtime_instantiate(wasm_module, stack_size, heap_size, error_buf, sizeof(error_buf)))) {
		printf("%s\n", error_buf);
		return fail();
	}

	/* create an execution env */
	if (!(exec_env = wasm_runtime_create_exec_env(wasm_module_inst, stack_size))) {
		printf("%s\n", "create exec env failed");
		return fail();
	}

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

	printf("GREAT! PASS ALL CHKs\n");

	return 0;
}

}
