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

MODULE_STAPPLER_WEBSERVER_UNIX_DEFINED_IN := $(TOOLKIT_MODULE_PATH)
MODULE_STAPPLER_WEBSERVER_UNIX_PRECOMPILED_HEADERS :=
MODULE_STAPPLER_WEBSERVER_UNIX_SRCS_DIRS := $(WEBSERVER_MODULE_DIR)/unix
MODULE_STAPPLER_WEBSERVER_UNIX_SRCS_OBJS :=
MODULE_STAPPLER_WEBSERVER_UNIX_INCLUDES_DIRS := $(WEBSERVER_MODULE_DIR)/unix
MODULE_STAPPLER_WEBSERVER_UNIX_INCLUDES_OBJS :=
MODULE_STAPPLER_WEBSERVER_UNIX_DEPENDS_ON := stappler_webserver_webserver

#spec

MODULE_STAPPLER_WEBSERVER_UNIX_SHARED_SPEC_SUMMARY := libstappler webserver implementation on unix sockets

define MODULE_STAPPLER_WEBSERVER_UNIX_SHARED_SPEC_DESCRIPTION
Module libstappler-webserver-unix implements simple webserver for debugging purpuses
endef

# module name resolution
MODULE_stappler_webserver_unix := MODULE_STAPPLER_WEBSERVER_UNIX
