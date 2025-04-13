# Copyright (c) 2023-2025 Stappler LLC <admin@stappler.dev>
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

.DEFAULT_GOAL := all

ifndef TARGET_DIR
$(error TARGET_DIR should be defined for dependency download script)
endif

MAKE_FILE := $(realpath $(lastword $(MAKEFILE_LIST)))
MAKE_ROOT := $(dir $(LIBS_MAKE_FILE))

SRC_ROOT := $(TARGET_DIR)/src
TMP_DIR := $(TARGET_DIR)/tmp
XWIN_ROOT := $(TARGET_DIR)/xwin

WGET = wget
MKDIR = mkdir -p
TAR_XF = tar -xf

LIBS = \
	jpeg \
	libpng \
	giflib \
	libwebp \
	brotli \
	curl \
	freetype \
	sqlite \
	libuidna \
	libbacktrace \
	mbedtls \
	nghttp3 \
	libzip \
	zlib \
	openssl \
	openssl-gost-engine \
	wasm-micro-runtime

UNAME := $(shell uname -a)

# DO NOT use MSYS cmake - it's buggy for compiling with clang on windows
MSYS_PREPARE := pacman --noconfirm --needed -S curl wget tar sed unzip git binutils gcc

PREPARE := $(if $(filter MSYS_NT%,$(UNAME)),$(MSYS_PREPARE))

get_tar_top_dir = `tar -tf $(1)  | head -1 | cut -f1 -d"/"`

unpack_tar = \
	$(WGET) -O $(TARGET_DIR)/$(notdir $(1)) $(1); \
	$(TAR_XF) $(TARGET_DIR)/$(notdir $(1)); \
	mv -f $(call get_tar_top_dir,$(TARGET_DIR)/$(notdir $(1))) $(SRC_ROOT)/$(firstword $(2));

prepare:
	$(PREPARE)

all: $(addprefix $(SRC_ROOT)/,$(LIBS)) replacements/curl/cacert.pem

all:
	rm -rf $(TMP_DIR)

clean:
	rm -rf $(SRC_ROOT) $(XWIN_ROOT) replacements/curl/cacert.pem

