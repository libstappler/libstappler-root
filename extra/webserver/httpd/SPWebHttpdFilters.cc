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

#include "SPWebHttpdFilters.h"
#include "SPWebHttpdRequest.h"
#include "SPWebOutput.h"
#include "SPWebTools.h"
#include "SPWebRoot.h"

namespace STAPPLER_VERSIONIZED stappler::web {

void HttpdInputFilter::filterRegister() {
	ap_register_input_filter(Name, &(filterFunc),
			&(filterInit), AP_FTYPE_CONTENT_SET);
}

HttpdInputFilter::HttpdInputFilter(const Request &req, Accept a)
: InputFilter(req, a) { }

apr_status_t HttpdInputFilter::filterFunc(ap_filter_t *f, apr_bucket_brigade *bb,
		ap_input_mode_t mode, apr_read_type_e block, apr_off_t readbytes) {
	return perform([&] () -> apr_status_t {
		if (f->ctx) {
			HttpdInputFilter *filter = (HttpdInputFilter *) f->ctx;
			return filter->func(f, bb, mode, block, readbytes);
		} else {
			return ap_get_brigade(f->next, bb, mode, block, readbytes);
		}
	}, (pool_t *)f->r->pool, config::TAG_REQUEST, HttpdRequestController::get(f->r));
}

int HttpdInputFilter::filterInit(ap_filter_t *f) {
	return perform([&] () -> int {
		if (f->ctx) {
			HttpdInputFilter *filter = (HttpdInputFilter *) f->ctx;
			return filter->init();
		} else {
			return OK;
		}
	}, (pool_t *)f->r->pool, config::TAG_REQUEST, HttpdRequestController::get(f->r));
}

apr_status_t HttpdInputFilter::func(ap_filter_t *f, apr_bucket_brigade *bb,
		ap_input_mode_t mode, apr_read_type_e block, apr_off_t readbytes) {

	apr_status_t rv = ap_get_brigade(f->next, bb, mode, block, readbytes);

	if (rv == APR_SUCCESS && _parser && getConfig().required != db::InputConfig::Require::None && _accept != Accept::None) {
		apr_bucket *b = NULL;
		for (b = APR_BRIGADE_FIRST(bb); b != APR_BRIGADE_SENTINEL(bb); b = APR_BUCKET_NEXT(b)) {
			if (!_eos) {
				if (!APR_BUCKET_IS_METADATA(b)) {
					const char *str;
					size_t len;
					apr_bucket_read(b, &str, &len, block);
					step(StringView(str, len));
				} else if (APR_BUCKET_IS_EOS(b)) {
					finalize();
				}
			}
		}
	}

	return rv;
}

void HttpdOutputFilter::filterRegister() {
	ap_register_output_filter_protocol(Name, &(filterFunc),
			&(filterInit), (ap_filter_type)(AP_FTYPE_PROTOCOL),
			AP_FILTER_PROTO_CHANGE | AP_FILTER_PROTO_CHANGE_LENGTH);
}

void HttpdOutputFilter::insert(const Request &r) {
	perform([&] () {
		auto f = new (r.pool()) HttpdOutputFilter(r);
		auto req = ((HttpdRequestController *)r.config())->getRequest();
		ap_add_output_filter(Name, (void *)f, req, req->connection );
	}, r.pool(), config::TAG_REQUEST, r.config());
}

apr_status_t HttpdOutputFilter::filterFunc(ap_filter_t *f, apr_bucket_brigade *bb) {
	return perform([&] () -> apr_status_t {
		if (APR_BRIGADE_EMPTY(bb)) {
			return APR_SUCCESS;
		}
		if (f->ctx) {
			HttpdOutputFilter *filter = (HttpdOutputFilter *) f->ctx;
			return filter->func(f, bb);
		} else {
			return ap_pass_brigade(f->next,bb);
		}
	}, (pool_t *)f->r->pool, config::TAG_REQUEST, HttpdRequestController::get(f->r));
}

int HttpdOutputFilter::filterInit(ap_filter_t *f) {
	return perform([&] () -> apr_status_t {
		if (f->ctx) {
			HttpdOutputFilter *filter = (HttpdOutputFilter *) f->ctx;
			return filter->init(f);
		} else {
			return OK;
		}
	}, (pool_t *)f->r->pool, config::TAG_REQUEST, HttpdRequestController::get(f->r));
}

HttpdOutputFilter::HttpdOutputFilter(const Request &rctx)
: _headers((apr_pool_t *)rctx.pool()), _nameBuffer(64), _buffer(255) {
	_tmpBB = NULL;
	_seenEOS = false;
	_request = rctx;
	_char = 0;
	_buf = 0;
	_isWhiteSpace = true;
}

int HttpdOutputFilter::init(ap_filter_t* f) {
	_seenEOS = false;
	_skipFilter = false;
	return OK;
}

apr_status_t HttpdOutputFilter::func(ap_filter_t *f, apr_bucket_brigade *bb) {
	if (_seenEOS) {
		return APR_SUCCESS;
	}
	if (_skipFilter || _state == State::Body || f->r->proto_num == 9) {
		if (_tmpBB) {
			apr_brigade_destroy(_tmpBB);
			_tmpBB = nullptr;
		}
		return ap_pass_brigade(f->next, bb);
	}
	apr_bucket *e;
	const char *data = NULL;
	size_t len = 0;
	apr_status_t rv;

	if (!_tmpBB) {
		_tmpBB = apr_brigade_create(f->r->pool, f->c->bucket_alloc);
	}

	apr_read_type_e mode = APR_NONBLOCK_READ;

	while ((e = APR_BRIGADE_FIRST(bb)) != APR_BRIGADE_SENTINEL(bb)) {

        if (APR_BUCKET_IS_EOS(e)) {
			_seenEOS = true;
			APR_BUCKET_REMOVE(e);
            APR_BRIGADE_INSERT_TAIL(_tmpBB, e);
            break;
        }

        if (APR_BUCKET_IS_METADATA(e)) {
			APR_BUCKET_REMOVE(e);
            APR_BRIGADE_INSERT_TAIL(_tmpBB, e);
			continue;
		}
		if ((_responseCode < 400 || !_hookErrors) && _state == State::Body) {
			_skipFilter = true;
			APR_BUCKET_REMOVE(e);
			APR_BRIGADE_INSERT_TAIL(_tmpBB, e);
			break;
		}

		//APR_BUCKET_REMOVE(e);
		//APR_BRIGADE_INSERT_TAIL(_tmpBB, e);

		rv = apr_bucket_read(e, &data, &len, mode);
		if (rv == APR_EAGAIN && mode == APR_NONBLOCK_READ) {
			// Pass down a brigade containing a flush bucket:
			APR_BRIGADE_INSERT_TAIL(_tmpBB, apr_bucket_flush_create(_tmpBB->bucket_alloc));
			rv = ap_pass_brigade(f->next, _tmpBB);
			apr_brigade_cleanup(_tmpBB);
			if (rv != APR_SUCCESS) return rv;

			// Retry, using a blocking read.
			mode = APR_BLOCK_READ;
			continue;
		} else if (rv != APR_SUCCESS) {
			return rv;
		}

		if (rv == APR_SUCCESS) {
			rv = process(f, e, data, len);
			if (rv != APR_SUCCESS) {
				return rv;
			}
		}
		// Remove bucket e from bb.
		APR_BUCKET_REMOVE(e);
		apr_bucket_destroy(e);
		// Pass brigade downstream.
		rv = ap_pass_brigade(f->next, _tmpBB);
		apr_brigade_cleanup(_tmpBB);
		if (rv != APR_SUCCESS) return rv;

		mode = APR_NONBLOCK_READ;

		if (_state == State::Body) {
			break;
		}
	}

	if (!APR_BRIGADE_EMPTY(_tmpBB)) {
		rv = ap_pass_brigade(f->next, _tmpBB);
		apr_brigade_cleanup(_tmpBB);
		if (rv != APR_SUCCESS) return rv;
	}

	if (!APR_BRIGADE_EMPTY(bb)) {
		rv = ap_pass_brigade(f->next, bb);
		apr_brigade_cleanup(bb);
		if (rv != APR_SUCCESS) return rv;
	}

	return APR_SUCCESS;
}

apr_status_t HttpdOutputFilter::process(ap_filter_t* f, apr_bucket *e, const char *data, size_t len) {
	apr_status_t rv = APR_SUCCESS;

	if (len > 0 && _state != State::Body) {
		StringView reader(data, len);
		if (_state == State::FirstLine) {
			if (readRequestLine(reader)) {
				_responseLine = _buffer.str();
				_buffer.clear();
				_nameBuffer.clear();
			}
		}
		if (_state == State::Headers) {
			if (readHeaders(reader)) {
				rv = outputHeaders(f, e, data, len);
				if (rv != APR_SUCCESS) {
					return rv;
				}
			}
		}
		if (_state == State::Body) {
			if (!reader.empty()) {
				auto d = pool::palloc((pool_t *)f->r->pool, reader.size());
				memcpy(d, reader.data(), reader.size());

				APR_BRIGADE_INSERT_TAIL(_tmpBB, apr_bucket_pool_create(
						(const char *)d, reader.size(), f->r->pool, _tmpBB->bucket_alloc));
			}
		}
	}

	return rv;
}

size_t HttpdOutputFilter::calcHeaderSize() const {
	auto len = _responseLine.size() + 2;
	for (auto &it : _headers) {
		len += strlen(it.key) + strlen(it.val) + 4;
	}

	auto &cookies = _request.getResponseCookies();
	for (auto &it : cookies) {
		if ((_responseCode < 400 && (it.second.flags & CookieFlags::SetOnSuccess) != 0)
				|| (_responseCode >= 400 && (it.second.flags & CookieFlags::SetOnError) != 0)) {
			len += "Set-Cookie: "_len + it.first.size() + 1 + it.second.data.size() + ";Path=/;Version=1"_len + 4;
			if (it.second.data.empty() || it.second.maxAge) {
				len += ";Max-Age="_len + 10;
			}
			if ((it.second.flags & CookieFlags::HttpOnly) != 0) {
				len += ";HttpOnly"_len;
			}
			if ((it.second.flags & CookieFlags::Secure) != 0) {
				len += ";Secure;"_len;
			}
		}
	}

	return len;
}

void HttpdOutputFilter::writeHeader(ap_filter_t* f, StringStream &output) const {
	output << _responseLine;
	if (_responseLine.back() != '\n') {
		output << '\n';
	}
	for (auto &it : _headers) {
		output << it.key << ": " << it.val << "\r\n";
	}

	auto &cookies = _request.getResponseCookies();
	for (auto &it : cookies) {
		if ((_responseCode < 400 && (it.second.flags & CookieFlags::SetOnSuccess) != 0)
				|| (_responseCode >= 400 && (it.second.flags & CookieFlags::SetOnError) != 0) ) {
			output << "Set-Cookie: " << it.first << "=" << it.second.data;
			if (it.second.data.empty() || it.second.maxAge) {
				output << ";Max-Age=" << it.second.maxAge.toSeconds();
			}
			if ((it.second.flags & CookieFlags::HttpOnly) != 0) {
				output << ";HttpOnly";
			}
			auto sameSite = it.second.flags & CookieFlags::SameSiteStrict;
			switch (sameSite) {
			case CookieFlags::SameSiteNone:
				if ((it.second.flags & CookieFlags::Secure) != 0 && _request.isSecureConnection()) {
					output << ";SameSite=None";
				}
				break;
			case CookieFlags::SameSiteLux: output << ";SameSite=Lax"; break;
			case CookieFlags::SameSiteStrict: output << ";SameSite=Strict"; break;
			default: break;
			}
			if ((it.second.flags & CookieFlags::Secure) != 0 && _request.isSecureConnection()) {
				output << ";Secure";
			}
			output << ";Path=/;Version=1\r\n";
		}
	}

	output << "\r\n";
}

apr_status_t HttpdOutputFilter::outputHeaders(ap_filter_t* f, apr_bucket *e, const char *data, size_t len) {
    auto r =  f->r;
	apr_status_t rv = APR_SUCCESS;

	_headers.emplace("Server", _request.host().getRoot()->getServerNameLine());
	_buffer.clear();
	if (_responseCode < 400 || (!_hookErrors && _responseCode != 401)) {
		_skipFilter = true;
	} else {
		output::writeData(_request, _buffer, [&] (const String &ct) {
			_headers.emplace("Content-Type", ct);
		}, _request.getController()->getDefaultResult(), true);
		_headers.emplace("Content-Length", apr_psprintf(r->pool, "%lu", _buffer.size()));
		// provide additional info for 416 if we can
		if (_responseCode == 416) {
			// we need to determine requested file size
			if (const char *filename = f->r->filename) {
				apr_finfo_t info;
				memset((void *)&info, 0, sizeof(info));
				if (apr_stat(&info, filename, APR_FINFO_SIZE, r->pool) == APR_SUCCESS) {
					_headers.emplace("X-Range", apr_psprintf(r->pool, "%ld", info.size));
				} else {
					_headers.emplace("X-Range", "0");
				}
			} else {
				_headers.emplace("X-Range", "0");
			}
		} else if (_responseCode == 401) {
			if (_headers.at("WWW-Authenticate").empty()) {
				_headers.emplace("WWW-Authenticate", toString("Basic realm=\"", _request.host().getHostInfo().hostname, "\""));
			}
		}
	}

	_headersBuffer.reserve(calcHeaderSize()+1);
	writeHeader(f, _headersBuffer);

	apr_bucket *b = apr_bucket_pool_create(_headersBuffer.data(), _headersBuffer.size(), r->pool,
			_tmpBB->bucket_alloc);

	APR_BRIGADE_INSERT_TAIL(_tmpBB, b);

	if (!_buffer.empty()) {
		APR_BUCKET_REMOVE(e);
		APR_BRIGADE_INSERT_TAIL(_tmpBB, apr_bucket_pool_create(
				_buffer.data(), _buffer.size(), r->pool, _tmpBB->bucket_alloc));
		APR_BRIGADE_INSERT_TAIL(_tmpBB, apr_bucket_flush_create(_tmpBB->bucket_alloc));
		APR_BRIGADE_INSERT_TAIL(_tmpBB, apr_bucket_eos_create(
				_tmpBB->bucket_alloc));
		_seenEOS = true;

		rv = ap_pass_brigade(f->next, _tmpBB);
		apr_brigade_cleanup(_tmpBB);
		return rv;
	} else {
		rv = ap_pass_brigade(f->next, _tmpBB);
		apr_brigade_cleanup(_tmpBB);
		if (rv != APR_SUCCESS) return rv;
	}

	return rv;
}

bool HttpdOutputFilter::readRequestLine(StringView &r) {
	while (!r.empty()) {
		if (_isWhiteSpace) {
			auto tmp = r.readChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
			_buffer << tmp;
			auto tmp2 = tmp.readUntil<StringView::Chars<'\n'>>();
			if (tmp2.is('\n')) {
				return true;
			}
			if (r.empty()) {
				return false;
			}
			_isWhiteSpace = false;
		}
		if (_subState == State::Protocol) {
			_buffer << r.readUntil<StringView::CharGroup<CharGroupId::WhiteSpace>>();
			if (r.is<CharGroupId::WhiteSpace>()) {
				_nameBuffer.clear();
				_isWhiteSpace = true;
				_subState = State::Code;
			}
		} else if (_subState == State::Code) {
			auto b = r.readChars<StringView::Range<'0', '9'>>();
			_nameBuffer << b;
			_buffer << b;
			if (r.is<CharGroupId::WhiteSpace>()) {
				_nameBuffer << '\0';
				_responseCode = apr_strtoi64(_nameBuffer.data(), NULL, 0);
				_isWhiteSpace = true;
				_subState = State::Status;
			}
		} else if (_subState == State::Status) {
			auto statusText = r.readUntil<StringView::Chars<'\n'>>();
			statusText.trimChars<StringView::WhiteSpace>();
			_statusText = statusText.str<Interface>();
			_buffer << statusText;
		} else {
			r.readUntil<StringView::Chars<'\n', '\r'>>();
		}
		if (r.is('\n') || r.is('\r')) {
			++ r;
			_buffer << r.readChars<StringView::Chars<'\n', '\r'>>();
			_state = State::Headers;
			_subState = State::HeaderName;
			_isWhiteSpace = true;
			return true;
		}
	}
	return false;
}
bool HttpdOutputFilter::readHeaders(StringView &r) {
	while (!r.empty()) {
		if (_subState == State::HeaderName) {
			_nameBuffer << r.readUntil<StringView::Chars<':', '\n'>>();
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
			_buffer << r.readUntil<StringView::Chars<'\n'>>();
			if (r.is('\n')) {
				r ++;

				auto key = StringView(_nameBuffer.data(), _nameBuffer.size());
				key.trimChars<StringView::WhiteSpace>();

				auto value = StringView(_buffer.data(), _buffer.size());
				value.trimChars<StringView::WhiteSpace>();

				_headers.emplace(key.pdup(), value.pdup());

				_buffer.clear();
				_nameBuffer.clear();
				_subState = State::HeaderName;
				_isWhiteSpace = true;
			}
		}
	}
	return false;
}

}
