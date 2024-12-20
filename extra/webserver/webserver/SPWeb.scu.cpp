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

#include "SPCommon.h"

#include "SPWebInfo.cc"
#include "SPWebDbd.cc"
#include "SPWebAsyncTask.cc"
#include "SPWebHost.cc"
#include "SPWebHostComponent.cc"
#include "SPWebWasmComponent.cc"
#include "SPWebHostController.cc"
#include "SPWebRoot.cc"
#include "SPWebRequestFilter.cc"
#include "SPWebRequest.cc"
#include "SPWebRequestController.cc"
#include "SPWebRequestHandler.cc"
#include "SPWebInputFilter.cc"
#include "SPWebMultipartParser.cc"
#include "SPWebSession.cc"

#include "SPWebResource.cc"
#include "SPWebResourceResolver.cc"
#include "SPWebResourceHandler.cc"

#include "SPWebOutput.cc"
#include "SPWebVirtualFile.cc"

#include "SPWebWebsocket.cc"
#include "SPWebWebsocketConnection.cc"
#include "SPWebWebsocketManager.cc"

#include "SPWebTools.cc"
#include "SPWebToolsAuth.cc"
#include "SPWebToolsErrors.cc"
#include "SPWebToolsShell.cc"
#include "SPWebToolsServer.cc"

#include "SPWebWasm.cc"

namespace STAPPLER_VERSIONIZED stappler::web::config {

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

const char * getWebserverVersionString() {
	return TOSTRING(WEBSERVER_VERSION_API) "/" TOSTRING(WEBSERVER_VERSION_REV);
}

uint32_t getWebserverVersionApi() {
	return WEBSERVER_VERSION_API;
}

uint32_t getWebserverVersionRev() {
	return WEBSERVER_VERSION_REV;
}

}

