/**
Copyright (c) 2017 Roman Katuntsev <sbkarr@stappler.org>
Copyright (c) 2023 Stappler LLC <admin@stappler.dev>

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
#include "SPBitmap.h"

#if MODULE_STAPPLER_DATA
#include "SPData.h"
#endif

#include "XLVkPlatform.h"
#include "XLVkLoop.h"
#include "XLCoreFrameRequest.h"
#include "XLCoreFrameQueue.h"
#include "XLMainLoop.h"

#include "../xenolith-cli/src/Queue.h"

static constexpr auto HELP_STRING(
R"HelpString(sptest <options> <test-name|all>
Options are one of:
    -v (--verbose)
    -h (--help))HelpString");


namespace stappler::xenolith::test {

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

static void run() {
	// create graphics instance
	auto instance = vk::platform::createInstance([] (vk::platform::VulkanInstanceData &data, const vk::platform::VulkanInstanceInfo &info) {
		data.applicationName = StringView("xenolith-cli");
		data.applicationVersion = XL_MAKE_API_VERSION(0, 0, 1, 0);
		return true;
	});

	// create main looper
	auto mainLoop = Rc<MainLoop>::create("MainLoop", MainLoopInfo{
		.instance = instance,
		.updateCallback = [] (const MainLoop &loop, const UpdateTime &time) {
			if (time.app == 0) {

			}
		}
	});

	// define device selector/initializer
	auto data = Rc<vk::LoopData>::alloc();
	data->deviceSupportCallback = [] (const vk::DeviceInfo &dev) {
		return dev.requiredExtensionsExists && dev.requiredFeaturesExists;
	};

	core::LoopInfo info;
	info.platformData = data;

	// run main loop with 2 additional threads and 0.5sec update interval
	mainLoop->run(move(info), 2, TimeInterval::microseconds(500000));
}

SP_EXTERN_C int _spMain(argc, argv) {
#if MODULE_STAPPLER_DATA
	Value opts = data::parseCommandLineOptions<Interface>(argc, argv,
			&parseOptionSwitch, &parseOptionString);
	if (opts.getBool("help")) {
		std::cout << HELP_STRING << "\n";
		return 0;
	}

	if (opts.getBool("verbose")) {
#if MODULE_STAPPLER_FILESYSTEM
		std::cout << " Current work dir: " << stappler::filesystem::currentDir<Interface>() << "\n";
		std::cout << " Documents dir: " << stappler::filesystem::documentsPathReadOnly<Interface>() << "\n";
		std::cout << " Cache dir: " << stappler::filesystem::cachesPathReadOnly<Interface>() << "\n";
		std::cout << " Writable dir: " << stappler::filesystem::writablePathReadOnly<Interface>() << "\n";
#endif
		std::cout << " Options: " << stappler::data::EncodeFormat::Pretty << opts << "\n";
	}
#endif

	auto mempool = memory::pool::create();
	memory::pool::push(mempool);

	run();

	memory::pool::pop();

	return 0;
}

}
