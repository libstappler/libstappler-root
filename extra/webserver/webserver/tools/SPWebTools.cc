/**
 Copyright (c) 2024 Stappler LLC <admin@stappler.dev>

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 **/

#include "SPWebTools.h"
#include "SPWebRequestHandler.h"
#include "SPWebVirtualFile.h"
#include "SPWebOutput.h"

namespace STAPPLER_VERSIONIZED stappler::web::tools {

void registerTools(StringView prefix, Host &host) {
	host.addHandler(prefix, RequestHandler::Make<tools::ServerGui>());
	host.addHandler(toString(prefix, config::TOOLS_SHELL), RequestHandler::Make<tools::ShellGui>());
	host.addHandler(toString(prefix, config::TOOLS_ERRORS), RequestHandler::Make<tools::ErrorsGui>());
	host.addHandler(toString(prefix, config::TOOLS_HANDLERS), RequestHandler::Make<tools::HandlersGui>());
	host.addHandler(toString(prefix, config::TOOLS_REPORTS), RequestHandler::Make<tools::ReportsGui>());
	host.addWebsocket(toString(prefix, config::TOOLS_SHELL_SOCKET), new tools::ShellSocket(host));

	host.addHandler(toString(prefix, config::TOOLS_AUTH), RequestHandler::Make<tools::AuthHandler>());
	host.addHandler(toString(prefix, config::TOOLS_VIRTUALFS), RequestHandler::Make<tools::VirtualFilesystem>());
}

Status VirtualFilesystem::onTranslateName(Request &rctx) {
	if (rctx.getInfo().method != RequestMethod::Get) {
		return DECLINED;
	}

	auto d = VirtualFile::getList();
	for (auto &it : d) {
		if (_subPath == it.name) {
			if (_subPath.ends_with(".js")) {
				rctx.setContentType("application/javascript");
			} else if (_subPath.ends_with(".css")) {
				rctx.setContentType("text/css");
			} else if (_subPath.ends_with(".html")) {
				rctx.setContentType("text/html;charset=UTF-8");
			}

			if (output::checkCacheHeaders(rctx, getCompileUnixTime(), hash::hash32(it.name.data(), it.name.size()))) {
				return HTTP_NOT_MODIFIED;
			}

			rctx << it.content;
			return DONE;
			break;
		}
	}

	return HTTP_NOT_FOUND;
}

}
