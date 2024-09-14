# Copyright (c) 2024 Stappler LLC <admin@stappler.dev>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

# Для зависимости от кода мейкфайла
MAKEFILE := $(lastword $(MAKEFILE_LIST))

# Базовые установки
ARCH ?= x86_64
BASE_IMAGE_NAME ?= sbkarr/libstappler-root:latest
HTTPD_IMAGE_NAME ?= libstappler-example-docker
CONTAINER_ID_FILE := .container_id

# Порты, доступные на внешней системе
HOST_PORT ?= 8080
HOST_SSL_PORT ?= 8443

# Порты внутри контейнера
CONTAINER_PORT := 8080
CONTAINER_SSL_PORT := 8443

# Данные административного пользователя внутри контейнера
CONTAINER_USERNAME ?= admin
CONTAINER_PASSWORD ?= 12345678

url_notdir = $(notdir $(subst https:/,,$(1)))
url_basename = $(basename $(basename $(call url_notdir,$(1))))

# Источники компонентов
HTTPD_URL ?= https://dlcdn.apache.org/httpd/httpd-2.4.62.tar.bz2
APR_URL ?= https://dlcdn.apache.org//apr/apr-1.7.5.tar.bz2
APR_UTIL_URL ?= https://dlcdn.apache.org//apr/apr-util-1.6.3.tar.bz2
PCRE_URL ?= https://github.com/PCRE2Project/pcre2/releases/download/pcre2-10.43/pcre2-10.43.tar.bz2
EXPAT_URL ?= https://github.com/libexpat/libexpat/releases/download/R_2_6_2/expat-2.6.2.tar.bz2

HTTPD_TARBALL := $(call url_notdir,$(HTTPD_URL))
APR_TARBALL := $(call url_notdir,$(APR_URL))
APR_UTIL_TARBALL := $(call url_notdir,$(APR_UTIL_URL))
PCRE_TARBALL := $(call url_notdir,$(PCRE_URL))
EXPAT_TARBALL := $(call url_notdir,$(EXPAT_URL))

HTTPD_VERSION := $(call url_basename,$(HTTPD_URL))
APR_VERSION := $(call url_basename,$(APR_URL))
APR_UTIL_VERSION := $(call url_basename,$(APR_UTIL_URL))
PCRE_VERSION := $(call url_basename,$(PCRE_URL))
EXPAT_VERSION := $(call url_basename,$(EXPAT_URL))

define newline


endef

# Шаблон для сборки контейнера
define DOCKERFILE_CONTENT
FROM $(BASE_IMAGE_NAME)
RUN \\
	apk add libstdc++ make cmake gcc g++ git pkgconfig perl linux-headers binutils-gold \\
	&& cd /opt \\
	&& mkdir httpd \\
	&& cd httpd \\
	&& wget $(HTTPD_URL) \\
	&& tar -xf $(HTTPD_TARBALL) \\
	&& rm $(HTTPD_TARBALL) \\
	&& mv $(HTTPD_VERSION) httpd \\
	&& wget $(APR_URL) \\
	&& tar -xf $(APR_TARBALL) \\
	&& rm $(APR_TARBALL) \\
	&& mv $(APR_VERSION) httpd/srclib/apr \\
	&& wget $(APR_UTIL_URL) \\
	&& tar -xf $(APR_UTIL_TARBALL) \\
	&& rm $(APR_UTIL_TARBALL) \\
	&& mv $(APR_UTIL_VERSION) httpd/srclib/apr-util \\
	&& wget $(PCRE_URL) \\
	&& tar -xf $(PCRE_TARBALL) \\
	&& rm $(PCRE_TARBALL) \\
	&& cd $(PCRE_VERSION) \\
	&& ./configure \\
	&& make \\
	&& make install \\
	&& cd .. \\
	&& wget $(EXPAT_URL) \\
	&& tar -xf $(EXPAT_TARBALL) \\
	&& rm $(EXPAT_TARBALL) \\
	&& cd $(EXPAT_VERSION) \\
	&& ./configure \\
	&& make \\
	&& make install \\
	&& cd .. \\
	&& cd httpd \\
	&& export STAPPLER_DEPS_ROOT=/opt/libstappler-root/deps/linux/$(ARCH) \\
	&& chmod +x /opt/libstappler-root/deps/replacements/httpd/configure-stappler.sh \\
	&& /opt/libstappler-root/deps/replacements/httpd/configure-stappler.sh /opt/apache \\
	&& make \\
	&& make install \\
	&& cd .. \\
	&& rm -rf /opts/httpd \\
	&& cd /opt/libstappler-root/examples/docker/web \\
	&& make APACHE_HTTPD_INCLUDE=/opt/apache/include RELEASE=1 install \\
	&& mkdir /opt/libstappler-root/examples/docker/web/www \\
	&& cd /opt/libstappler-root/examples/docker/web/conf \\
	&& apk add openssl \\
	&& openssl req -x509 -out localhost.crt -keyout localhost.key   -newkey rsa:2048 -nodes \\
		-sha256   -subj '/CN=localhost' -extensions EXT -config <( \\
		printf "[dn]\\nCN=localhost\\n[req]\\ndistinguished_name = dn\\n[EXT]\\nsubjectAltName=DNS:localhost\\nkeyUsage=digitalSignature\\nextendedKeyUsage=serverAuth") \\
	&& rm -rf /opt/libstappler-root/examples/docker/web/stappler-build/host/release \\
	&& apk del make cmake gcc g++ git pkgconfig perl linux-headers binutils-gold openssl
