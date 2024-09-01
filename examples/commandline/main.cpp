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

// Для выбора подсистемы памяти
#include "SPMemory.h"

// Для функций чтения командной строки
#include "SPData.h"

// Для функций генерации пароля
#include "SPValid.h"

#include "SPCrypto.h"

namespace stappler::app {

// Выбираем стандартную подсистему памяти для текущего пространства имён
using namespace mem_std;

// Длина пароля по умолчанию
static constexpr size_t DEFAULT_LENGTH = 0;

// Строка при запросе помощи по команде
static constexpr auto HELP_STRING(
R"HelpString(genpasswd <options> - generates password, at least one number, uppercase and lowercase char
Options are one of:
	-l<n> (--length <n>) - length for password in [6-256] (default: 6)
	-v (--verbose)
	-h (--help))HelpString");

// Разбор коротких переключателей (-h, -v)
static int parseOptionSwitch(Value &ret, char c, const char *str) {
	if (c == 'h') {
		ret.setBool(true, "help");
	} else if (c == 'v') {
		ret.setBool(true, "verbose");
	} else if (c == 'l') {
		// читаем дополнительные числовые символы из строки
		StringView r(str);

		// читаем число из строки
		auto result = r.readInteger(10);

		// при успехе записываем в данные
		result.unwrap([&] (int64_t length) {
			ret.setInteger(length, "length");
		});

		// возвращаем число прочитанных символов включая исходный
		return r.data() - str + 1;
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
	} else if (str.starts_with("length=")) {
		// читаем из одного параметра

		// выделяем вторую часть параметра и читаем число
		auto length = str.sub("length="_len)
				.readInteger(10).get(DEFAULT_LENGTH);

		ret.setInteger(length, "length");
	} else if (str == "length") {
		// читаем из лвух параметров
		if (argc > 0) {
			// читаем число, либо значение по умолчанию
			auto length = StringView(argv[0]).readInteger(10).get(DEFAULT_LENGTH);

			ret.setInteger(length, "length");

			// прочитано два параметра
			return 2;
		}
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

	// читаем запрошенную длину из параметров
	size_t length = math::clamp<size_t>(opts.first.getInteger("length", DEFAULT_LENGTH), valid::MIN_GENPASSWORD_LENGTH, 256);

	// выполняем в контексте временного пула памяти
	// пример не использует подсистему пулов памяти, но всегда заворачивать выполнение основного
	// потока во временный пул памяти - практика, позволяющая избегать ошибок
	perform_temporary([&] {
		// генерируем и выводим пароль
		std::cout << valid::generatePassword<Interface>(length) << "\n";

		crypto::PrivateKey pkey;
		pkey.generate(crypto::KeyType::GOST3410_2012_512);
		pkey.exportPem([] (BytesView data) {
			std::cout << data.toStringView() << "\n";
		});
	});

	return 0;
}

}
