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
dbconnect <options> - generates password
Options are one of:
	--dbname <name> - DB name or path
	--user <name> - username for postgresql
	--password <name> - password for postgresql
	--host <name> - hostname for postgresql
	-S (--sqlite) - force to use sqlite driver
	-P (--pgsql) - force to use postgresql driver
	-v (--verbose)
	-h (--help)

$ stappler-build/host/dbconnect --dbname test.sqlite
Connected!

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
