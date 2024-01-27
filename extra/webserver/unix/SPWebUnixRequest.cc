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

#include "SPWebUnixRequest.h"
#include "SPWebInputFilter.h"

namespace stappler::web {

UnixRequestController::UnixRequestController(pool_t *pool, RequestInfo &&info, ConnectionWorker::Client *client)
: RequestController(pool, move(info)) {
	_client = client;

	_info.useragentIp = client->addr;
	_info.useragentPort = client->port;
}

void UnixRequestController::startResponseTransmission() {

}

size_t UnixRequestController::getBytesSent() const {
	return _client->bytesSent;
}

void UnixRequestController::putc(int c) {
	uint8_t ch = c;
	_client->write(_client->response, &ch, 1);
}

size_t UnixRequestController::write(const uint8_t *buf, size_t size) {
	_client->write(_client->response, buf, size);
	return size;
}

void UnixRequestController::flush() { }

bool UnixRequestController::isSecureConnection() const {
	return false;
}

void UnixRequestController::setDocumentRoot(StringView val) {
	_info.documentRoot = val.pdup(_pool);
}

void UnixRequestController::setContentType(StringView val) {
	_info.contentType = val.pdup(_pool);
}

void UnixRequestController::setHandler(StringView val) {
	_info.handler = val.pdup(_pool);
}

void UnixRequestController::setContentEncoding(StringView val) {
	_info.contentEncoding = val.pdup(_pool);
}

void UnixRequestController::setStatus(Status status, StringView val) {
	_info.status = status;
	if (!val.empty()) {
		_info.statusLine = val.pdup(_pool);
	} else {
		_info.statusLine = getStatusLine(Status(status));
	}
}

StringView UnixRequestController::getCookie(StringView name, bool removeFromHeadersTable) {
	return StringView();
}

void UnixRequestController::setFilename(StringView val, bool updateStat, Time mtime) {
	if (!val.starts_with(_info.documentRoot)) {
		auto path = filepath::merge<Interface>(_info.documentRoot, val);
		if (filesystem::exists(path)) {
			val = StringView(path).pdup(_pool);
		} else {
			return;
		}
	} else {
		if (filesystem::exists(val)) {
			val = val.pdup(_pool);
		} else {
			return;
		}
	}

	_info.filename = val;
	if (updateStat) {
		filesystem::stat(_info.filename, _info.stat);
		if (mtime) {
			_info.stat.mtime = mtime;
		}
	}
}

StringView UnixRequestController::getRequestHeader(StringView key) const {
	auto tmp = key.str<memory::StandartInterface>();
	string::apply_tolower_c(tmp);

	auto it = _requestHeaders.find(StringView(tmp));
	if (it != _requestHeaders.end()) {
		return it->second;
	}
	return StringView();
}

void UnixRequestController::foreachRequestHeaders(const Callback<void(StringView, StringView)> &cb) const {
	for (auto &it : _requestHeaders) {
		cb(it.first, it.second);
	}
}

void UnixRequestController::setRequestHeader(StringView key, StringView val) {
	auto tmp = key.str<memory::StandartInterface>();
	string::apply_tolower_c(tmp);

	auto it = _requestHeaders.find(StringView(tmp));
	if (it != _requestHeaders.end()) {
		it->second = val.pdup(_pool);
	} else {
		it = _requestHeaders.emplace(StringView(tmp).pdup(_pool), val.pdup(_pool)).first;
	}

	if (it->first == "host") {
		StringView r(it->second);
		auto h = r.readUntil<StringView::Chars<':'>>();
		_info.url.host = h;
		if (r.is(':')) {
			++ r;
			_info.url.port = r;
		}
	} else if (it->first == "content-length") {
		_info.contentLength = StringView(it->second).readInteger(10).get(0);
	}
}

StringView UnixRequestController::getResponseHeader(StringView key) const {
	auto tmp = key.str<memory::StandartInterface>();
	string::apply_tolower_c(tmp);

	auto it = _responseHeaders.find(StringView(tmp));
	if (it != _responseHeaders.end()) {
		return it->second;
	}
	return StringView();
}

void UnixRequestController::foreachResponseHeaders(const Callback<void(StringView, StringView)> &cb) const {
	for (auto &it : _responseHeaders) {
		cb(it.first, it.second);
	}
}

void UnixRequestController::setResponseHeader(StringView key, StringView val) {
	auto tmp = key.str<memory::StandartInterface>();
	string::apply_tolower_c(tmp);

	auto it = _responseHeaders.find(StringView(tmp));
	if (it != _responseHeaders.end()) {
		it->second = val.pdup(_pool);
	} else {
		_responseHeaders.emplace(StringView(tmp).pdup(_pool), val.pdup(_pool));
	}
}

StringView UnixRequestController::getErrorHeader(StringView key) const {
	auto tmp = key.str<memory::StandartInterface>();
	string::apply_tolower_c(tmp);

	auto it = _errorHeaders.find(StringView(tmp));
	if (it != _errorHeaders.end()) {
		return it->second;
	}
	return StringView();
}

void UnixRequestController::foreachErrorHeaders(const Callback<void(StringView, StringView)> &cb) const {
	for (auto &it : _errorHeaders) {
		cb(it.first, it.second);
	}
}

void UnixRequestController::setErrorHeader(StringView key, StringView val) {
	auto tmp = key.str<memory::StandartInterface>();
	string::apply_tolower_c(tmp);

	auto it = _errorHeaders.find(StringView(tmp));
	if (it != _errorHeaders.end()) {
		it->second = val.pdup(_pool);
	} else {
		_errorHeaders.emplace(StringView(tmp).pdup(_pool), val.pdup(_pool));
	}
}

Status UnixRequestController::processInput(ConnectionWorker::BufferChain &chain) {
	if (!_filter) {
		return DECLINED;
	}

	auto ret = chain.read([&] (const ConnectionWorker::Buffer *, const uint8_t *data, size_t len) {
		auto size = std::min(len, size_t(_info.contentLength));
		StringView r((const char *)data, size);

		_info.contentLength -= size;

		_filter->step(r);

		if (_info.contentLength > 0) {
			return int(size);
		}

		_filter->finalize();
		return int(DONE);
	}, true);

	return _info.contentLength > 0 ? SUSPENDED : ret;
}

void UnixRequestController::submitResponse(Status status) {
	if (status > OK) {
		setStatus(status, StringView());
	}

	if (_info.status < HTTP_OK) {
		switch (status) {
		case DONE:
			setStatus(HTTP_OK, StringView());
			break;
		case OK:
			setStatus(HTTP_OK, StringView());
			break;
		case SUSPENDED:
		case DECLINED:
			setStatus(HTTP_BAD_REQUEST, StringView());
			break;
		default:
			break;
		}
	}

	if (!_info.filename.empty()) {
		_client->writeFile(_client->response, _info.filename, 0, _info.stat.size, ConnectionWorker::Buffer::Eos);
	}

	if (_client->response.empty()) {
		// create default response
		auto result = getDefaultResult();
		bool allowCbor = isAcceptable("application/cbor") > 0.0f;

		auto data = data::write<Interface>(result, allowCbor ? data::EncodeFormat::Cbor : data::EncodeFormat::Json);

		_info.contentType = (allowCbor ? StringView("application/json; charset=utf-8") : StringView("application/cbor"));

		_client->response.write(_client->pool, data.data(), data.size(), ConnectionWorker::Buffer::Eos);
	} else {
		_client->response.write(_client->pool, nullptr, 0, ConnectionWorker::Buffer::Eos);
	}

	Time date = Time::now();
	auto contentLength = _client->response.size();

	StringView crlf("\r\n");
	StringView statusLine = getStatusLine(status);
	if (statusLine.empty()) {
		statusLine = getStatusLine(HTTP_INTERNAL_SERVER_ERROR);
	}

	sp_time_exp_t xt(date);
	char dateBuf[30] = { 0 };
	xt.encodeRfc822(dateBuf);

	auto outFn = [&] (StringView str) {
		_client->write(_client->output, str);
	};

	auto out = Callback<void(StringView)>(outFn);

	out << StringView("HTTP/1.1 ") << statusLine << crlf;
	if (_info.status >= HTTP_BAD_REQUEST) {
		setErrorHeader("Date", dateBuf);
		setErrorHeader("Connection", "close");
		setErrorHeader("Server", _host->getRoot()->getServerNameLine());
		setErrorHeader("Content-Type", _info.contentType);
		setErrorHeader("Content-Encoding", _info.contentEncoding);
		setErrorHeader("Content-Length", toString(contentLength));

		for (auto &it : _errorHeaders) {
			out << it.first << StringView(": ") << it.second << crlf;
		}
	} else {
		setResponseHeader("Date", dateBuf);
		setResponseHeader("Connection", "close");
		setResponseHeader("Server", _host->getRoot()->getServerNameLine());
		setResponseHeader("Content-Type", _info.contentType);
		setResponseHeader("Content-Encoding", _info.contentEncoding);
		setResponseHeader("Content-Length", toString(contentLength));

		for (auto &it : _errorHeaders) {
			auto iit = _responseHeaders.find(it.first);
			if (iit == _responseHeaders.end()) {
				_responseHeaders.emplace(it.first, it.second);
			}
		}

		for (auto &it : _responseHeaders) {
			out << it.first << StringView(": ") << it.second << crlf;
		}
	}

	out << crlf;

	_client->write(_client->output, _client->response);
}

}
