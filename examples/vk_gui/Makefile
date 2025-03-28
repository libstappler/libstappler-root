# Copyright (c) 2024-2025 Stappler LLC <admin@stappler.dev>
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

# Путь к SDK
STAPPLER_BUILD_ROOT ?= ../../build/make

# Указание перестаривать проект, если этот файл изменился
LOCAL_MAKEFILE := $(lastword $(MAKEFILE_LIST))

# Рабочая директория
LOCAL_ROOT = .

# Директория для результатов сборки
LOCAL_OUTDIR := stappler-build

# Название исполняемого файла
LOCAL_EXECUTABLE := testapp

# Пути к наборам модулей
LOCAL_MODULES_PATHS = \
	core/stappler-modules.mk \
	xenolith/xenolith-modules.mk

# Используемые модули
LOCAL_MODULES := \
	xenolith_backend_vkgui \
	xenolith_renderer_material2d \
	stappler_backtrace

LOCAL_SRCS_DIRS :=  src proj.android/src
LOCAL_SRCS_OBJS :=

LOCAL_INCLUDES_DIRS :=
LOCAL_INCLUDES_OBJS := src

LOCAL_SHADERS_DIR := shaders

# Файл исходного кода, содержащий функцию main
LOCAL_MAIN := main.cpp

# Android config
LOCAL_ANDROID_MK := proj.android/app/Android.mk # path to root Android.mk
LOCAL_APPLICATION_MK := proj.android/app/Application.mk # path to Application.mk
LOCAL_ANDROID_TARGET := application # target to build
LOCAL_ANDROID_PLATFORM := android-24 # android platform version

# ASAN := 1

# Подключения системы сборки
include $(STAPPLER_BUILD_ROOT)/universal.mk
