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

LOCAL_LIBRARY := libstappler
LOCAL_VERSION := 0.3

# force to rebuild if this makefile changed
LOCAL_MAKEFILE := $(lastword $(MAKEFILE_LIST))

LOCAL_OUTDIR := stappler-build

LOCAL_MODULES_PATHS = \
	core/stappler-modules.mk \
	xenolith/xenolith-modules.mk \
	extra/document/document-modules.mk \
	extra/webserver/webserver-modules.mk

LOCAL_MODULES ?= \
	stappler_core \
	stappler_brotli_lib \
	stappler_data \
	stappler_bitmap \
	stappler_crypto \
	stappler_search \
	stappler_db \
	stappler_geom \
	stappler_vg \
	stappler_tess \
	stappler_threads \
	stappler_idn \
	stappler_network \
	stappler_font \
	stappler_zip \
	xenolith_core \
	xenolith_font \
	xenolith_backend_vk \
	xenolith_backend_vkgui \
	xenolith_renderer_basic2d \
	xenolith_renderer_material2d \
	xenolith_resources_assets \
	stappler_document_document \
	stappler_document_richtext \
	stappler_document_mmd_document \
	stappler_webserver_unix

# Force macOS make to detect shaders module
LOCAL_MODULES += xenolith_renderer_basic2d_shaders

LOCAL_ARCHIVE_FILES := \
	build/ \
	core/ \
	extra/ \
	xenolith/application/ \
	xenolith/backend/ \
	xenolith/core/ \
	xenolith/font/ \
	xenolith/platform/ \
	xenolith/renderer/ \
	xenolith/resources/ \
	xenolith/scene/ \
	xenolith/thirdparty/ \
	xenolith/LICENSE \
	xenolith/xenolith-modules.mk \
	LICENSE \
	Makefile \
	README.md

$(LOCAL_LIBRARY)-$(LOCAL_VERSION).tar: $(LOCAL_MAKEFILE)
	tar --transform 's,^\.,$(LOCAL_LIBRARY)-$(LOCAL_VERSION),' -cf $(LOCAL_LIBRARY)-$(LOCAL_VERSION).tar $(LOCAL_ARCHIVE_FILES)

tar: $(LOCAL_LIBRARY)-$(LOCAL_VERSION).tar

.PHONY: tar

include build/make/shared.mk
