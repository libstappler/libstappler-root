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

#include "SPWebHttpdRequest.h"
#include "SPWebInputFilter.h"

namespace stappler::web {

void HttpdRequestController::readRequestInfo(request_rec *rec, RequestInfo &info) {
	info.requestTime = Time::microseconds(rec->request_time);
	info.method = RequestMethod(rec->method_number);
	info.protocolVersion = rec->proto_num;
	info.status = rec->status;
	info.contentLength = rec->clength;

	info.stat.isDir = (rec->finfo.filetype == APR_DIR) ? true : false;
	info.stat.size = rec->finfo.size;
	info.stat.mtime = Time::microseconds(rec->finfo.mtime);
	info.stat.ctime = Time::microseconds(rec->finfo.ctime);
	info.stat.atime = Time::microseconds(rec->finfo.atime);

	info.requestLine = StringView(rec->the_request);
	info.protocol = StringView(rec->protocol);
	info.methodName = StringView(rec->method);
	info.statusLine = StringView(rec->status_line);
	info.rangeLine = StringView(rec->range);
	info.documentRoot = StringView(ap_context_document_root(rec));
	info.contentType = StringView(rec->content_type);
	info.contentEncoding = StringView(rec->content_encoding);
	info.authType = StringView(rec->ap_auth_type);
	info.unparserUri = StringView(rec->unparsed_uri);
	info.filename = StringView(rec->filename);
	info.useragentIp = StringView(rec->useragent_ip);
	info.handler = StringView(rec->handler);
	info.url.scheme = StringView(rec->parsed_uri.scheme);
	info.url.user = StringView(rec->parsed_uri.user);
	info.url.password = StringView(rec->parsed_uri.password);
	info.url.host = StringView(rec->parsed_uri.hostname);
	info.url.port = StringView(rec->parsed_uri.port_str);
	info.url.path = StringView(rec->parsed_uri.path);
	info.url.query = StringView(rec->parsed_uri.query);
	info.url.fragment = StringView(rec->parsed_uri.fragment);

	info.headerRequest = rec->header_only ? true : false;
}

HttpdRequestController::HttpdRequestController(HostController *host, request_rec *req, RequestInfo &&info)
: RequestController(req->pool, move(info)) {
	_request = req;

	_requestHeaders = httpd::table::wrap(_request->headers_in);
	_responseHeaders = httpd::table::wrap(_request->headers_out);
	_errorHeaders = httpd::table::wrap(_request->err_headers_out);

	_host = host;
}

void HttpdRequestController::startResponseTransmission() {
	ap_send_interim_response(_request, 1);
}

size_t HttpdRequestController::getBytesSent() const {
	return _request->bytes_sent;
}

void HttpdRequestController::putc(int c) {
	ap_rputc(c, _request);
}

void HttpdRequestController::write(const uint8_t *buf, size_t size) {
	ap_rwrite((const void *)buf, size, _request);
}

void HttpdRequestController::flush() {
	ap_rflush(_request);
}

void HttpdRequestController::setDocumentRoot(StringView data) {
	ap_set_document_root(_request, data.terminated() ? data.data() : data.pdup(_pool).data());
}

void HttpdRequestController::setContentType(StringView data) {
	ap_set_content_type(_request, data.terminated() ? data.data() : data.pdup(_pool).data());
}

void HttpdRequestController::setHandler(StringView data) {
	_request->handler = data.terminated() ? data.data() : data.pdup(_pool).data();
	_info.handler = StringView(_request->handler);
}

void HttpdRequestController::setContentEncoding(StringView data) {
	_request->content_encoding = data.terminated() ? data.data() : data.pdup(_pool).data();
	_info.contentEncoding = StringView(_request->content_encoding);
}

void HttpdRequestController::setStatus(int status, StringView str) {
	_request->status = status;
	if (!str.empty()) {
		_request->status_line = str.pdup(_pool);
	} else {
		_request->status_line = ap_get_status_line(status);
	}

	_info.status = _request->status;
	_info.statusLine = StringView(_request->status_line);
}

void HttpdRequestController::setFilename(StringView data, bool updateStat, Time mtime) {
	_request->filename = data.terminated() ? data.data() : data.pdup(_pool).data();
	_request->canonical_filename = _request->filename;
	_info.filename = StringView(_request->filename);
	if (updateStat) {
		apr_stat(&_request->finfo, _request->filename, APR_FINFO_NORM, _request->pool);

		if (mtime) {
			_request->finfo.mtime = mtime.toMicroseconds();
		}
		_info.stat.mtime = Time::microseconds(_request->finfo.mtime);
		_info.stat.ctime = Time::microseconds(_request->finfo.ctime);
		_info.stat.atime = Time::microseconds(_request->finfo.atime);
		_info.stat.isDir = _request->finfo.filetype == APR_DIR;
		_info.stat.size = _request->finfo.size;

		ap_set_etag(_request);
		ap_set_last_modified(_request);
	}
}

StringView HttpdRequestController::getCookie(StringView name, bool removeFromHeadersTable) {
	const char *val = nullptr;
	if (ap_cookie_read(_request, name.data(), &val, int(removeFromHeadersTable)) == APR_SUCCESS) {
		if (val) {
			if (removeFromHeadersTable) {
				ap_unescape_urlencoded((char *)val);
				return StringView(val);
			} else {
				StringView ret(StringView(val).pdup(_pool));
				ap_unescape_urlencoded((char *)ret.data());
				return StringView(ret.data());
			}
		}
	}
	return StringView();
}

StringView HttpdRequestController::getRequestHeader(StringView key) const {
	return _requestHeaders.at(key);
}

StringView HttpdRequestController::getResponseHeader(StringView key) const {
	return _responseHeaders.at(key);
}

void HttpdRequestController::setResponseHeader(StringView key, StringView value) {
	_responseHeaders.emplace(key, value);
}

StringView HttpdRequestController::getErrorHeader(StringView key) const {
	return _errorHeaders.at(key);
}

void HttpdRequestController::setErrorHeader(StringView key, StringView value) {
	_responseHeaders.emplace(key, value);
}

void HttpdRequestController::setInputFilter(InputFilter *f) {
	ap_add_input_filter(f->getName().data(), (void *)f, _request, _request->connection);
	RequestController::setInputFilter(f);
}

}
