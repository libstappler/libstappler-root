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

MODULE_DOCUMENT_RICHTEXT_DEFINED_IN := $(TOOLKIT_MODULE_PATH)
MODULE_DOCUMENT_RICHTEXT_PRECOMPILED_HEADERS :=
MODULE_DOCUMENT_RICHTEXT_SRCS_DIRS := $(DOCUMENT_MODULE_DIR)/richtext
MODULE_DOCUMENT_RICHTEXT_SRCS_OBJS :=
MODULE_DOCUMENT_RICHTEXT_INCLUDES_DIRS := $(DOCUMENT_MODULE_DIR)/richtext
MODULE_DOCUMENT_RICHTEXT_INCLUDES_OBJS :=
MODULE_DOCUMENT_RICHTEXT_DEPENDS_ON := \
	stappler_document_layout \
	xenolith_renderer_material2d \
	xenolith_resources_assets

# module name resolution
MODULE_stappler_document_richtext := MODULE_DOCUMENT_RICHTEXT
