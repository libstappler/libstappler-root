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

Также, может генерировать закрытые ключи по алгоритму ГОСТ Р 2012.

```
$ stappler-build/host/genpasswd --help
genpasswd <options> [<action>]
Actions:
	genpassword (default) - generates password, at least one number, uppercase and lowercase char
	genkey - generates private key with GOST3410_2012 algorithm

Options:
  -v, --verbose
     - Produce more verbose output
  -h, --help
     - Show help message and exit
  -l<#>, --length <#>
     - Length for password or key

$ stappler-build/host/genpasswd
A6d9Q3

$ stappler-build/host/genpasswd -l12
7UE1JjKgYVC6

$ stappler-build/host/genpasswd --length 10
xY6WgpNP97

$ stappler-build/host/genpasswd genkey -l512
-----BEGIN PRIVATE KEY-----
MGgCAQAwIQYIKoUDBwEBAQIwFQYJKoUDBwECAQIBBggqhQMHAQECAwRAA+iz8gYO
KPNaasmETVwa5EC1SEzlhy2yGk5ZrMTWshStchIirp3PUQLzDZq20Llm+gUwmCgw
TVDTPnLoWFXbFw==
-----END PRIVATE KEY-----


```
