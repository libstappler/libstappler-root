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

#include "SPWebInfo.h"

namespace stappler::web {

void SessionInfo::init(const Value &val) {
	name = val.getString("name");
	key = val.getString("key");
	maxAge = TimeInterval(val.getInteger("maxage"));
	secure = val.getBool("secure");
}

void SessionInfo::setParam(StringView n, StringView v) {
	if (n.is("name")) {
		name = v.str<Interface>();
	} else if (n.is("key")) {
		key = v.str<Interface>();
	} else if (n.is("maxage")) {
		maxAge = v.readInteger().get(0);
	} else if (n.is("secure")) {
		if (v.is("true") || v.is("on") || v.is("On")) {
			secure = true;
		} else if (v.is("false") || v.is("off") || v.is("Off")) {
			secure = false;
		}
	}
}

void WebhookInfo::init(const Value &val) {
	name = val.getString("name");
	url = val.getString("url");
	format = val.getString("format");
	extra = val.getValue("extra");
}

void WebhookInfo::setParam(StringView n, StringView v) {
	if (n.is("name")) {
		name = v.str<Interface>();
	} else if (n.is("url")) {
		url = v.str<Interface>();
	} else if (n.is("format")) {
		format = v.str<Interface>();
	} else {
		extra.setString(v.str<Interface>(), n.str<Interface>());
	}
}

RequestInfo RequestInfo::clone(pool_t *pool) {
	return perform([&] {
		RequestInfo ret(*this);

		auto dupString = [&] (StringView &str) {
			if (!str.empty()) {
				str = str.pdup(pool);
			}
		};

		dupString(ret.requestLine);
		dupString(ret.protocol);
		dupString(ret.methodName);
		dupString(ret.statusLine);
		dupString(ret.rangeLine);
		dupString(ret.documentRoot);
		dupString(ret.contentEncoding);
		dupString(ret.authType);
		dupString(ret.unparserUri);
		dupString(ret.filename);
		dupString(ret.handler);

		dupString(ret.url.scheme);
		dupString(ret.url.user);
		dupString(ret.url.password);
		dupString(ret.url.host);
		dupString(ret.url.port);
		dupString(ret.url.path);
		dupString(ret.url.query);
		dupString(ret.url.fragment);
		dupString(ret.url.url);

		if (!ret.queryPath.empty()) {
			for (auto &it : ret.queryPath) {
				dupString(const_cast<StringView &>(it));
			}
			ret.queryPath = ret.queryPath.pdup(pool);
		}

		return ret;
	}, pool);
}

RequestMethod getRequestMethod(StringView method) {
	if (method == "GET") { return RequestMethod::Get; }
	else if (method == "HEAD") { return RequestMethod::Get; }
	else if (method == "PUT") { return RequestMethod::Put; }
	else if (method == "POST") { return RequestMethod::Post; }
	else if (method == "DELETE") { return RequestMethod::Delete; }
	else if (method == "CONNECT") { return RequestMethod::Connect; }
	else if (method == "OPTIONS") { return RequestMethod::Options; }
	else if (method == "TRACE") { return RequestMethod::Trace; }
	else if (method == "PATCH") { return RequestMethod::Patch; }
	else if (method == "PROPFIND") { return RequestMethod::Propfind; }
	else if (method == "PROPPATCH") { return RequestMethod::Proppatch; }
	else if (method == "MKCOL") { return RequestMethod::MkCol; }
	else if (method == "COPY") { return RequestMethod::Copy; }
	else if (method == "MOVE") { return RequestMethod::Move; }
	else if (method == "LOCK") { return RequestMethod::Lock; }
	else if (method == "UNLOCK") { return RequestMethod::Unlock; }
	else if (method == "VERSIONCONTROL") { return RequestMethod::VersionControl; }
	else if (method == "CHECKOUT") { return RequestMethod::Checkout; }
	else if (method == "UNCHECKOUT") { return RequestMethod::Uncheckout; }
	else if (method == "CHECKIN") { return RequestMethod::Checkin; }
	else if (method == "UPDATE") { return RequestMethod::Update; }
	else if (method == "LABEL") { return RequestMethod::Label; }
	else if (method == "REPORT") { return RequestMethod::Report; }
	else if (method == "MKWORKSPACE") { return RequestMethod::MkWorkspace; }
	else if (method == "MKACTIVITY") { return RequestMethod::MkActivity; }
	else if (method == "BASELINECONTROL") { return RequestMethod::BaselineControl; }
	else if (method == "MERGE") { return RequestMethod::Merge; }

	return RequestMethod::Invalid;
}

uint32_t getProtocolVersionNumber(StringView str) {
	uint32_t ret = 0;
	if (str.is<StringView::Numbers>()) {
		ret += str.readInteger(10).get(0) * 1000;
	}
	if (str.is('.')) {
		++ str;
		ret += str.readInteger(10).get(0);
	}
	return ret;
}

StringView extractCharset(StringView s) {
	s.skipUntil<StringView::Chars<';'>>();
	if (!s.is(';')) {
		return StringView();
	}

	++ s;

	s.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
	while (!s.empty()) {
		StringView name;
		StringView value;

		name = s.readUntil<StringView::Chars<'='>>();
		name.trimChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
		if (s.is('=')) {
			++ s;
			s.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
			if (s.is('"')) {
				while (s.is('"')) {
					++ s;
					s.skipUntil<StringView::Chars<'"'>>();
					if (s.is('"')) {
						++ s;
						s.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
					}
				}
				s.skipUntil<StringView::Chars<';'>>();
				if (s.is(';')) {
					value = StringView(name.data() + name.size() + 1, s.data() - (name.data() + name.size() + 1));
				}
			} else {
				value = s.readUntil<StringView::Chars<';'>>();
			}
		}

		if (name == "charset" && !value.empty()) {
			return value;
		}
	}
	return StringView();
}

StringView getStatusLine(Status status) {
	switch (status) {
	case HTTP_CONTINUE: return StringView("100 Continue"); break;
	case HTTP_SWITCHING_PROTOCOLS: return StringView("101 Switching Protocols"); break;
	case HTTP_PROCESSING: return StringView("102 Processing"); break;
	case HTTP_OK: return StringView("200 OK"); break;
	case HTTP_CREATED: return StringView("201 Created"); break;
	case HTTP_ACCEPTED: return StringView("202 Accepted"); break;
	case HTTP_NON_AUTHORITATIVE: return StringView("203 Non-Authoritative Information"); break;
	case HTTP_NO_CONTENT: return StringView("204 No Content"); break;
	case HTTP_RESET_CONTENT: return StringView("205 Reset Content"); break;
	case HTTP_PARTIAL_CONTENT: return StringView("206 Partial Content"); break;
	case HTTP_MULTI_STATUS: return StringView("207 Multi-Status"); break;
	case HTTP_ALREADY_REPORTED: return StringView("208 Already Reported"); break;
	case HTTP_IM_USED: return StringView("226 IM Used"); break;
	case HTTP_MULTIPLE_CHOICES: return StringView("300 Multiple Choices"); break;
	case HTTP_MOVED_PERMANENTLY: return StringView("301 Moved Permanently"); break;
	case HTTP_MOVED_TEMPORARILY: return StringView("302 Found"); break;
	case HTTP_SEE_OTHER: return StringView("303 See Other"); break;
	case HTTP_NOT_MODIFIED: return StringView("304 Not Modified"); break;
	case HTTP_USE_PROXY: return StringView("305 Use Proxy"); break;
	case HTTP_TEMPORARY_REDIRECT: return StringView("307 Temporary Redirect"); break;
	case HTTP_PERMANENT_REDIRECT: return StringView("308 Permanent Redirect"); break;
	case HTTP_BAD_REQUEST: return StringView("400 Bad Request"); break;
	case HTTP_UNAUTHORIZED: return StringView("401 Unauthorized"); break;
	case HTTP_PAYMENT_REQUIRED: return StringView("402 Payment Required"); break;
	case HTTP_FORBIDDEN: return StringView("403 Forbidden"); break;
	case HTTP_NOT_FOUND: return StringView("404 Not Found"); break;
	case HTTP_METHOD_NOT_ALLOWED: return StringView("405 Method Not Allowed"); break;
	case HTTP_NOT_ACCEPTABLE: return StringView("406 Not Acceptable"); break;
	case HTTP_PROXY_AUTHENTICATION_REQUIRED: return StringView("407 Proxy Authentication Required"); break;
	case HTTP_REQUEST_TIME_OUT: return StringView("408 Request Timeout"); break;
	case HTTP_CONFLICT: return StringView("409 Conflict"); break;
	case HTTP_GONE: return StringView("410 Gone"); break;
	case HTTP_LENGTH_REQUIRED: return StringView("411 Length Required"); break;
	case HTTP_PRECONDITION_FAILED: return StringView("412 Precondition Failed"); break;
	case HTTP_REQUEST_ENTITY_TOO_LARGE: return StringView("413 Request Entity Too Large"); break;
	case HTTP_REQUEST_URI_TOO_LARGE: return StringView("414 Request-URI Too Long"); break;
	case HTTP_UNSUPPORTED_MEDIA_TYPE: return StringView("415 Unsupported Media Type"); break;
	case HTTP_RANGE_NOT_SATISFIABLE: return StringView("416 Requested Range Not Satisfiable"); break;
	case HTTP_EXPECTATION_FAILED: return StringView("417 Expectation Failed"); break;
	case HTTP_MISDIRECTED_REQUEST: return StringView("421 Misdirected Request"); break;
	case HTTP_UNPROCESSABLE_ENTITY: return StringView("422 Unprocessable Entity"); break;
	case HTTP_LOCKED: return StringView("423 Locked"); break;
	case HTTP_FAILED_DEPENDENCY: return StringView("424 Failed Dependency"); break;
	case HTTP_UPGRADE_REQUIRED: return StringView("426 Upgrade Required"); break;
	case HTTP_PRECONDITION_REQUIRED: return StringView("428 Precondition Required"); break;
	case HTTP_TOO_MANY_REQUESTS: return StringView("429 Too Many Requests"); break;
	case HTTP_REQUEST_HEADER_FIELDS_TOO_LARGE: return StringView("431 Request Header Fields Too Large"); break;
	case HTTP_UNAVAILABLE_FOR_LEGAL_REASONS: return StringView("451 Unavailable For Legal Reasons"); break;
	case HTTP_INTERNAL_SERVER_ERROR: return StringView("500 Internal Server Error"); break;
	case HTTP_NOT_IMPLEMENTED: return StringView("501 Not Implemented"); break;
	case HTTP_BAD_GATEWAY: return StringView("502 Bad Gateway"); break;
	case HTTP_SERVICE_UNAVAILABLE: return StringView("503 Service Unavailable"); break;
	case HTTP_GATEWAY_TIME_OUT: return StringView("504 Gateway Timeout"); break;
	case HTTP_VERSION_NOT_SUPPORTED: return StringView("505 HTTP Version Not Supported"); break;
	case HTTP_VARIANT_ALSO_VARIES: return StringView("506 Variant Also Negotiates"); break;
	case HTTP_INSUFFICIENT_STORAGE: return StringView("507 Insufficient Storage"); break;
	case HTTP_LOOP_DETECTED: return StringView("508 Loop Detected"); break;
	case HTTP_NOT_EXTENDED: return StringView("510 Not Extended"); break;
	case HTTP_NETWORK_AUTHENTICATION_REQUIRED: return StringView("511 Network Authentication Required"); break;
	default: break;
	}
	return StringView();
}

}
