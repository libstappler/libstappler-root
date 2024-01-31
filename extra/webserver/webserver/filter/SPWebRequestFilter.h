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

#ifndef EXTRA_WEBSERVER_WEBSERVER_FILTER_SPWEBREQUESTFILTER_H_
#define EXTRA_WEBSERVER_WEBSERVER_FILTER_SPWEBREQUESTFILTER_H_

#include "SPWebInfo.h"

namespace STAPPLER_VERSIONIZED stappler::web {

class RequestFilter : public AllocBase {
public:
	enum class RequestState {
		None,
		FirstLine,
		Headers,
		Body,

		Protocol,
		Code,
		Status,

		HeaderName,
		HeaderValue,
	};

	static bool readRequestLine(StringView &source, RequestInfo &);
	static bool readRequestHeader(StringView &source, StringView &key, StringView &value);

	RequestFilter();

	bool readRequestLine(StringView &r);
	bool readHeaders(StringView & r);

protected:
	RequestState _state = RequestState::FirstLine;
	RequestState _subState = RequestState::Protocol;

	StringStream _nameBuffer;
	StringStream _buffer;
	StringStream _headersBuffer;
	String _responseLine;
	String _statusText;
};

}

#endif /* EXTRA_WEBSERVER_WEBSERVER_FILTER_SPWEBREQUESTFILTER_H_ */
