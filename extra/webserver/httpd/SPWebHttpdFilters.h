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

#ifndef EXTRA_WEBSERVER_HTTPD_SPWEBHTTPDFILTERS_H_
#define EXTRA_WEBSERVER_HTTPD_SPWEBHTTPDFILTERS_H_

#include "SPWebInputFilter.h"
#include "SPWebHttpdTable.h"

#include "util_filter.h"

namespace STAPPLER_VERSIONIZED stappler::web {

class SP_PUBLIC HttpdInputFilter : public InputFilter {
public:
	static constexpr auto Name = "stappler::web::HttpdInputFilter";

	static void filterRegister();

	HttpdInputFilter(const Request &, Accept a);

protected:
	static apr_status_t filterFunc(ap_filter_t *f, apr_bucket_brigade *bb,
			ap_input_mode_t mode, apr_read_type_e block, apr_off_t readbytes);

	static int filterInit(ap_filter_t *f);

	apr_status_t func(ap_filter_t *f, apr_bucket_brigade *bb,
			ap_input_mode_t mode, apr_read_type_e block, apr_off_t readbytes);
};

class SP_PUBLIC HttpdOutputFilter : public AllocBase {
public:
	static constexpr auto Name = "stappler::web::HttpdOutputFilter";

	static void filterRegister();
	static void insert(const Request &);

	HttpdOutputFilter(const Request &rctx);

	apr_status_t func(ap_filter_t *f, apr_bucket_brigade *bb);
	int init(ap_filter_t *f);

	bool readRequestLine(StringView & r);
	bool readHeaders(StringView & r);

protected:
	static apr_status_t filterFunc(ap_filter_t *f, apr_bucket_brigade *bb);
	static int filterInit(ap_filter_t *f);

	apr_status_t process(ap_filter_t* f, apr_bucket *e, const char *data, size_t len);
	apr_status_t outputHeaders(ap_filter_t* f, apr_bucket *e, const char *data, size_t len);

	size_t calcHeaderSize() const;
	void writeHeader(ap_filter_t* f, StringStream &) const;

	apr_bucket_brigade *_tmpBB;
	bool _hookErrors = true;
	bool _seenEOS = false;
	bool _skipFilter = false;
	Request _request;

	enum class State {
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

	State _state = State::FirstLine;
	State _subState = State::Protocol;

	char _char, _buf;
	bool _isWhiteSpace;

	int64_t _responseCode = 0;

	httpd::table _headers;

	StringStream _nameBuffer;
	StringStream _buffer;
	StringStream _headersBuffer;
	apr_bucket *_bucket = nullptr;
	String _responseLine;
	String _statusText;
};


}

#endif /* EXTRA_WEBSERVER_HTTPD_SPWEBHTTPDFILTERS_H_ */
