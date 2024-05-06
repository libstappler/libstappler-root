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

#include "SPWebUnixWebsocket.h"

namespace STAPPLER_VERSIONIZED stappler::web {

class UnixRequestController;

UnixWebsocketSim::~UnixWebsocketSim() {
	memory::pool::destroy(_pool);
	memory::allocator::destroy(_alloc);
}

void UnixWebsocketSim::attachRequest(allocator_t *a, pool_t *p, UnixRequestController *c) {
	_alloc = a;
	_pool = p;
	_request = c;
}

void UnixWebsocketSim::attachSocket(UnixWebsocketConnectionSim *sock) {
	_socket = sock;
}

bool UnixWebsocketSim::read(WebsocketFrameType t, const uint8_t *bytes, size_t count) {
	return false;
}

bool UnixWebsocketSim::write(BytesView frame) {
	_socket->read(frame);
	return true;
}

void UnixWebsocketSim::send(const mem_std::Value &data) {
	_socket->receive(data);
}

void UnixWebsocketSim::onStarted() { }

void UnixWebsocketSim::onEnded() { }

UnixWebsocketConnectionSim::UnixWebsocketConnectionSim(allocator_t *a, pool_t *p, HostController *c, UnixWebsocketSim *sim)
: WebsocketConnection(a, p, c), _sim(sim), _reader(c->getRoot(), p), _writer(p) {
	_commonReader = &_reader;
	_commonWriter = &_writer;
}

bool UnixWebsocketConnectionSim::write(WebsocketFrameType t, const uint8_t *bytes, size_t count) {
	_sim->read(t, bytes, count);
	return true;
}

bool UnixWebsocketConnectionSim::run(WebsocketHandler *h, const Callback<void()> &beginCb, const Callback<void()> &endCb) {
	_enabled = true;
	_handler = h;
	_shouldTerminate.test_and_set();

	_sim->onStarted();
	perform([&] {
		h->handleBegin();
	}, _pool, config::TAG_WEBSOCKET, this);

	beginCb();
	while (_shouldTerminate.test_and_set()) {
		std::unique_lock lock(_waitMutex);
		if (_inputFrames.empty() && _inputValues.empty()) {
			_waitCond.wait(lock);
		}

		for (auto &it : _inputFrames) {
			if (!processFrame(it)) {
				_shouldTerminate.clear();
			}
		}

		for (auto &it : _inputValues) {
			if (!_handler->handleMessage(it)) {
				_shouldTerminate.clear();
			}
		}

		_inputFrames.clear();

		_inputValues.clear();

		lock.unlock();

		auto p = memory::pool::create(_pool);
		if (_handler) {
			perform([&, this] {
				_handler->sendPendingNotifications(p);
			}, p);
		}
		memory::pool::destroy(p);
	}

	_enabled = false;

	endCb();

	_group.waitForAll();
	perform([&] {
		h->handleEnd();
	}, _pool, config::TAG_WEBSOCKET, this);
	_sim->onEnded();

	return true;
}

void UnixWebsocketConnectionSim::wakeup() {
	_waitCond.notify_one();
}

void UnixWebsocketConnectionSim::read(BytesView frame) {
	std::unique_lock lock(_waitMutex);
	_inputFrames.emplace_back(frame.bytes<memory::StandartInterface>());
	_waitCond.notify_one();
}

void UnixWebsocketConnectionSim::receive(const mem_std::Value &val) {
	std::unique_lock lock(_waitMutex);
	_inputValues.emplace_back(val);
	_waitCond.notify_one();
}

bool UnixWebsocketConnectionSim::processFrame(BytesView frame) {
	auto p = memory::pool::create(_pool);
	WebsocketFrameReader reader(_reader.root, p);
	while (!frame.empty()) {
		size_t len = reader.getRequiredBytes();
		uint8_t *buf = reader.prepare(len);

		auto sub = frame.readBytes(len);
		memcpy(buf, sub.data(), sub.size());
		reader.save(buf, len);

		if (reader.isControlReady()) {
			if (reader.type == WebsocketFrameType::Close) {
				_clientCloseCode = WebsocketStatusCode(reader.buffer.get<BytesViewNetwork>().readUnsigned16());
				reader.popFrame();
				memory::pool::destroy(p);
				return false;
			} else if (reader.type == WebsocketFrameType::Ping) {
				write(WebsocketFrameType::Pong, nullptr, 0);
				reader.popFrame();
				memory::pool::destroy(p);
				return true;
			}
		} else if (reader.isFrameReady()) {
			auto ret = perform([&, this] {
				if (_handler && !_handler->handleFrame(reader.type, reader.frame.buffer)) {
					return false;
				}
				return true;
			}, reader.pool, config::TAG_WEBSOCKET, this);
			reader.popFrame();
			if (!ret) {
				memory::pool::destroy(p);
				return false;
			}
		}
	}
	memory::pool::destroy(p);
	return true;
}

}
