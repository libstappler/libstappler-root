# Пример запуска в docker-контейнере

Пример собирает новый docker-контейнер и запускает в нём модуль вебсервера.

## Требования

Приложение командной строки [docker](https://docs.docker.com/engine/reference/commandline/cli/)

Образ [sbkarr/libstappler-root](https://hub.docker.com/repository/docker/sbkarr/libstappler-root/general) из Docker Hub.

Опционально, в случае, если Docker Hub недоступен, образ можно собрать локально из корня SDK локально:

```
cd <путь>/libstappler-root
wget https://dl-cdn.alpinelinux.org/alpine/v3.20/releases/x86_64/alpine-minirootfs-3.20.3-x86_64.tar.gz
docker build -t sbkarr/libstappler-root:latest .
rm alpine-minirootfs-3.20.2-x86_64.tar.gz

# Не забудьте вернуться к исходному коду примера
cd -
```

## Структура

`Dockerfile` - Файл описания сборки контейнера

`Makefile` - Контрольный файл для управления сборкой и запуском контейнера

`web` - код для запуска внутри контейнера

`web/Makefile` - Файл описания проекта

`web/src/TestComponent.cpp` - Простой серверный компонент, пример работы с БД

`web/conf/httpd.conf` - базовая конфигурация для Apache HTTPD

## Запуск

Для упрощения, все манипуляции проводятся с помощью make, реально вызываемые команды docker будут показаны при соотвествующих вызовах.

Сборка образа для контейнера

```sh
make all
# подготавливает обновление для Dockerfile и вызывает
# docker build -t libstappler-example-docker .
```

Для запуска контейнера с вебсервером:

```sh
make start
```

Команда создаёт новый контейнер, настраивает порты, описанные в Makefile (по умолчанию: 8080 и 8443), вызывает `/opt/apache/bin/httpd -f /opt/libstappler-root/examples/docker/web/stappler-build/httpd.conf -X` внутри контейнера, запускаемого в фоновом режиме.

Для остановки контейнера:

```sh
make stop
```

Команда останавливает и удаляет ранее созданный контейнер.


## Запросы к приложению

Модуль предоставляет [ресурсный интерфейс](https://github.com/libstappler/libstappler-doc/blob/master/docs-ru/modules/web/resource.md) по адресу `http://localhost:8080/objects` внутри контейнера. Схема данных описана в web/src/TestComponent.cpp.

Для тестирования запросов можно использовать `make`, когда контейнер запущен (команды используют curl):

```
# Создание объекта
make create TEXT=<text> ALIAS=<alias>

# Удаление объекта (по полю __oid)
make delete ID=<id>

# Получение всех объектов
make get
```

Обратите внимание, что для аутентификации необходимо использовать HTTPS (порт 8443), в противном случае сервер не будет авторизовать пользователя, поскольку стандартная политика не допускает работу без защищённого соединения.
