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

#include "ExampleDb.h"

namespace stappler::app {

// Строка при запросе помощи по команде
static constexpr auto HELP_STRING(
R"HelpString(dbconnect <options> - generates password
Options are one of:
	--dbname <name> - DB name or path
	--user <name> - username for postgresql
	--password <name> - password for postgresql
	--host <name> - hostname for postgresql
	-S (--sqlite) - force to use sqlite driver
	-P (--pgsql) - force to use postgresql driver
	-v (--verbose)
	-h (--help))HelpString");

// Разбор коротких переключателей (-h, -v)
static int parseOptionSwitch(Value &ret, char c, const char *str) {
	if (c == 'h') {
		ret.setBool(true, "help");
	} else if (c == 'v') {
		ret.setBool(true, "verbose");
	} else if (c == 'S') {
		ret.setString("sqlite", "driver");
	} else if (c == 'P') {
		ret.setString("pgsql", "driver");
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
	} else if (str == "sqlite") {
		ret.setBool("sqlite", "driver");
	} else if (str == "pgsql") {
		ret.setString("pgsql", "driver");
	} else if (str == "dbname" && argc > 0) {
		ret.setString(StringView(argv[0]), "dbname");
		return 2;
	} else if (str == "user" && argc > 0) {
		ret.setString(StringView(argv[0]), "user");
		return 2;
	} else if (str == "password" && argc > 0) {
		ret.setString(StringView(argv[0]), "password");
		return 2;
	} else if (str == "host" && argc > 0) {
		ret.setString(StringView(argv[0]), "host");
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

	bool success = true;

	perform_temporary([&] {
		success = ExampleDb::run(opts.first, opts.second.size() > 1 ? opts.second.at(1) : StringView());
	});

	return success ? 0 : -1;
}

}
