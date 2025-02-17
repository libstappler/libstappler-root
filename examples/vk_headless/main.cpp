/**
 Copyright (c) 2024-2025 Stappler LLC <admin@stappler.dev>

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
#include "SPValid.h"
#include "SPCommandLineParser.h"
#include "NoiseQueue.h"

namespace stappler::xenolith::app {

// Строка при запросе помощи по команде
static constexpr auto HELP_STRING(
R"HelpString(noisegen <options> <filename> - generates noise image)HelpString");

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
			"-q", "--quiet"
		},
		.description = StringView("Disable verbose Vulkan output"),
		.callback = [] (Value &target, StringView pattern, SpanView<StringView> args) -> bool {
			target.setBool(true, "quiet");
			return true;
		}
	},
	CommandLineOption<Value> {
		.patterns = {
			"-R", "--random"
		},
		.description = StringView("Use random seed"),
		.callback = [] (Value &target, StringView pattern, SpanView<StringView> args) -> bool {
			target.setBool(true, "random");
			return true;
		}
	},
	CommandLineOption<Value> {
		.patterns = {
			"-X<#>", "--dx <#>"
		},
		.description = StringView("Seed for X"),
		.callback = [] (Value &target, StringView pattern, SpanView<StringView> args) -> bool {
			target.setInteger(StringView(args[0]).readInteger(10).get(0), "dx");
			return true;
		}
	},
	CommandLineOption<Value> {
		.patterns = {
			"-Y<#>", "--dy <#>"
		},
		.description = StringView("Seed for Y"),
		.callback = [] (Value &target, StringView pattern, SpanView<StringView> args) -> bool {
			target.setInteger(StringView(args[0]).readInteger(10).get(0), "dy");
			return true;
		}
	},
	CommandLineOption<Value> {
		.patterns = {
			"-W<#>", "--width <#>"
		},
		.description = StringView("Image width"),
		.callback = [] (Value &target, StringView pattern, SpanView<StringView> args) -> bool {
			target.setInteger(StringView(args[0]).readInteger(10).get(0), "width");
			return true;
		}
	},
	CommandLineOption<Value> {
		.patterns = {
			"-H<#>", "--height <#>"
		},
		.description = StringView("Image height"),
		.callback = [] (Value &target, StringView pattern, SpanView<StringView> args) -> bool {
			target.setInteger(StringView(args[0]).readInteger(10).get(0), "height");
			return true;
		}
	},
});

SP_EXTERN_C int main(int argc, const char *argv[]) {
	Value opts;
	Vector<StringView> args;

	// читаем параметры командной строки
	if (!CommandLine.parse(opts, argc, argv, [&] (Value &, StringView arg) {
		// записываем аргумент в список
		args.emplace_back(arg);
	})) {
		std::cerr << "Fail to parse command line arguments\n";
		return -1;
	}

	// выводим подробности об окружении, если запрошен флаг подробностей
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

	// проверяем, запрошена ли помощь
	if (opts.getBool("help")) {
		std::cout << HELP_STRING << "\n";
		CommandLine.describe([&] (StringView str) {
			std::cout << str;
		});
		return 0;
	}

	// Проверяем, что аргумент файла результата передан
	if (args.size() != 2) {
		std::cerr << "<filename> is missed\n";
		return -2;
	}

	if (opts.getBool("quiet")) {
		log::setLogFilterMask(log::ErrorsOnly);
	}

	bool success = true;

	// выполняем в контексте временного пула памяти
	perform_temporary([&] {
		// Конвертируем в путь от текущей рабочей директории
		auto path = args.at(1);
		if (path[0] != '/') {
			// дублируем память из строки, в противном случае это будет dangling reference
			path = StringView(filesystem::currentDir<Interface>(path)).pdup();
		}

		// Размерность изображения
		Extent2 extent(1024, 768);

		// Данные для генерации шума - зерно и плотность пикселей
		NoiseData data{0, 0, 1.0f, 1.0f};

		// Проверяем, запрошено ли случайное зерно
		if (opts.getBool("random")) {
			valid::makeRandomBytes((uint8_t *)&data, sizeof(uint32_t) * 2);
		}

		// Читаем значения зерна
		if (opts.isInteger("dx")) {
			data.seedX = opts.getInteger("dx");
		}

		if (opts.isInteger("dx")) {
			data.seedY = opts.getInteger("dx");
		}

		// Читаем значения размерности
		if (opts.isInteger("width")) {
			extent.width = opts.getInteger("width");
		}

		if (opts.isInteger("height")) {
			extent.height = opts.getInteger("height");
		}

		// Запускаем генератор
		success = NoiseQueue::run(path, data, extent);
	});

	return success ? 0 : -1;
}

}
