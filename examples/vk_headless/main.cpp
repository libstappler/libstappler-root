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

#include "SPCommon.h"
#include "SPValid.h"
#include "NoiseQueue.h"

namespace stappler::xenolith::app {

// Строка при запросе помощи по команде
static constexpr auto HELP_STRING(
R"HelpString(noisegen <options> <filename> - generates noise image
Options are one of:
	--dx <number> - seed for X
	--dy <number> - seed for Y
	--width <number> - image width
	--height <number> - image height
	-R (--random) - use random seed
	-v (--verbose)
	-h (--help))HelpString");

// Разбор коротких переключателей (-h, -v)
static int parseOptionSwitch(Value &ret, char c, const char *str) {
	if (c == 'h') {
		ret.setBool(true, "help");
	} else if (c == 'v') {
		ret.setBool(true, "verbose");
	} else if (c == 'R') {
		ret.setBool(true, "random");
	}

	// прочитан только один символ
	return 1;
}

// Разбор строковых переключателей (--help, -verbose)
static int parseOptionString(Value &ret, const StringView &str, int argc, const char * argv[]) {
	if (str == "help") {
		ret.setBool(true, "help");
	} else if (str == "verbose") {
		ret.setBool(true, "verbose");
	} else if (str == "random") {
		ret.setBool(true, "random");
	} else if (str == "dx" && argc > 0) {
		ret.setInteger(StringView(argv[0]).readInteger(10).get(0), "dx");
		return 2;
	} else if (str == "dx" && argc > 0) {
		ret.setInteger(StringView(argv[0]).readInteger(10).get(0), "dx");
		return 2;
	} else if (str == "width" && argc > 0) {
		ret.setInteger(StringView(argv[0]).readInteger(10).get(0), "width");
		return 2;
	} else if (str == "height" && argc > 0) {
		ret.setInteger(StringView(argv[0]).readInteger(10).get(0), "height");
		return 2;
	}

	// прочитан один параметр
	return 1;
}

SP_EXTERN_C int main(int argc, const char *argv[]) {
	// читаем параметры командной строки

	// возвращает дополнительные параметры и список основных аргументов
	Pair<Value, Vector<String>> opts = data::parseCommandLineOptions<Interface, Value>(
			argc, argv, &parseOptionSwitch, &parseOptionString);

	// проверяем, запрошена ли помощь
	if (opts.first.getBool("help")) {
		std::cout << HELP_STRING << "\n";
		return 0;
	}

	// выводим подробности об окружении, если запрошен флаг подробностей
	if (opts.first.getBool("verbose")) {
		std::cerr << " Current work dir: " << stappler::filesystem::currentDir<Interface>() << "\n";
		std::cerr << " Documents dir: " << stappler::filesystem::documentsPathReadOnly<Interface>() << "\n";
		std::cerr << " Cache dir: " << stappler::filesystem::cachesPathReadOnly<Interface>() << "\n";
		std::cerr << " Writable dir: " << stappler::filesystem::writablePathReadOnly<Interface>() << "\n";
		std::cerr << " Options: " << stappler::data::EncodeFormat::Pretty << opts.first << "\n";
		std::cerr << " Arguments: \n";
		for (auto &it : opts.second) {
			std::cerr << "\t" << it << "\n";
		}
	}

	// Проверяем, что аргумент файла результата передан
	if (opts.second.size() != 2) {
		std::cerr << "<filename> is missed\n";
		return -2;
	}

	bool success = true;

	// Конвертируем в путь от текущей рабочей директории
	auto path = opts.second.at(1);
	if (path[0] != '/') {
		path = filesystem::currentDir<Interface>(path);
	}

	// выполняем в контексте временного пула памяти
	perform_temporary([&] {
		// Размерность изображения
		Extent2 extent(1024, 768);

		// Данные для генерации шума - зерно и плотность пикселей
		NoiseData data{0, 0, 1.0f, 1.0f};

		// Проверяем, запрошено ли случайное зерно
		if (opts.first.getBool("random")) {
			valid::makeRandomBytes((uint8_t *)&data, sizeof(uint32_t) * 2);
		}

		// Читаем значения зерна
		if (opts.first.isInteger("dx")) {
			data.seedX = opts.first.getInteger("dx");
		}

		if (opts.first.isInteger("dx")) {
			data.seedY = opts.first.getInteger("dx");
		}

		// Читаем значения размерности
		if (opts.first.isInteger("width")) {
			extent.width = opts.first.getInteger("width");
		}

		if (opts.first.isInteger("height")) {
			extent.height = opts.first.getInteger("height");
		}

		// Запускаем генератор
		success = NoiseQueue::run(path, data, extent);
	});

	return success ? 0 : -1;
}

}
