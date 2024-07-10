# Пример модуля веб-сервера, использующего Vulkan

Приложение работает в качестве модуля веб-сервера, позволяющего генерировать шум (аналогично примеру vk_headless) на сервере и отправлять результат веб-клиенту по HTTP-запросу.

Также, реализует простой компонент для доступа к БД, и базовый интерфейс для WebAssembly компонента.

## Требования

Основные инструменты сборки: компиляторы C и C++, make. Установленные в системе средства [Vulkan или Vulkan SDK](https://github.com/libstappler/libstappler-doc/blob/master/docs-ru/other/vulkan.md). [Средства сборки WebAssembly](https://github.com/libstappler/libstappler-doc/blob/master/docs-ru/other/webassembly.md). Установленный и доступный пользователю Apache HTTPD.

## Структура

Makefile - Файл описания проекта
src/TestComponent.cpp - Простой серверный компонент, пример работы с БД
src/VkComponent.cpp - заголовок класса приложения
src/component.wit - интерфейс загрузки WebAssembly компонента
wasm/module.cpp - пример модуля WebAssembly
conf/httpd.conf - базовая конфигурация для Apache HTTPD

## Сборка

При использовании Vulkan SDK необходимо указать переменную `VULKAN_SDK_PREFIX=<префикс платформы внутри SDK>`.

Для заголовков Apache HTTPD указать путь к заголовкам `APACHE_INCLUDE=<путь к include собранного сервера>`.

```
make <список перемменных>
make install
```

## Запуск приложения

```
# Флаг -X - запускает сервер в текущем терминале и блокирует егоб направляет вывод сервера в текущий терминал
# -f <файл конфигурации> - указывает конфигурацию для запуска

<путь к директории собранного вебсервера>/httpd -X -f `pwd`/stappler-build/httpd.conf
```

При успешном запуске терминал покажет сообщение о загрузке и будет ожидать входящих запросов

```
[Log] wasm::Runtime: handleChildInit
[Log] wasm::Runtime: handleStorageInit
Verbose: [Gl::Loop] VkComponent: Queue compiled
```

## Запросы к приложению

Выполняются из браузера или другого терминала:

Генерирует изображение шума со случайным зерном. Используется формат [Stappler/Serenity Query](https://github.com/libstappler/libstappler-doc/blob/master/docs-ru/modules/data.md#serenity-query) для параметров.

```
wget "http://localhost:8080/vk?(random;width:32;height:32)" > test.png
```

Генерирует изображение шума с заданным зерном. Стандартный формат параметров.

```
wget "http://localhost:8080/vk?dw=123&dh=456&width=32&height=32" > test.png
```

Модуль предоставляет [ресурсный интерфейс](https://github.com/libstappler/libstappler-doc/blob/master/docs-ru/modules/web/resource.md) по адресу `http://localhost:8080/objects`. Схема данных описана в src/TestComponent.cpp.
