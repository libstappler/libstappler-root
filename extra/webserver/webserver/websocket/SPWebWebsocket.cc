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

#include "SPWebWebsocket.h"
#include "SPWebRoot.h"

namespace stappler::web {

constexpr auto WEBSOCKET_GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
constexpr auto WEBSOCKET_GUID_LEN = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"_len;

static String makeAcceptKey(const String &key) {
	apr_byte_t digest[APR_SHA1_DIGESTSIZE];
	apr_sha1_ctx_t context;

	apr_sha1_init(&context);
	apr_sha1_update(&context, key.c_str(), key.size());
	apr_sha1_update(&context, (const char *)WEBSOCKET_GUID, (unsigned int)WEBSOCKET_GUID_LEN);
	apr_sha1_final(digest, &context);

	return base64::encode(CoderSource(digest, APR_SHA1_DIGESTSIZE));
}

WebsocketManager::WebsocketManager(const Host &host) : _pool(getCurrentPool()), _host(host) { }
WebsocketManager::~WebsocketManager() { }

WebsocketHandler * WebsocketManager::onAccept(const Request &req, pool_t *) {
	return nullptr;
}
bool WebsocketManager::onBroadcast(const Value & val) {
	return false;
}

size_t WebsocketManager::size() const {
	return _count.load();
}

void WebsocketManager::receiveBroadcast(const Value &val) {
	if (onBroadcast(val)) {
		_mutex.lock();
		for (auto &it : _handlers) {
			it->receiveBroadcast(val);
		}
		_mutex.unlock();
	}
}

static void *WebsocketManager_spawnThread(apr_thread_t *self, void *data) {
#if LINUX
	pthread_setname_np(pthread_self(), "WebSocketThread");
#endif

	Handler *h = (Handler *)data;
	Manager *m = h->manager();
	mem::perform([&] {
		mem::perform([&] {
			m->run(h);
		}, h->connection()->getConnection());
	}, m->server());

	Connection::destroy(h->connection());

	return NULL;
}

static int WebsocketManager_abortfn(int retcode) {
	std::cout << "WebSocket Handle allocation failed with code: " << retcode << "\n";
	return retcode;
}

int WebsocketManager::accept(Request &req) {
	auto version = req.getRequestHeader("sec-websocket-version");
	auto key = req.getRequestHeader("sec-websocket-key");
	auto decKey = base64::decode<Interface>(key);
	if (decKey.size() != 16 || version != "13") {
		req.setErrorHeader("Sec-WebSocket-Version", "13");
		return HTTP_BAD_REQUEST;
	}

	allocator_t *alloc = nullptr;
	pool_t *pool = nullptr;

	auto FailCleanup = [&] (int code) -> int {
		if (pool) {
			pool::destroy(pool);
		}

		if (alloc) {
			allocator::destroy(alloc);
		}

		return code;
	};

	alloc = allocator::create();
	pool = pool::create(alloc);

	allocator::max_free_set(alloc, 20_MiB);

	auto orig = pool::acquire();
	auto handler = onAccept(req, pool);

	if (handler) {
		auto hout = req.getResponseHeaders();

		hout.clear();
		req.setResponseHeader("Upgrade", "websocket");
		req.setResponseHeader("Connection", "Upgrade");
		req.setResponseHeader("Sec-WebSocket-Accept", makeAcceptKey(key));

		auto r = req.config()->convertToWebsocket();
		auto sock = ap_get_conn_socket(r->connection);

		// send HTTP_SWITCHING_PROTOCOLS right f*cking NOW!
		apr_socket_timeout_set(sock, -1);
		req.setStatus(HTTP_SWITCHING_PROTOCOLS);
		ap_send_interim_response(req.request(), 1);

		// block any other output
		ap_add_output_filter(WEBSOCKET_FILTER, (void *)handler, r, r->connection);

		// duplicate connection
		if (auto conn = Connection::create(alloc, pool, req)) {
			handler->setConnection(conn);
			apr_thread_t *thread = nullptr;
			apr_threadattr_t *attr = nullptr;
			apr_status_t error = apr_threadattr_create(&attr, orig);
			if (error == APR_SUCCESS) {
				apr_threadattr_detach_set(attr, 1);
				conn->prepare(sock);
				if (apr_thread_create(&thread, attr,
						Manager_spawnThread, handler, orig) == APR_SUCCESS) {
					return HTTP_OK;
				}
			} else {
				conn->drop();
			}
		}
	}
	if (req.getStatus() == HTTP_OK) {
		return FailCleanup(HTTP_BAD_REQUEST);
	}
	return FailCleanup(req.getStatus());
}

void WebsocketManager::run(WebsocketHandler *h) {
	auto c = h->connection();
	c->run(h, [&] {
		addHandler(h);
	}, [&] {
		removeHandler(h);
	});
}

void WebsocketManager::addHandler(WebsocketHandler * h) {
	_mutex.lock();
	_handlers.emplace_back(h);
	++ _count;
	_mutex.unlock();
}

void WebsocketManager::removeHandler(WebsocketHandler * h) {
	_mutex.lock();
	auto it = _handlers.begin();
	while (it != _handlers.end() && *it != h) {
		++ it;
	}
	if (it != _handlers.end()) {
		_handlers.erase(it);
	}
	-- _count;
	_mutex.unlock();
}

WebsocketHandler::WebsocketHandler(WebsocketManager *m, pool_t *p, StringView url, TimeInterval ttl, size_t max)
: _pool(p), _manager(m), _url(url.pdup(_pool)), _ttl(ttl), _maxInputFrameSize(max), _broadcastMutex(_pool) {
}

WebsocketHandler::~WebsocketHandler() { }

// Data frame was received from network
bool WebsocketHandler::onFrame(WebsocketFrameType, const Bytes &) { return true; }

// Message was received from broadcast
bool WebsocketHandler::onMessage(const Value &) { return true; }

void WebsocketHandler::sendBroadcast(Value &&val) const {
	Value bcast {
		std::make_pair("server", Value(_manager->host().getHostInfo().hostname)),
		std::make_pair("url", Value(_url)),
		std::make_pair("data", Value(std::move(val))),
	};

	performWithStorage([&] (const db::Transaction &t) {
		t.getAdapter().broadcast(bcast);
	});
}

void WebsocketHandler::setEncodeFormat(const data::EncodeFormat &fmt) {
	_format = fmt;
}

bool WebsocketHandler::send(StringView str) {
	return _conn->write(WebsocketFrameType::Text, (const uint8_t *)str.data(), str.size());
}
bool WebsocketHandler::send(BytesView bytes) {
	return _conn->write(WebsocketFrameType::Binary, bytes.data(), bytes.size());
}
bool WebsocketHandler::send(const Value &data) {
	if (_format.isTextual()) {
		StringStream stream;
		stream << _format << data;
		return send(StringView(stream.weak()));
	} else {
		return send(data::write(data, _format));
	}
}

void WebsocketHandler::performWithStorage(const Callback<void(const db::Transaction &)> &cb) const {
	_manager->host().performWithStorage(cb);
}

bool WebsocketHandler::performAsync(const Callback<void(AsyncTask &)> &cb) const {
	return _conn->performAsync(cb);
}

pool_t *WebsocketHandler::pool() const {
	return _conn->getHandlePool();
}

void WebsocketHandler::setConnection(WebsocketConnection *c) {
	_conn = c;
}

void WebsocketHandler::receiveBroadcast(const Value &data) {
	if (_conn->isEnabled()) {
		_broadcastMutex.lock();
		if (!_broadcastsPool) {
			_broadcastsPool = memory::pool::create(_pool);
		}
		if (_broadcastsPool) {
			perform([&] {
				if (!_broadcastsMessages) {
					_broadcastsMessages = new (_broadcastsPool) Vector<Value>(_broadcastsPool);
				}

				_broadcastsMessages->emplace_back(data);
			}, _broadcastsPool, config::TAG_WEBSOCKET, this);
		}
		_broadcastMutex.unlock();
		_conn->wakeup();
	}
}

bool WebsocketHandler::processBroadcasts() {
	pool_t *pool;
	Vector<Value> * vec;

	_broadcastMutex.lock();

	pool = _broadcastsPool;
	vec = _broadcastsMessages;

	_broadcastsPool = nullptr;
	_broadcastsMessages = nullptr;

	_broadcastMutex.unlock();

	bool ret = true;
	if (pool) {
		perform([&] {
			sendPendingNotifications(pool);
			if (vec) {
				for (auto & it : (*vec)) {
					if (!onMessage(it)) {
						ret = false;
						break;
					}
				}
			}
		}, pool, config::TAG_WEBSOCKET, this);
		pool::destroy(pool);
	}

	return ret;
}

void WebsocketHandler::sendPendingNotifications(pool_t *pool) {
	perform([&] {
		_manager->host().getRoot()->setErrorNotification(pool, [this] (Value && data) {
			send(Value({
				std::make_pair("error", Value(std::move(data)))
			}));
		}, [this] (Value && data) {
			send(Value({
				std::make_pair("debug", Value(std::move(data)))
			}));
		});
	}, pool, config::TAG_WEBSOCKET, this);
}

}
