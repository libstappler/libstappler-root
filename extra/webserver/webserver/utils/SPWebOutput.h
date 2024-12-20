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

#ifndef EXTRA_WEBSERVER_WEBSERVER_UTILS_SPWEBOUTPUT_H_
#define EXTRA_WEBSERVER_WEBSERVER_UTILS_SPWEBOUTPUT_H_

#include "SPWebRequest.h"

namespace STAPPLER_VERSIONIZED stappler::web::output {

SP_PUBLIC void formatJsonAsHtml(const Callback<void(StringView)> &stream, const Value &, bool actionHandling = false);

SP_PUBLIC void writeData(Request &rctx, const Value &, bool allowJsonP = true);
SP_PUBLIC void writeData(Request &rctx, const Callback<void(StringView)> &stream, const Callback<void(StringView)> &ct,
		const Value &, bool allowJsonP = true);

SP_PUBLIC Status writeResourceFileData(Request &rctx, Value &&);
SP_PUBLIC Status writeResourceData(Request &rctx, Value &&, Value && origin);

SP_PUBLIC Status writeResourceFileHeader(Request &rctx, const Value &);

// write file headers with respect for cache headers (if-none-match, if-modified-since)
// returns true if we should write file data or false if we should return HTTP_NOT_MODIFIED
SP_PUBLIC bool writeFileHeaders(Request &rctx, const Value &, StringView convertType = StringView());

SP_PUBLIC String makeEtag(uint32_t idHash, Time mtime);

// returns true if requested entity is matched "if-none-match" and "if-modified-since"
// suggest HTTP_NOT_MODIFIED if true
SP_PUBLIC bool checkCacheHeaders(Request &rctx, Time, const StringView &etag);

// shortcut for checkCacheHeaders + makeEtag;
SP_PUBLIC bool checkCacheHeaders(Request &rctx, Time, uint32_t idHash);

}

#endif /* EXTRA_WEBSERVER_WEBSERVER_UTILS_SPWEBOUTPUT_H_ */
