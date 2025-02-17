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

#include "ExampleDb.h"

// Парсер опций командной строки
#include "SPCommandLineParser.h"

namespace stappler::app {

// Строка при запросе помощи по команде
static constexpr auto HELP_STRING(
R"HelpString(dbconnect <options> [query] - opens simple connection with database and execute query

Example:
	dbconnect -P --user=stappler --password=stappler --dbname=stappler --host=localhost "SELECT version();"
)HelpString");

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
			"-S", "--sqlite"
		},
		.description = StringView("Use SQLite driver"),
		.callback = [] (Value &target, StringView pattern, SpanView<StringView> args) -> bool {
			target.setString("sqlite", "driver");
			return true;
		}
	},
	CommandLineOption<Value> {
		.patterns = {
			"-P", "--pgsql"
		},
		.description = StringView("Use PostgreSQL driver (libpq)"),
		.callback = [] (Value &target, StringView pattern, SpanView<StringView> args) -> bool {
			target.setString("pgsql", "driver");
			return true;
		}
	},
	CommandLineOption<Value> {
		.patterns = {
			"-D<name>", "--dbname <name>"
		},
		.description = StringView("Target database name or path for SQLite"),
		.callback = [] (Value &target, StringView pattern, SpanView<StringView> args) -> bool {
			target.setString(args[0], "dbname");
			return true;
		}
	},
	CommandLineOption<Value> {
		.patterns = {
			"-U<username>", "--user <username>"
		},
		.description = StringView("Username for database connection"),
		.callback = [] (Value &target, StringView pattern, SpanView<StringView> args) -> bool {
			target.setString(args[0], "user");
			return true;
		}
	},
	CommandLineOption<Value> {
		.patterns = {
			"-W<password>", "--password <password>"
		},
		.description = StringView("Password for database connection"),
		.callback = [] (Value &target, StringView pattern, SpanView<StringView> args) -> bool {
			target.setString(args[0], "password");
			return true;
		}
	},
	CommandLineOption<Value> {
		.patterns = {
			"-H<hostname>", "--host <hostname>"
		},
		.description = StringView("Database host for connection"),
		.callback = [] (Value &target, StringView pattern, SpanView<StringView> args) -> bool {
			target.setString(args[0], "host");
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

	bool success = true;

	// выполняем в контексте временного пула памяти
	perform_temporary([&] {
		// соединяемся с БД
		success = ExampleDb::run(opts,
				// Так можно передать команду на исполнение серверу аргументом, сейчас не используется
				args.size() > 1 ? args.at(1) : StringView());
	});

	return success ? 0 : -1;
}

}
