# Stappler SDK - корневой репозиторий

Stappler SDK - набор инструментов для разработки современных кроссплатформенных приложений:

* Высокопроизводительная графика и вычисления на Vulkan
* Доступ к базам данных на клиенте и сервере
* Приложение и вебсервер для него на одном языке с разделяемыми компонентами
* Скриптовая машина на WebAssembly (и любом языке, компилируемом в него)
* Только необходимый минимум зависимостей

## Поддержка платформ

* Linux - x86_64, arm64, e2k
* Android - все платформы, не требует Google Services
* Windows - x86_64
* MacOS - в разработке
* iOS - в разработке

## Графические приложения

SDK позволяет создавать полнофункциональные оконные приложения для всех систем. Графическая система основана на Vulkan.

* Графика с использованием векторных элементов и иконок - чёткая при любой плотности пикселей
* Полнофункциональная типографика с поддержкой Rich Text и отображения HTML
* Виджеты на основе Material Design
* Быстрая и отзывчивая система анимаций
* Экономия энергии устройств за счёт отрисовки по необходимости

## Доступ к базам данных

Работает поверх PostgreSQL или SQLite на клиенте и сервере

* Локальный firebase-подобный объектный интерфейс
* Вычислимые, автоматические, виртуальные поля, триггеры изменений
* Полнотекстовый поиск из коробки
* Принудительный контроль доступа

## Модуль веб-сервера

Построен поверх Apache HTTPD, в виде подключаемого модуля

* Автоматические интерфейсы для доступа к данным
* Поддержка WebSocket
* Асинхронные задачи, работа в фоновом режиме

## Интерфейс WebAssembly

Использует wit-bindgen для поддержки гостевых языков и интерпретатор WAMR.

* Поддержка любого языка из доступного для wit-bindgen или другого генератора биндингов на основе WIT
* Поддержка скомпилированных под платформу бинарных файлов (скрипты без потери в скорости)
* Возможность отладки WebAssembly кода

# Установка и запуск

## Зависимости

### Сборка и компиляция

* Make 4.0+ (gmake для MacOS)
* GCC 11+ или Clang 14+ (16+ для Windows), lcc 1.26+ для e2k (рекомендуется 1.28+)

### Базы данных

PostgreSQL или PostgresPro версии от 12. SQLite входит в поставку.

### Для Vulkan:

* Заголовки Vulkan в система или Vulkan SDK (Переменная `VULKAN_SDK_PREFIX`)
* glslangValidator (входит в SDK, переменная `GLSLC`)
* spirv-link (входит в SDK, переменная `SPIRV_LINK`)

### Для WebAssembly

* WASI SDK (`WASI_SDK ?= /opt/wasi-sdk`)
* wit-bindgen (`WIT_BINDGEN ?= wit-bindgen`)

### Для вебсервера

* Apache HTTPD
* Заголовки в системе (`APACHE_HTTPD_INCLUDE ?= /usr/local/include/apache`)

## Установка

```sh
git clone git@github.com:libstappler/libstappler-root.git
cd libstappler-root
git submodule update --init
```

Запуск первого примера

```sh
cd examples/commandline
make && make install
stappler-build/host/genpasswd
```

Тесты

```sh
cd tests/common
make && make install
stappler-build/host/testapp
```

## Первое приложение

Структура приложения:
* Makefile - корневой Mekafile проекта
* main.cpp - точка входа
* src - директория с исходными кодами

### Makefile

```make
# Путь к SDK
STAPPLER_ROOT ?= <путь к libstappler-root>

# Указание перестаривать проект, если этот файл изменился
LOCAL_MAKEFILE := $(lastword $(MAKEFILE_LIST))

# Название исполняемого файла
LOCAL_EXECUTABLE := helloworld

# Пути к наборам модулей
LOCAL_MODULES_PATHS = \
	$(STAPPLER_ROOT)/core/stappler-modules.mk

# Используемые модули
LOCAL_MODULES := \
	stappler_brotli_lib \
	stappler_data

# Пути к исходным кодам
LOCAL_SRCS_DIRS :=  src
LOCAL_INCLUDES_DIRS := src

# Файл исходного кода, содержащий функцию main
LOCAL_MAIN := main.cpp

# Подключения системы сборки
include $(STAPPLER_ROOT)/build/make/universal.mk
```

### main.cpp

```cpp
// Для функций чтения командной строки
#include "SPData.h"

namespace stappler::app {

// Выбираем стандартную подсистему памяти для текущего пространства имён
using namespace mem_std;

// Строка при запросе помощи по команде
static constexpr auto HELP_STRING(
R"HelpString(helloworld <options> - my first program
Options are one of:
	-h (--help))HelpString");

// Разбор коротких переключателей (-h, -v)
static int parseOptionSwitch(Value &ret, char c, const char *str) {
	if (c == 'h') {
		ret.setBool(true, "help");
	}

	// прочитан только один символ
	return 1;
}

// Разбор строковых переключателей (--help, -verbose)
static int parseOptionString(Value &ret, const StringView &str, int argc, const char * argv[]) {
	if (str == "help") {
		ret.setBool(true, "help");
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

	// выполняем в контексте временного пула памяти
	// пример не использует подсистему пулов памяти, но всегда заворачивать выполнение основного
	// потока во временный пул памяти - практика, позволяющая избегать ошибок
	perform_temporary([&] {
		std::cout << "Hello world!" << "\n";
	});

	return 0;
}

}

```

# Структура SDK

* [build](https://github.com/libstappler/libstappler-build.git) - файлы системы сборки
* [deps](https://github.com/libstappler/libstappler-deps.git) - файлы зависимостей
* [docs](https://github.com/libstappler/libstappler-doc.git) - файлы средств документации и инструментации
* [core](https://github.com/libstappler/libstappler-core.git) - базовая библиотека SDK
* [xenolith](https://github.com/libstappler/libstappler-xenolith.git) - графический движок
* extra - дополнительные библиотеки
* examples - примеры
* tests - тестовые приложения

## Документация

Основная документация расположена по адресу https://docs.stappler.dev/

Рабочая версия руководства по SDK: https://github.com/libstappler/libstappler-doc/tree/master/docs

Email для связи: admin@stappler.dev