EXPOSE $(CONTAINER_PORT)
EXPOSE $(CONTAINER_SSL_PORT)
endef

# Сборка шаблона для скрипта сборки контейнера
Dockerfile: $(MAKEFILE)
	@echo Dockerfile update:
	@echo Using Apache HTTPD: $(HTTPD_VERSION)
	@echo Using Apache Portable Runtime: $(APR_VERSION)
	@echo Using Apache Portable Runtime Utils: $(APR_UTIL_VERSION)
	@echo Using PCRE: $(PCRE_VERSION)
	@echo Using Expat: $(EXPAT_VERSION)
	@printf '$(subst $(newline),\n,$(DOCKERFILE_CONTENT))\n' > Dockerfile

# Сборка образа для контейнеров
httpd-docker-image: Dockerfile
	docker build -t $(HTTPD_IMAGE_NAME) .

all: httpd-docker-image

# Запускает образ в фоновом режиме и открывает доступ к нему по заданным портам
# Также. настраивает вебсервер и создаёт административного пользователя
start:
	docker run -d --rm -p $(HOST_PORT):$(CONTAINER_PORT) -p $(HOST_SSL_PORT):$(CONTAINER_SSL_PORT) $(HTTPD_IMAGE_NAME) \
		/opt/apache/bin/httpd -f /opt/libstappler-root/examples/docker/web/stappler-build/httpd.conf -X > $(CONTAINER_ID_FILE)
	@cat $(CONTAINER_ID_FILE)
	curl -k "https://localhost:$(HOST_SSL_PORT)/__server" --data "name=$(CONTAINER_USERNAME)&passwd=$(CONTAINER_PASSWORD)" \
		-H "Content-Type: application/x-www-form-urlencoded"

ifneq ($(file < $(CONTAINER_ID_FILE)),)
# Останавливает и удаляет образ, ранее запущенный в фоновом режиме
stop:
	docker stop $(file < $(CONTAINER_ID_FILE)) >> /dev/null
	rm $(CONTAINER_ID_FILE)

# Создаёт объект с помощью интерфейса сервера с параметрами TEXT и ALIAS
create:
	curl -k "https://$(CONTAINER_USERNAME):$(CONTAINER_PASSWORD)@localhost:$(HOST_SSL_PORT)/objects?pretty" --data "text=$(TEXT)&alias=$(ALIAS)" \
		-H "Content-Type: application/x-www-form-urlencoded"

# Получает все объекты, созданные на сервере
get:
	curl -k "https://$(CONTAINER_USERNAME):$(CONTAINER_PASSWORD)@localhost:$(HOST_SSL_PORT)/objects?pretty"

# Удаляет объект по идентификатору ID
delete:
	curl -X "DELETE" -k "https://$(CONTAINER_USERNAME):$(CONTAINER_PASSWORD)@localhost:$(HOST_SSL_PORT)/objects/id$(ID)"

# Удаляет ранее собранный образ из системы
clean:
	docker image rm $(HTTPD_IMAGE_NAME)

else

stop:
	@echo Container id from 'make start' is not defined

create:
	@echo Container is not launched

get:
	@echo Container is not launched

delete:
	@echo Container is not launched

endif

.DEFAULT_GOAL := all
.PHONY: all clean httpd-docker-image
