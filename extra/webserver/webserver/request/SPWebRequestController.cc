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

#include "SPWebRequestController.h"
#include "SPWebRequest.h"
#include "SPWebHost.h"

namespace stappler::web {

RequestController::~RequestController() { }

RequestController::RequestController(pool_t *pool, RequestInfo &&info)
: _pool(pool), _info(move(info)) {
	registerCleanupDestructor(this, pool);
}

bool RequestController::init() {
	auto str = _info.url.query;
	if (str.is('(')) {
		_info.queryData = data::serenity::read<Interface>(str);
	}
	if (!str.empty()) {
		if (str.is('?') || str.is('&')) {
			++ str;
		}
		auto d = UrlView::parseArgs<Interface>(str, 100_KiB);
		if (_info.queryData.empty()) {
			_info.queryData = std::move(d);
		} else {
			for (auto &it : d.asDict()) {
				if (!_info.queryData.hasValue(it.first)) {
					_info.queryData.setValue(std::move(it.second), it.first);
				}
			}
		}
	}

	_info.queryPath = makeSpanView(UrlView::parsePath<Interface>(_info.url.path)).pdup(_pool);

	auto h = getRequestHeader("accept");
	if (!h.empty()) {
		string::split(h, ",", [&] (StringView v) {
			float q = 1.0f;
			auto val = v.readUntil<StringView::Chars<';'>>();
			if (v.starts_with(";q=")) {
				v += 3;
				q = v.readFloat().get(1.0f);
			}
			val.trimChars<StringView::WhiteSpace>();
			_acceptList.emplace_back(pair(val, q));
		});

		std::stable_sort(_acceptList.begin(), _acceptList.end(), [] (const Pair<StringView, float> &l, const Pair<StringView, float> &r) {
			return l.second < r.second;
		});
	}

	return true;
}

float RequestController::isAcceptable(StringView name) const {
	for (auto &it : _acceptList) {
		if (it.first == name) {
			return it.second;
		}
	}
	return 0.0f;
}

void RequestController::bind(HostController *host) {
	_host = host;
	_info.documentRoot = _host->getHostInfo().documentRoot;
}

bool RequestController::isSecureAuthAllowed() const {
	return Host(_host).isSecureAuthAllowed(Request(const_cast<RequestController *>(this)));
}

db::Adapter RequestController::acquireDatabase() {
	if (!_database) {
		_database = Host(_host).acquireDbForRequest(this);
	}
	return db::Adapter(_database, _host->getRoot());
}

void RequestController::setInputFilter(InputFilter *f) {
	_filter = f;
}

Value RequestController::getDefaultResult() {
	Value ret;
	ret.setBool(false, "OK");
	ret.setInteger(_info.requestTime.toMicros(), "date");
	ret.setInteger(toInt(_info.status), "status");
	ret.setString(_info.statusLine, "message");

#if DEBUG
	if (!_debug.empty()) {
		ret.setArray(std::move(_debug), "debug");
	}
#endif
	if (!_errors.empty()) {
		ret.setArray(std::move(_errors), "errors");
	}

	return ret;
}

}
