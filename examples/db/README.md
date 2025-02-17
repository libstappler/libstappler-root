# Пример приложения для соединения с БД

Приложение устанавливает и настраивает соединение с указанной БД, настраивает в ней служебные функции и таблицы. Оно создано, чтобы продемонстрировать основы соединения с БД с помощью Stappler SDK.

## Требования

Основные инструменты сборки: компиляторы C и C++, make. Предсобранные или собранные вручную зависимости SDK.

## Структура

Makefile - Файл описания проекта
main.cpp - Точка входа
src/ExambleDb.cpp - код соединения с БД
src/ExambleDb.h - заголовок соединения с БД

## Сборка

```
make
make install
```

Успешная сборка выглядит так:

```
Build for x86_64
Build executable: stappler-build/host/debug/gcc/dbconnect
Enabled modules: stappler_brotli_lib stappler_db stappler_build_debug_module stappler_search stappler_sql stappler_data stappler_filesystem stappler_crypto stappler_crypto_openssl stappler_core
Modules was updated
[dbconnect: 10% 1/10] [g++] SPCommon.h.gch
...
[dbconnect: 100% 10/10] [g++] main.o
[Link] stappler-build/host/debug/gcc/dbconnect
```

Готовое приложение будет расположено в `stappler-build/host/dbconnect`

## Работа приложения

Приложение соединяется с БД и записывает в неё необходимые для работы SDK служебные таблицы. Приложение может соединяться с SQLite или сервером PostgreSQL. Для SQLite необходимо указать только параметр `--dbname <путь к файлу>`. 

*Важно: SDK использует POSIX пути даже на Windows (cygwin-пути)*

```
$ stappler-build/host/dbconnect --help
dbconnect <options> [query] - opens simple connection with database and execute query

Example:
	dbconnect -P --user=stappler --password=stappler --dbname=stappler --host=localhost "SELECT version();"

Options:
  -v, --verbose
     - Produce more verbose output
  -h, --help
     - Show help message and exit
  -S, --sqlite
     - Use SQLite driver
  -P, --pgsql
     - Use PostgreSQL driver (libpq)
  -D<name>, --dbname <name>
     - Target database name or path for SQLite
  -U<username>, --user <username>
     - Username for database connection
  -P<password>, --password <password>
     - Password for database connection
  -H<hostname>, --host <hostname>
     - Database host for connection

# Соединение с PostgreSQL
$ stappler-build/host/dbconnect -P --user=stappler --password=stappler --dbname=stappler --host=localhost "SELECT version();"
Connected!
Server DocumentRoot: examples/db/stappler-build/host/AppData
Database name: stappler
Query: "SELECT version();"
Query: success; rows affected: 1; rows in result: 1;
Fields: "version"
[0]	"PostgreSQL 16.6"

# Соединение с SQLite, создаёт файл БД, если его не существует
$ stappler-build/host/dbconnect --dbname test.sqlite
Connected!
Server DocumentRoot: examples/db/stappler-build/host/AppData
Database name: examples/db/test.sqlite

# Проверяем, что команда сработала, просматривая таблицы в новой БД
$ sqlite3 test.sqlite -cmd "SELECT name FROM sqlite_schema WHERE type ='table';" ".exit"
__objects
__removed
__versions
__sessions
__broadcasts
sqlite_sequence
__login
__words
__error
__files
__users
```
