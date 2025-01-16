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

#include "SPWebRequestFilter.h"

namespace STAPPLER_VERSIONIZED stappler::web {

size_t RequestFilter::readRequestLine(StringView &r, RequestInfo &req) {
	auto source = r;
	r.skipChars<StringView::WhiteSpace>();
	auto methodName = r.readChars<StringView::LatinUppercase>();
	auto method = getRequestMethod(methodName);
	if (method == RequestMethod::Invalid) {
		return 0;
	}

	r.skipChars<StringView::WhiteSpace>();

	auto unparsedUrl = r;

	if (!req.url.parse(r)) {
		return 0;
	}

	unparsedUrl = StringView(unparsedUrl.data(), unparsedUrl.size() - r.size());

	r.skipChars<StringView::WhiteSpace>();

	StringView protocolName = r.readChars<StringView::LatinUppercase>();
	StringView protocolVersion;
	if (r.is('/')) {
		++ r;
		protocolVersion = r.readChars<StringView::Alphanumeric, StringView::Chars<'.'>>();
	}

	if (protocolName != "HTTP") {
		return 0;
	}

	protocolName = StringView(protocolName.data(), r.data() - protocolName.data());

	if (!r.is('\r')) {
		return 0;
	}

	++ r;
	if (r.is('\n')) {
		++ r;
	}

	if (methodName == "HEAD") {
		req.headerRequest = true;
	}

	req.requestTime = Time::now();
	req.method = method;
	req.methodName = methodName;
	req.protocol = protocolName;
	req.protocolVersion = getProtocolVersionNumber(protocolVersion);
	req.unparserUri = unparsedUrl;

	return r.data() - source.data();
}

size_t RequestFilter::readRequestHeader(BytesView &source, StringView &key, StringView &value) {
	auto tmp = source;
	auto keyData = source.readUntil<uint8_t(':')>();
	key = keyData.toStringView();
	if (!source.is(':') || key.size() != keyData.size()) {
		return 0;
	}

	++ source;

	auto valueData = source.readUntil<uint8_t('\n')>();
	value = valueData.toStringView();
	if (!source.is('\n') || value.size() != valueData.size()) {
		return 0;
	}

	key.trimChars<StringView::WhiteSpace>();
	value.trimChars<StringView::WhiteSpace>();
	return source.data() - tmp.data();
}

/*
RequestFilter::RequestFilter() { }

bool RequestFilter::readRequestLine(StringView &r) {
	while (!r.empty()) {
		if (_isWhiteSpace) {
			auto tmp = r.readChars<Reader::CharGroup<CharGroupId::WhiteSpace>>();
			_buffer << tmp;
			auto tmp2 = tmp.readUntil<Reader::Chars<'\n'>>();
			if (tmp2.is('\n')) {
				return true;
			}
			if (r.empty()) {
				return false;
			}
			_isWhiteSpace = false;
		}
		if (_subState == RequestState::Protocol) {
			_buffer << r.readUntil<Reader::CharGroup<CharGroupId::WhiteSpace>>();
			if (r.is<CharGroupId::WhiteSpace>()) {
				_nameBuffer.clear();
				_isWhiteSpace = true;
				_subState = State::Code;
			}
		} else if (_subState == RequestState::Code) {
			auto b = r.readChars<Reader::Range<'0', '9'>>();
			_nameBuffer << b;
			_buffer << b;
			if (r.is<CharGroupId::WhiteSpace>()) {
				_nameBuffer << '\0';
				_responseCode = apr_strtoi64(_nameBuffer.data(), NULL, 0);
				_isWhiteSpace = true;
				_subState = State::Status;
			}
		} else if (_subState == RequestState::Status) {
			auto statusText = r.readUntil<Reader::Chars<'\n'>>();
			_statusText = statusText.str();
			string::trim(_statusText);
			_buffer << statusText;
		} else {
			r.readUntil<Reader::Chars<'\n', '\r'>>();
		}
		if (r.is('\n') || r.is('\r')) {
			++ r;
			_buffer << r.readChars<Reader::Chars<'\n', '\r'>>();
			_state = State::Headers;
			_subState = State::HeaderName;
			_isWhiteSpace = true;
			return true;
		}
	}
	return false;
}

bool RequestFilter::readHeaders(StringView &r) {
	while (!r.empty()) {
		if (_subState == State::HeaderName) {
			_nameBuffer << r.readUntil<Reader::Chars<':', '\n'>>();
			if (r.is('\n')) {
				r ++;
				_state = State::Body;
				return true;
			} else if (r.is<':'>()) {
				r ++;
				_isWhiteSpace = true;
				_subState = State::HeaderValue;
			}
		} else if (_subState == State::HeaderValue) {
			_buffer << r.readUntil<Reader::Chars<'\n'>>();
			if (r.is('\n')) {
				r ++;

				auto key = _nameBuffer.str(); string::trim(key);
				auto value = _buffer.str(); string::trim(value);

				_headers.emplace(sp::move(key), sp::move(value));

				_buffer.clear();
				_nameBuffer.clear();
				_subState = State::HeaderName;
				_isWhiteSpace = true;
			}
		}
	}
	return false;
}*/

}
