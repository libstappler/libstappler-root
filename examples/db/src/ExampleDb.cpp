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
#include "SPDbSimpleServer.h"
#include "SPSqlHandle.h"

#include "ExampleDb.h"

namespace STAPPLER_VERSIONIZED stappler::app {

static void ExampleDb_performQuery(db::sql::SqlHandle *iface, StringView query) {
	iface->performSimpleSelect(query, [] (db::Result &res) {
		if (res.success()) {
			std::cout << "Query: success;"
					" rows affected: " << res.getAffectedRows() << ";";
			if (!res.empty()) {
				// Обратите внимание, что для драйвера с последовательным чтением курсора (sqlite) значение будет некорректным
				std::cout << " rows in result: " << res.nrows() << ";";
			}
			std::cout << "\n";

			if (!res.empty()) {
				// Обходим имена полей
				std::cout << "Fields: ";
				for (size_t i = 0; i < res.nfields(); ++ i) {
					if (i != 0) {
						std::cout << "\t";
					}
					std::cout << "\"" << res.name(i) << "\"";
				}
				std::cout << "\n";

				size_t rowNumber = 0;
				// Обходим результат по курсору
				for (auto row : res) {
					std::cout << "[" << rowNumber << "]";
					for (size_t i = 0; i < res.nfields(); ++ i) {
						// Вместо выяснения типа поля и затем чтения типизированного значения
						// можно сразу извлечь нетипизированное значение

						// row.toTypedData использует аннотации, переданные драйвером БД, для декодирования значения
						// row.toData в свою очередь использует схему данных на стороне приложения для декодирования
						std::cout << "\t" << data::EncodeFormat::Json << row.toTypedData(i);
					}
					std::cout << "\n";
					++ rowNumber;
				}
			}

		} else {
			std::cout << "Query: failed\n";
		}
	}, [] (const Value &err) {
		log::error("ExampleDb", "Query error: ", data::EncodeFormat::Pretty, err);
	});
}

bool ExampleDb::run(const Value &val, StringView command) {
	bool isSqlite = true;
	db::Value data;
	if (val.isString("driver")) {
		if (val.getString("driver") != "sqlite") {
			isSqlite = false;
		}
		data.setString(val.getString("driver"), "driver");
	} else {
		// если установлен хост - не используем sqlite
		if (val.hasValue("host")) {
			isSqlite = false;
			data.setString("pgsql", "driver");
		} else {
			data.setString("sqlite", "driver");
		}
	}

	for (auto &it : val.asDict()) {
		if (it.first == "host") {
			data.setString(it.second.getString(), "host");
		} else if (it.first == "dbname") {
			if (isSqlite) {
				// для sqlite - конвертируем путь в зависимый от текущей директории
				// абсолютные пути корректно обрабатываются автоматически
				if (it.second.isString()) {
					data.setString(filesystem::currentDir<Interface>(it.second.getString()), "dbname");
				} else {
					data.setString(filesystem::currentDir<Interface>("db.sqlite"), "dbname");
				}
			} else {
				data.setString(it.second.getString(), "dbname");
			}
		} else if (it.first == "user") {
			data.setString(it.second.getString(), "user");
		} else if (it.first == "password") {
			data.setString(it.second.getString(), "password");
		}
	}

	auto path = filesystem::writablePath<Interface>();

	// создаём простой сервер и соединяемся с ним
	auto serv = Rc<db::SimpleServer>::create(data,
			path, // базовый путь для хранения файлов вне БД
			db::AccessRoleId::Admin, // Пользовательская роль соединения
			SpanView<const db::Scheme *>() // список схем данных для загрузки, сейчас пустой
			);
	if (serv) {
		std::cout << "Connected!\n";
		std::cout << "Server DocumentRoot: " << serv->getDocumentRoot() << "\n";
		std::cout << "Database name: " << serv->getDatabaseName() << "\n";

		if (!command.empty()) {
			std::cout << "Query: \"" << command << "\"\n";

			// выполняем команду с помощью сервера
			serv->perform([&] (const db::Transaction &t) -> bool {
				// Получаем прямой доступ к SQL интерфейсу
				// Интерфейс может использовать не-SQL базу данных или объектный архив, потому используем dynamic_cast
				if (auto iface = dynamic_cast<db::sql::SqlHandle *>(t.getAdapter().getBackendInterface())) {
					ExampleDb_performQuery(iface, command);
				} else {
					log::error("ExampleDb", "Non-sql backing connection");
				}
				return true;
			});

		}
		return true;
	}

	return false;
}

}
