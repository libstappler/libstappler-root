/**
 Copyright (c) 2023 Stappler LLC <admin@stappler.dev>

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

#include "XLCommon.h"
#include "MainLoop.h"

namespace stappler::xenolith::test {

MainLoop::~MainLoop() {
	if (_updatePool) {
		memory::pool::destroy(_updatePool);
	}
}

bool MainLoop::init(StringView name, Function<void(uint64_t, uint64_t)> &&cb) {
	_updatePool = memory::pool::create((memory::pool_t *)nullptr);
	_shouldQuit.test_and_set();
	_name = name;
	_updateCallback = move(cb);

	return true;
}

void MainLoop::run(uint32_t threadsCount, TimeInterval iv) {
	_threadId = std::this_thread::get_id();

	if (!spawnWorkers(thread::TaskQueue::Flags::Waitable, 0, threadsCount, _name)) {
		log::text("Application", "Fail to spawn worker threads");
	}

	uint32_t count = 0;
	uint64_t clock = platform::clock(core::ClockType::Monotonic);
	uint64_t lastUpdate = clock;
	do {
		count = 0;
		if (!_immediateUpdate) {
			wait(iv - TimeInterval::microseconds(clock - lastUpdate), &count);
		}
		if (count > 0) {
			memory::pool::push(_updatePool);
			TaskQueue::update();
			memory::pool::pop();
			memory::pool::clear(_updatePool);
		}
		clock = platform::clock(core::ClockType::Monotonic);

		auto dt = TimeInterval::microseconds(clock - lastUpdate);
		if (dt >= iv || _immediateUpdate) {
			update(clock, dt.toMicros());
			lastUpdate = clock;
			_immediateUpdate = false;
		}
	} while (_shouldQuit.test_and_set());

	waitForAll();
}

void MainLoop::end() {
	_shouldQuit.clear();
}

void MainLoop::scheduleUpdate() {
	if (isOnMainThread()) {
		_immediateUpdate = true;
	} else {
		onMainThread([this] {
			_immediateUpdate = true;
		}, this);
	}
}

bool MainLoop::isOnMainThread() const {
	return _threadId == std::this_thread::get_id();
}

void MainLoop::update(uint64_t t, uint64_t dt) {
	memory::pool::push(_updatePool);
	if (_updateCallback) {
		_updateCallback(t, dt);
	}
	memory::pool::pop();
	memory::pool::clear(_updatePool);
}

}