# http://www.ijg.org/
$(SRC_ROOT)/jpeg: prepare
	$(call unpack_tar, http://ijg.org/files/jpegsrc.v9f.tar.gz, jpeg) # revised: 10 feb 2025

# http://www.libpng.org/pub/png/libpng.html
$(SRC_ROOT)/libpng: prepare
	$(call unpack_tar, http://prdownloads.sourceforge.net/libpng/libpng-1.6.45.tar.xz, libpng) # revised: 10 feb 2025

# https://sourceforge.net/projects/giflib/files/latest/download
$(SRC_ROOT)/giflib: prepare
	$(call unpack_tar, https://altushost-swe.dl.sourceforge.net/project/giflib/giflib-5.2.2.tar.gz, giflib) # revised: 10 feb 2025

# https://storage.googleapis.com/downloads.webmproject.org/releases/webp/index.html
$(SRC_ROOT)/libwebp: prepare
	$(call unpack_tar, https://storage.googleapis.com/downloads.webmproject.org/releases/webp/libwebp-1.5.0.tar.gz, libwebp) # revised: 10 feb 2025

# https://github.com/google/brotli/releases
$(SRC_ROOT)/brotli: prepare
	$(call unpack_tar, https://github.com/google/brotli/archive/refs/tags/v1.1.0.tar.gz, brotli) # revised: 10 feb 2025

# https://github.com/Mbed-TLS/mbedtls/releases
$(SRC_ROOT)/mbedtls: prepare
	$(call unpack_tar, https://github.com/Mbed-TLS/mbedtls/releases/download/mbedtls-3.6.1/mbedtls-3.6.1.tar.bz2, mbedtls) # revised: 10 feb 2025

# https://github.com/ngtcp2/nghttp3/releases
$(SRC_ROOT)/nghttp3: prepare
	$(call unpack_tar, https://github.com/ngtcp2/nghttp3/releases/download/v1.7.0/nghttp3-1.7.0.tar.bz2, nghttp3) # revised: 10 feb 2025

# https://curl.se/download.html
$(SRC_ROOT)/curl: prepare
	$(call unpack_tar, https://curl.se/download/curl-8.12.0.tar.xz, curl) # revised: 10 feb 2025

# https://download.savannah.gnu.org/releases/freetype/?C=M&O=D
$(SRC_ROOT)/freetype: prepare
	$(call unpack_tar, https://download.savannah.gnu.org/releases/freetype/freetype-2.13.3.tar.xz, freetype) # revised: 10 feb 2025

# https://www.sqlite.org/download.html
$(SRC_ROOT)/sqlite: prepare
	@$(MKDIR) $(SRC_ROOT) $(TMP_DIR)
	cd $(TMP_DIR); $(WGET) https://www.sqlite.org/2025/sqlite-amalgamation-3490000.zip # revised: 10 feb 2025
	cd $(TMP_DIR); unzip sqlite-amalgamation-3490000.zip -d .
	rm $(TMP_DIR)/sqlite-amalgamation-3490000.zip
	mv -f $(TMP_DIR)/sqlite-amalgamation-3490000 $(SRC_ROOT)/sqlite

# https://github.com/SBKarr/libuidna
$(SRC_ROOT)/libuidna: prepare
	@$(MKDIR) $(SRC_ROOT)
	rm -rf $(SRC_ROOT)/libuidna
	cd $(SRC_ROOT); git clone https://github.com/SBKarr/libuidna.git $(SRC_ROOT)/libuidna # use upstream: 10 feb 2025

# https://github.com/ianlancetaylor/libbacktrace
$(SRC_ROOT)/libbacktrace: prepare
	@$(MKDIR) $(SRC_ROOT)
	rm -rf $(SRC_ROOT)/libbacktrace
	cd $(SRC_ROOT); git clone https://github.com/ianlancetaylor/libbacktrace.git --depth 1 $(SRC_ROOT)/libbacktrace # use upstream: 10 feb 2025

# https://libzip.org/download/
$(SRC_ROOT)/libzip: prepare
	$(call unpack_tar, https://libzip.org/download/libzip-1.11.3.tar.xz, libzip) # revised: 10 feb 2025

# https://www.zlib.net/
$(SRC_ROOT)/zlib: prepare
	$(call unpack_tar, https://www.zlib.net/zlib-1.3.1.tar.gz, zlib) # revised: 10 feb 2025

# https://openssl-library.org/source/index.html
$(SRC_ROOT)/openssl: prepare
	$(call unpack_tar, https://github.com/openssl/openssl/releases/download/openssl-3.3.2/openssl-3.3.2.tar.gz, openssl) # revised: 10 feb 2025
	cp -f patches/openssl/async_posix.c $(SRC_ROOT)/openssl/crypto/async/arch/async_posix.c

# https://github.com/gost-engine/engine
$(SRC_ROOT)/openssl-gost-engine: prepare
	@$(MKDIR) $(SRC_ROOT)
	rm -rf $(SRC_ROOT)/openssl-gost-engine
	cd $(SRC_ROOT); git clone  --recurse-submodules  --branch v3.0.3 https://github.com/gost-engine/engine.git --depth 1 openssl-gost-engine # revised: 10 feb 2025
	cp -f patches/openssl-gost-engine/CMakeLists.txt $(SRC_ROOT)/openssl-gost-engine

# https://github.com/bytecodealliance/wasm-micro-runtime
$(SRC_ROOT)/wasm-micro-runtime: prepare
	@$(MKDIR) $(SRC_ROOT)
	rm -rf $(SRC_ROOT)/wasm-micro-runtime
	cd $(SRC_ROOT); git clone  --recurse-submodules  --branch WAMR-2.1.2 https://github.com/bytecodealliance/wasm-micro-runtime.git --depth 1 wasm-micro-runtime # revised: 10 feb 2025
	cd $(SRC_ROOT)/wasm-micro-runtime; git apply ../../replacements/wamr/0001-Initial-e2k-support.patch
	cd $(SRC_ROOT)/wasm-micro-runtime; git apply ../../replacements/wamr/0002-Windows-clang-fixes.patch

# Inject Russia Ministry of Digital Development certificates
# https://curl.se/ca
# https://www.gosuslugi.ru/crt
replacements/curl/cacert.pem: prepare $(LIBS_MAKE_FILE)
	@$(MKDIR) replacements/curl
	cd $(TMP_DIR); wget https://curl.se/ca/cacert-2024-12-31.pem # revised: 10 feb 2025
	cd $(TMP_DIR); wget https://gu-st.ru/content/lending/russian_trusted_root_ca_pem.crt
	cd $(TMP_DIR); wget https://gu-st.ru/content/lending/russian_trusted_sub_ca_pem.crt
	cd $(TMP_DIR); wget https://gu-st.ru/content/lending/russian_trusted_sub_ca_2024_pem.crt
	sed -i '1s;^;\nhttps://www.gosuslugi.ru/crt - Root\n====================\n;' $(TMP_DIR)/russian_trusted_root_ca_pem.crt
	sed -i '1s;^;\n\nhttps://www.gosuslugi.ru/crt - Sub\n====================\n;' $(TMP_DIR)/russian_trusted_sub_ca_pem.crt
	sed -i '1s;^;\n\nhttps://www.gosuslugi.ru/crt - Sub 2024\n====================\n;' $(TMP_DIR)/russian_trusted_sub_ca_2024_pem.crt
	cd replacements/curl; cat \
		$(TMP_DIR)/cacert-2024-12-31.pem \
		$(TMP_DIR)/russian_trusted_root_ca_pem.crt \
		$(TMP_DIR)/russian_trusted_sub_ca_pem.crt \
		$(TMP_DIR)/russian_trusted_sub_ca_2024_pem.crt > cacert.pem
	@rm \
		$(TMP_DIR)/cacert-2024-12-31.pem \
		$(TMP_DIR)/russian_trusted_root_ca_pem.crt \
		$(TMP_DIR)/russian_trusted_sub_ca_pem.crt \
		$(TMP_DIR)/russian_trusted_sub_ca_2024_pem.crt

# https://github.com/Jake-Shadle/xwin/releases
xwin: prepare
	@$(MKDIR) $(SRC_ROOT)
	$(WGET) https://github.com/Jake-Shadle/xwin/releases/download/0.6.5/xwin-0.6.5-x86_64-unknown-linux-musl.tar.gz # revised: 10 feb 2025
	$(TAR_XF) xwin-0.6.5-x86_64-unknown-linux-musl.tar.gz
	rm xwin-0.6.5-x86_64-unknown-linux-musl.tar.gz
	mv xwin-0.6.5-x86_64-unknown-linux-musl xwin
	cd xwin; ./xwin --accept-license unpack; ./xwin --accept-license splat
	mv xwin/.xwin-cache/splat xwin/splat

.PHONY: all clean prepare
