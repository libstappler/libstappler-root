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

#include "XLCommon.h"
#include "SPData.h"
#include "ExampleApplication.h"

namespace stappler::xenolith::app {

static constexpr auto HELP_STRING(
R"HelpString(testapp <options>
Options are one of:
	--w=<initial screen width in pixels>
	--h=<initial screen height in pixels>
	--d=<pixel density>
	--l=<application locale code>
	--bundle=<application bundle name>
	--renderdoc - try to connect with renderdoc capture layers
	--novalidation - force-disable vulkan validation
	--decor=<left,top,right,bottom> - view decoration padding in pixels
	-v (--verbose)
	-h (--help))HelpString");

SP_EXTERN_C int main(int argc, const char *argv[]) {
	ApplicationInfo data;
	Vector<String> args;

	// читаем данные командной строки в структуру данных о приложении
	data::parseCommandLineOptions<Interface, ApplicationInfo>(data, argc, argv,
			[&] (ApplicationInfo &, StringView str) {
		args.emplace_back(str.str<Interface>());
	}, &ApplicationInfo::parseCmdSwitch, &ApplicationInfo::parseCmdString);
	if (data.help) {
		std::cout << HELP_STRING << "\n";
		return 0;
	}

	if (data.verbose) {
		std::cout << " Current work dir: " << stappler::filesystem::currentDir<Interface>() << "\n";
		std::cout << " Documents dir: " << stappler::filesystem::documentsPathReadOnly<Interface>() << "\n";
		std::cout << " Cache dir: " << stappler::filesystem::cachesPathReadOnly<Interface>() << "\n";
		std::cout << " Writable dir: " << stappler::filesystem::writablePathReadOnly<Interface>() << "\n";
		std::cout << " Options: " << stappler::data::EncodeFormat::Pretty << data.encode() << "\n";
		std::cout << " Arguments: \n";
		for (auto &it : args) {
			std::cout << "\t" << it << "\n";
		}
	}

	// Выполняем все действия во временном пуле памяти
	memory::pool::perform_temporary([&] {
		// Создаём приложение на основании данных командной строки
		auto app = Rc<ExampleApplication>::create(move(data));

		// Инициализируем приложение
		app->run();

		// Здесь может располагаться инициализация дополнительных инструментов ОС

		// Ожидаем завершения работы приложения
		app->waitFinalized();
	});
	return 0;
}

}
