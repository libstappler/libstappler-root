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

#include "SPMemPoolInterface.h"
#include "XLCommon.h"
#include "SPData.h"
#include "ExampleApplication.h"

namespace stappler::xenolith::app {

static constexpr auto HELP_STRING(
R"HelpString(testapp <options>)HelpString");

SP_EXTERN_C int main(int argc, const char *argv[]) {
	ApplicationInfo data = ApplicationInfo::readFromCommandLine(argc, argv);

	if (data.help) {
		std::cout << HELP_STRING << "\n";
		ApplicationInfo::CommandLine.describe([&] (StringView str) {
			std::cout << str;
		});
		return 0;
	}

	if (data.verbose) {
		std::cout << " Current work dir: " << stappler::filesystem::currentDir<Interface>() << "\n";
		std::cout << " Documents dir: " << stappler::filesystem::documentsPathReadOnly<Interface>() << "\n";
		std::cout << " Cache dir: " << stappler::filesystem::cachesPathReadOnly<Interface>() << "\n";
		std::cout << " Writable dir: " << stappler::filesystem::writablePathReadOnly<Interface>() << "\n";
		std::cout << " Options: " << stappler::data::EncodeFormat::Pretty << data.encode() << "\n";
	}

	// Выполняем все действия во временном пуле памяти
	memory::pool::perform_temporary([&] {
		// Создаём приложение на основании данных командной строки
		auto app = Rc<ExampleApplication>::create(move(data));

		// Инициализируем приложение
		app->run();

		// Здесь может располагаться инициализация дополнительных инструментов ОС

		// Ожидаем завершения работы приложения
		app->waitStopped();
	});

	return 0;
}

}
