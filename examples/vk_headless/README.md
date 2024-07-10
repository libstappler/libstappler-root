# Пример приложения командной строки с использованием Vulkan API

Приложение генерирует изображение с шумом по зерну с использованием вычислительного шейдера Vulkan. Создано для демонстрации запуска Vulkan. 

## Требования

Основные инструменты сборки: компиляторы C и C++, make. Установленные в системе средства [Vulkan или Vulkan SDK](https://github.com/libstappler/libstappler-doc/blob/master/docs-ru/other/vulkan.md).

## Структура

Makefile - Файл описания проекта
main.cpp - Код приложения

Приложение использует вспомогательный модуль, расположенный в [examples/modules/noisequeue](../modules/noisequeue).

## Сборка

При использовании Vulkan SDK:

```
make VULKAN_SDK_PREFIX=<префикс платформы внутри SDK>
```

При системных средствах Vulkan:

```
make
make install
```

Успешная сборка выглядит так:

```
Build for x86_64
Build executable: stappler-build/host/debug/gcc/noisegen
Enabled modules: example_noisequeue stappler_build_debug_module xenolith_backend_vk xenolith_application xenolith_platform xenolith_core stappler_threads stappler_geom stappler_font stappler_bitmap stappler_brotli_lib stappler_data stappler_filesystem stappler_core
Modules was updated
...
[glslangValidator] noise.comp/main.comp
[spirv-link] noise.comp
[embed] noise.comp.h
...
[noisegen: 100% 18/18] [g++] main.o
[Link] stappler-build/host/debug/gcc/noisegen
```

Готовое приложение будет расположено в `stappler-build/host/noisegen`

## Работа приложения

Приложение создаёт изображение, содержащее шум с заданным зерном и сохраняет его по переданному пути.

*Важно: SDK использует POSIX пути даже на Windows (cygwin-пути)*

```
$ stappler-build/host/noisegen --help
noisegen <options> <filename> - generates noise image
Options are one of:
	--dx <number> - seed for X
	--dy <number> - seed for Y
	--width <number> - image width
	--height <number> - image height
	-R (--random) - use random seed
	-v (--verbose)
	-h (--help)

# Создаём изображение 32 на 32 со случайным зерном, сохраняем в формате WebP.
$ stappler-build/host/noisegen -R --width 32 --height 32 test.webp
...
[Log] Application: started
[Log] core::Loop: ~Loop
[Log] core::Instance: ~Instance

# Создаём изображение 64 на 32 с заданным зерном, сохраняем в формате PNG. Вывод должен быть всегда одинаковым.
$ stappler-build/host/noisegen --dx 123 --dy 456 --width 64 --height 16 test.png
...
[Log] Application: started
[Log] core::Loop: ~Loop
[Log] core::Instance: ~Instance
```
