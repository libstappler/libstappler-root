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

// Для выбора подсистемы памяти
#include "SPMemory.h"

// Для функций чтения командной строки
#include "SPData.h"

// Для функций генерации пароля
#include "SPValid.h"

// Функции криптографии
#include "SPCrypto.h"

// Парсер опций командной строки
#include "SPCommandLineParser.h"

namespace stappler::app {

// Выбираем стандартную подсистему памяти для текущего пространства имён
using namespace mem_std;

// Длина пароля по умолчанию
static constexpr size_t DEFAULT_PASSWORD_LENGTH = 6;
static constexpr size_t DEFAULT_KEY_LENGTH = 256;

// Строка при запросе помощи по команде
static constexpr auto HELP_STRING =
R"(genpasswd <options> [<action>]
Actions:
	genpassword (default) - generates password, at least one number, uppercase and lowercase char
	genkey - generates private key with GOST3410_2012 algorithm
)";

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

	// проверяем, запрошена ли помощь
	if (opts.getBool("help")) {
		std::cout << HELP_STRING << "\n";
		CommandLine.describe([&] (StringView str) {
			std::cout << str;
		});
		return 0;
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

	// выполняем в контексте временного пула памяти
	// пример не использует подсистему пулов памяти, но всегда заворачивать выполнение основного
	// потока во временный пул памяти - практика, позволяющая избегать ошибок
	return perform_temporary([&] () -> int {
		if (args.size() < 2 || args.at(1) == "genpassword") {
			auto length = size_t(opts.getInteger("length", DEFAULT_PASSWORD_LENGTH));

			if (length < valid::MIN_GENPASSWORD_LENGTH || length > 256) {
				std::cerr << "Length should be in range [" << valid::MIN_GENPASSWORD_LENGTH << "-256]\n";
				return -1;
			}

			// генерируем и выводим пароль
			std::cout << valid::generatePassword<Interface>(length) << "\n";
		} else if (args.at(1) == "genkey") {
			// читаем запрошенную длину из параметров
			auto length = opts.getInteger("length", DEFAULT_KEY_LENGTH);

			crypto::KeyType keyType = crypto::KeyType::GOST3410_2012_256;

			switch (length) {
			case 256:
				keyType = crypto::KeyType::GOST3410_2012_256;
				break;
			case 512:
				keyType = crypto::KeyType::GOST3410_2012_512;
				break;
			default:
				std::cerr << "Length should 256 or 512\n";
				return -1;
			}

			crypto::PrivateKey pkey;
			pkey.generate(keyType);
			pkey.exportPem([] (BytesView data) {
				std::cout << data.toStringView() << "\n";
			});
		}
		return 0;
	});
}

}
