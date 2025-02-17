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

STAPPLER_BUILD_ROOT ?= ../../build/make

# STAPPLER_VERSION_PREFIX := sp_test

# force to rebuild if this makefile changed
LOCAL_MAKEFILE := $(lastword $(MAKEFILE_LIST))

LOCAL_OUTDIR := stappler-build
LOCAL_EXECUTABLE := testapp

LOCAL_MODULES_PATHS = \
	core/stappler-modules.mk \
	xenolith/xenolith-modules.mk \
	extra/webserver/webserver-modules.mk

LOCAL_MODULES := \
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
	stappler_font \
	xenolith_core

LOCAL_MODULES_OPTIONAL := \
	stappler_crypto_gnutls \
	stappler_crypto_mbedtls \
	stappler_webserver_pug \
	stappler_webserver_webserver \
	stappler_wasm

ifndef XWIN
LOCAL_MODULES += \
	xenolith_font \
	xenolith_backend_vkgui \
	xenolith_resources_assets \
	xenolith_renderer_material2d

LOCAL_MODULES_OPTIONAL += \
	stappler_webserver_unix
endif

LOCAL_ROOT = .

SHADERNN_DIR := ../../xenolith/utils/shadernn

LOCAL_SRCS_DIRS :=  src
LOCAL_SRCS_OBJS := app.wit

LOCAL_INCLUDES_DIRS := src
LOCAL_INCLUDES_OBJS :=

ifndef XWIN
LOCAL_SRCS_DIRS += $(SHADERNN_DIR)/src $(SHADERNN_DIR)/tests xenolith web
LOCAL_INCLUDES_DIRS += $(SHADERNN_DIR)/src $(SHADERNN_DIR)/tests xenolith
endif

LOCAL_MAIN := main.cpp

ifdef SHARED_PREFIX
$(info SHARED_ROOT: $(SHARED_PREFIX))
LOCAL_LDFLAGS := -lpng
else
LOCAL_WASM_MODULE := app.wasm
LOCAL_WASM_DIRS := wasm
endif

SHADER_WORK_GROUP_X = 4
SHADER_WORK_GROUP_Y = 8
SHADER_WORK_GROUP_Z = 4

LOCAL_SHADERS_RULES := -DWG_X=${SHADER_WORK_GROUP_X} -DWG_Y=${SHADER_WORK_GROUP_Y} -DWG_Z=${SHADER_WORK_GROUP_Z}
LOCAL_SHADERS_DIR := shaders \
	$(SHADERNN_DIR)/shaders/f16 $(SHADERNN_DIR)/shaders/f32 $(SHADERNN_DIR)/shaders/nme $(SHADERNN_DIR)/shaders/stat

#ASAN := 1

include $(STAPPLER_BUILD_ROOT)/universal.mk
