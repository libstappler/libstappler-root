# Пример запуска в docker-контейнере

Пример собирает новый docker-контейнер и запускает в нём модуль вебсервера.

## Требования

Приложение командной строки [docker](https://docs.docker.com/engine/reference/commandline/cli/)

Дистрибутив [Alpine Linux](https://www.alpinelinux.org/downloads/) в виде архива (alpine-minirootfs-3.20.0-x86_64.tar.gz), помещённый в эту директорию.

## Структура

Dockerfile - Файл описания проекта
web - код для запуска внутри контейнера
web/Makefile - Файл описания проекта
web/src/TestComponent.cpp - Простой серверный компонент, пример работы с БД
web/conf/httpd.conf - базовая конфигурация для Apache HTTPD

## Запуск

Сборка и запуск контейнера

```sh
wget https://dl-cdn.alpinelinux.org/alpine/v3.20/releases/x86_64/alpine-minirootfs-3.20.1-x86_64.tar.gz
docker build -t stappler-alpine .
docker run -it --rm stappler-alpine /bin/sh
```

Внутри контейнера:

```sh
cd /opt/libstappler-root/examples/docker/web
make APACHE_HTTPD_INCLUDE=/opt/apache/include install

# Запуск сервера
/opt/apache/bin/httpd -X -f /opt/libstappler-root/examples/docker/web/stappler-build/httpd.conf
```


## Запросы к приложению

Модуль предоставляет [ресурсный интерфейс](https://github.com/libstappler/libstappler-doc/blob/master/docs-ru/modules/web/resource.md) по адресу `http://localhost:8080/objects` внутри контейнера. Схема данных описана в web/src/TestComponent.cpp.