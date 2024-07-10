# Пример приложения командной строки

Приложение генерирует пароль средствами Stappler SDK. Оно создано, чтобы продемонстрировать основные принципы сборки приложений с помощью Stappler SDK

## Требования

Основные инструменты сборки: компиляторы C и C++, make. Предсобранные или собранные вручную зависимости SDK.

## Структура

Makefile - Файл описания проекта
main.cpp - Код приложения

## Сборка

```
make
make install
```

Успешная сборка выглядит так:

```
Build for x86_64
Build executable: stappler-build/host/debug/gcc/genpasswd
Enabled modules: stappler_brotli_lib stappler_data stappler_build_debug_module stappler_filesystem stappler_core
Modules was updated
[genpasswd: 16% 1/6] [g++] SPCommon.h.gch
[genpasswd: 33% 2/6] [g++] SPFilesystem.scu.o
[genpasswd: 50% 3/6] [g++] SPCore.scu.o
[genpasswd: 66% 4/6] [gcc] SPThirdparty.scu.o
[genpasswd: 83% 5/6] [g++] SPDebugModule.o
[genpasswd: 100% 6/6] [g++] main.o
[Link] stappler-build/host/debug/gcc/genpasswd
cp -f stappler-build/host/debug/gcc/genpasswd stappler-build/host/genpasswd
```

Готовое приложение будет расположено в `stappler-build/host/genpasswd`

## Работа приложения

Приложение генерирует пароль, содержащий, как минимум, одну цифру, один строчный и один заглавный символ. По умолчанию длина пароля - 6 символов.

```
$ stappler-build/host/genpasswd --help
genpasswd <options> - generates password, at least one number, uppercase and lowercase char
Options are one of:
	-l<n> (--length <n>) - length for password in [6-256] (default: 6)
	-v (--verbose)
	-h (--help)
>$ stappler-build/host/genpasswd
A6d9Q3
>$ stappler-build/host/genpasswd -l12
7UE1JjKgYVC6
>$ stappler-build/host/genpasswd --length 10
xY6WgpNP97
```
