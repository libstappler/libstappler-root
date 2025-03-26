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
#include "SPPlatform.h"
#include "SPTime.h"
#include "SPData.h"
#include "SPCommandLineParser.h"
#include "SPEventLooper.h"
#include "SPEventHandle.h"
#include "SPEventFileHandle.h"
#include "SPEventTimerHandle.h"
#include "SPEventThreadHandle.h"

namespace STAPPLER_VERSIONIZED stappler::app {

using namespace mem_std;

static constexpr auto HELP_STRING =
R"(eventtest <options> [<action>])";

// Опции для аргументов командной строки
static CommandLineParser<Value> CommandLine({
	CommandLineOption<Value> {
		.patterns = {
			"-v", "--verbose"
		},
		.description = "Produce more verbose output",
		.callback = [] (Value &target, StringView pattern, SpanView<StringView> args) -> bool {
			target.setBool(true, "verbose");
			return true;
		}
	},
	CommandLineOption<Value> {
		.patterns = {
			"-h", "--help"
		},
		.description = StringView("Show help message and exit"),
		.callback = [] (Value &target, StringView pattern, SpanView<StringView> args) -> bool {
			target.setBool(true, "help");
			return true;
		}
	},
	CommandLineOption<Value> {
		.patterns = {
			"-l<#>", "--length <#>"
		},
		.description = StringView("Length for password or key"),
		.callback = [] (Value &target, StringView pattern, SpanView<StringView> args) -> bool {
			// Дублируем StringView, поскольку SpanView запрещает менять аргументы
			auto firstArg = StringView(args.at(0));

			// читаем целое число и записываем значение
			target.setInteger(firstArg.readInteger(10).get(0), "length");
			return true;
		}
	},
});


SP_EXTERN_C int main(int argc, const char *argv[]) {
	memory::pool::initialize();

	Value opts;
	Vector<StringView> args;

	if (!CommandLine.parse(opts, argc, argv, [&] (Value &, StringView arg) {
		args.emplace_back(arg);
	})) {
		std::cerr << "Fail to parse command line arguments\n";
		return -1;
	}

	if (opts.getBool("help")) {
		std::cout << HELP_STRING << "\n";
		CommandLine.describe([&] (StringView str) {
			std::cout << str;
		});
		return 0;
	}

	if (opts.getBool("verbose")) {
		std::cerr << " Current work dir: " << stappler::filesystem::currentDir<Interface>() << "\n";
		std::cerr << " Documents dir: " << stappler::filesystem::documentsPathReadOnly<Interface>() << "\n";
		std::cerr << " Cache dir: " << stappler::filesystem::cachesPathReadOnly<Interface>() << "\n";
		std::cerr << " Writable dir: " << stappler::filesystem::writablePathReadOnly<Interface>() << "\n";
		std::cerr << " Options: " << stappler::data::EncodeFormat::Pretty << opts << "\n";
		if (!args.empty()) {
			std::cerr << " Arguments: \n";
			for (auto &it : args) {
				std::cerr << "\t" << it << "\n";
			}
		}
	}

	auto looper = event::Looper::acquire();
	if (!looper) {
		return -1;
	}

	auto c = platform::clock();

	auto handle = looper->schedule(TimeInterval::seconds(10), [c] (event::Handle *, bool success) {
		auto t = platform::clock() - c;
		std::cout << platform::clock(ClockType::Realtime) - c << " " << t / 1000000 << "\n";
		log::debug("App", "Fn timer: ", success);
	});

	/*(void)looper->scheduleTimer(event::TimerInfo{
		.completion = event::TimerInfo::Completion::create<void>(handle.get(),
				[] (void *data, event::TimerHandle *self, uint32_t value, Status status) {
			log::debug("App", "Timer1: ", value, " ", status);
			if (status != Status::Ok) {
				log::debug("App", "Timer1 ended: ", value, " ", status);
			}

			if (value == 2) {
				//((event::Handle *)data)->cancel();
			}

			if (value == 3) {
				event::Looper::acquire()->wakeup(event::QueueWakeupInfo{
					event::WakeupFlags::Graceful
				});
			}
		}),
		.interval = TimeInterval::seconds(1),
		.count = 100,
	});*/

	(void)looper->scheduleTimer(event::TimerInfo{
		.completion = event::TimerInfo::Completion::create<void>(nullptr,
				[] (void *data, event::TimerHandle *self, uint32_t value, Status status) {
			log::debug("App", "Timer2: ", value, " ", status);
		}),
		.interval = TimeInterval::seconds(1),
		.count = 50,
	});

	std::thread thread([] (event::Looper *looper) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		looper->performOnThread([] {
			log::debug("App", "From thread");
		}, nullptr);
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
		looper->performOnThread([] {
			log::debug("App", "From thread");
		}, nullptr);
	}, looper);

