# Copyright (c) 2023 Stappler LLC <admin@stappler.dev>
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

STAPPLER_ROOT ?= ../..
# STAPPLER_VERSION_PREFIX := sp_test

# force to rebuild if this makefile changed
LOCAL_MAKEFILE := $(lastword $(MAKEFILE_LIST))

LOCAL_OUTDIR := stappler-build
LOCAL_EXECUTABLE := testapp
#LOCAL_WASM_MODULE := app.wasm

LOCAL_MODULES_PATHS = \
	$(STAPPLER_ROOT)/core/stappler-modules.mk \
	$(STAPPLER_ROOT)/xenolith/xenolith-modules.mk \
	$(STAPPLER_ROOT)/extra/webserver/webserver-modules.mk

LOCAL_MODULES ?= \
	stappler_bitmap \
	stappler_brotli_lib \
	stappler_db \
	stappler_vg \
	stappler_threads \
	stappler_zip \
	stappler_threads \
	stappler_network \
	stappler_backtrace \
	stappler_crypto_openssl \
	stappler_crypto_mbedtls \
	stappler_font

ifdef LINUX
ifneq ($(STAPPLER_ARCH),e2k)
ifneq ($(STAPPLER_ARCH),aarch64)
LOCAL_MODULES += \
	stappler_crypto_gnutls
endif
endif

LOCAL_MODULES += \
	stappler_webserver_unix

endif

ifndef WIN32
LOCAL_MODULES += \
	stappler_webserver_pug \
	stappler_webserver_webserver \
	xenolith_font \
	xenolith_backend_vkgui \
	xenolith_resources_assets \
	xenolith_renderer_material2d
endif

LOCAL_ROOT = .

SHADERNN_DIR := $(STAPPLER_ROOT)/xenolith/utils/shadernn

LOCAL_SRCS_DIRS :=  src xenolith web $(SHADERNN_DIR)/src $(SHADERNN_DIR)/tests
LOCAL_SRCS_OBJS := app.wit

LOCAL_INCLUDES_DIRS := src xenolith $(SHADERNN_DIR)/src $(SHADERNN_DIR)/tests
LOCAL_INCLUDES_OBJS :=

LOCAL_WASM_DIRS := wasm

LOCAL_MAIN := main.cpp

SHADER_WORK_GROUP_X = 4
SHADER_WORK_GROUP_Y = 8
SHADER_WORK_GROUP_Z = 4

LOCAL_SHADERS_RULES := -DWG_X=${SHADER_WORK_GROUP_X} -DWG_Y=${SHADER_WORK_GROUP_Y} -DWG_Z=${SHADER_WORK_GROUP_Z}
LOCAL_SHADERS_DIR := shaders \
	$(SHADERNN_DIR)/shaders/f16 $(SHADERNN_DIR)/shaders/f32 $(SHADERNN_DIR)/shaders/nme $(SHADERNN_DIR)/shaders/stat

#ASAN := 1

ifdef LINUX
# For dynamic self-loading
TOOLKIT_CFLAGS := -DXL_LINK=1
LOCAL_LDFLAGS := -Wl,--export-dynamic -ldbus-1 -lwayland-client -lxcb -lxcb-randr -lxcb-keysyms -lxcb-xkb -lxkbcommon -lxkbcommon-x11
endif

include $(STAPPLER_ROOT)/build/make/universal.mk
