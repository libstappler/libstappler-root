# Тесты покрытия

Приложение с тестами покрытия. Может выполняться только на настольных платформах с поддержкой графического вывода.

## Требования

Полная установка:
* Vulkan
* WebAssembly
* GnuTLS для Linux

## Структура

Makefile - Файл описания проекта
main.cpp - Точка входа
src - Основная директория с тестами
shaders - Необходимые шейдеры для теста
resources, data - Необходимые ресурсы для тестов
web - Директория тестов вебсервера
xenolith - Директория автотестов графического движка

## Сборка

```
make
make install
```

Для покрытия:

```
make host-coverage
make host-report
```

## Работа приложения

Список доступных тестов:

```
./testapp list
```

Запуск тестов по имени:

```
./testapp MemVectorTest TimeTest XenolithGuiTest
```

Запуск всех тестов:

```
./testapp all
```

## Добавление теста

Для использования пулов памяти

```
#include "SPCommon.h"
#include "Test.h"

namespace STAPPLER_VERSIONIZED stappler::app::test {

struct NewPoolTest : MemPoolTest {
	NewPoolTest() : MemPoolTest("NewPoolTest") { }

	virtual bool run(pool_t *pool) {
		StringStream stream;
		size_t count = 0;
		size_t passed = 0;
		stream << "\n";

		runTest(stream, "subtest1", count, passed, [&] {
			// возвращает успешность прохождения теста
			return true;
		});

		runTest(stream, "subtest2", count, passed, [&] {
			return true;
		});

		runTest(stream, "subtest3", count, passed, [&] {
			return true;
		});

		_desc = stream.str();
		return count == passed;
	}
} _NewPoolTest;

}
```

Без использования пулов памяти

```
#include "SPCommon.h"
#include "Test.h"

namespace STAPPLER_VERSIONIZED stappler::app::test {

struct NewTest : Test {
	NewTest() : Test("NewTest") { }

	virtual bool run() {
		StringStream stream;
		size_t count = 0;
		size_t passed = 0;
		stream << "\n";

		runTest(stream, "subtest1", count, passed, [&] {
			// возвращает успешность прохождения теста
			return true;
		});

		runTest(stream, "subtest2", count, passed, [&] {
			return true;
		});

		runTest(stream, "subtest3", count, passed, [&] {
			return true;
		});

		_desc = stream.str();
		return count == passed;
	}
} _NewTest;

}
```
