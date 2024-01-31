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

#include "SPWebWebsocketConnection.h"

namespace STAPPLER_VERSIONIZED stappler::web {

void WebsocketConnection::destroy(WebsocketConnection *conn) {
	auto p = conn->_pool;
	auto a = conn->_allocator;

	pool::destroy(p);
	allocator::destroy(a);
}

WebsocketConnection::~WebsocketConnection() { }

void WebsocketConnection::terminate() {
	_shouldTerminate.clear();
	wakeup();
}

pool_t *WebsocketConnection::getHandlePool() const {
	return _commonReader->pool;
}

Host WebsocketConnection::getHost() const {
	return _group.getHost();
}

bool WebsocketConnection::performAsync(const Callback<void(AsyncTask &)> &cb) const {
	if (!_enabled) {
		return false;
	}
	return _group.perform(cb);
}

void WebsocketConnection::setStatusCode(WebsocketStatusCode s, StringView r) {
	_serverCloseCode = s;
	if (!r.empty()) {
		_serverReason = r.str<Interface>();
	}
}

void WebsocketConnection::setAccessRole(db::AccessRoleId id) {
	_accessRole = id;
}

WebsocketStatusCode WebsocketConnection::resolveStatus(WebsocketStatusCode code) {
	if (code == WebsocketStatusCode::Auto) {
		switch (_commonReader->error) {
		case WebsocketFrameReader::Error::NotInitialized: return WebsocketStatusCode::UnexceptedCondition; break;
		case WebsocketFrameReader::Error::ExtraIsNotEmpty: return WebsocketStatusCode::ProtocolError; break;
		case WebsocketFrameReader::Error::NotMasked: return WebsocketStatusCode::ProtocolError; break;
		case WebsocketFrameReader::Error::UnknownOpcode: return WebsocketStatusCode::ProtocolError; break;
		case WebsocketFrameReader::Error::InvalidSegment: return WebsocketStatusCode::ProtocolError; break;
		case WebsocketFrameReader::Error::InvalidSize: return WebsocketStatusCode::TooLarge; break;
		case WebsocketFrameReader::Error::InvalidAction: return WebsocketStatusCode::UnexceptedCondition; break;
		default: return WebsocketStatusCode::Ok; break;
		}
	} else if (code == WebsocketStatusCode::None) {
		return WebsocketStatusCode::Ok;
	}
	return code;
}

WebsocketConnection::WebsocketConnection(allocator_t *a, pool_t *p, HostController *c)
: _allocator(a), _pool(p), _group(Host(c), [this] {
	wakeup();
}) { }

}