	std::thread thread2([] (event::Looper *looper) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		looper->performOnThread([] {
			log::debug("App", "From thread2");
		}, nullptr);
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
		looper->performOnThread([] {
			log::debug("App", "From thread2");
		}, nullptr);
	}, looper);

	auto status = looper->run();

	std::cout << "Wakeup: " << status << "\n";

	status = looper->run();

	thread.join();
	thread2.join();

	struct AppData {
		uint32_t timerTicks = 0;
		Rc<event::TimerHandle> timer1;
		Rc<event::TimerHandle> timer2;
		Rc<event::QueueRef> queue;
		Rc<event::DirHandle> dir;
		Rc<event::DirHandle> dir2;
		Rc<event::StatHandle> stat;
		Rc<event::ThreadHandle> thread;
	};

	auto ret = perform_temporary([&] () -> int {
		AppData data;

		//data.queue = Rc<event::QueueRef>::create(event::QueueInfo(), event::QueueFlags::Protected);

		/*data.timer2 = data.queue->scheduleTimer(event::TimerInfo{
			.completion = event::TimerInfo::Completion::create<AppData>(&data,
					[] (AppData *data, event::TimerHandle *self, uint32_t value, event::Status status) {
				if (!event::isSuccessful(status)) {
					log::debug("App", "Error: ", status);
				} else {
					log::debug("App", "Timer2: ", value);
				}
			}),
			.timeout = TimeInterval::seconds(5),
			.count = uint32_t(10),
		});*/



		/*data.dir = data.queue->openDir(event::OpenDirInfo{
			.completion = event::OpenDirInfo::Completion::create<AppData>(&data,
					[] (AppData *data, event::DirHandle *self, uint32_t value, event::Status err) {
				log::debug("App", "OpenDir: ", self->getPath(), ": ", err);
				self->scan([] (event::FileType type, StringView name) {
					log::debug("App", "scan: (", type, ") ", name);
				});
			}),
			.file = event::FileOpInfo{
				.path = StringView("/home/sbkarr")
			}
		});

		data.dir2 = data.queue->openDir(event::OpenDirInfo{
			.completion = event::OpenDirInfo::Completion::create<AppData>(&data,
					[] (AppData *data, event::DirHandle *self, uint32_t value, event::Status err) {
				log::debug("App", "OpenDir2: ", self->getPath(), ": ", err);
				self->scan([] (event::FileType type, StringView name) {
					log::debug("App", "scan2: (", type, ") ", name);
				});
			}),
			.file = event::FileOpInfo{
				.root = data.dir,
				.path = StringView("videos")
			}
		});

		data.stat = data.queue->stat(event::StatOpInfo{
			.completion = event::StatOpInfo::Completion::create<AppData>(&data,
					[] (AppData *data, event::StatHandle *self, uint32_t value, event::Status err) {
				log::debug("App", "Stat: ", self->getPath(), ": ", self->getStat());
			}),
			.file = event::FileOpInfo{
				.path = StringView("/home/sbkarr/image1076.png")
			}
		});*/

		data.thread = data.queue->addThreadHandle();

		return 0;
	});

	memory::pool::terminate();
	return ret;
}

}
