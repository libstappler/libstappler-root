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

#ifndef TESTS_XENILITH_CLI_SRC_MAINLOOP_H_
#define TESTS_XENILITH_CLI_SRC_MAINLOOP_H_

#include "XLCommon.h"
#include "SPThreadTaskQueue.h"

namespace stappler::xenolith::test {

class MainLoop : protected thread::TaskQueue {
public:
	virtual ~MainLoop();

	bool init(StringView name, Function<void(uint64_t, uint64_t)> &&);

	void run(uint32_t threadsCount, TimeInterval);

	void end();

	void scheduleUpdate();

	bool isOnMainThread() const;

	using mem_std::AllocBase::operator new;
	using mem_std::AllocBase::operator delete;

	using Ref::release;
	using Ref::retain;

	using thread::TaskQueue::onMainThread;

protected:
	void update(uint64_t t, uint64_t dt);

	std::thread::id _threadId;
	memory::pool_t *_updatePool = nullptr;
	bool _immediateUpdate = false;
	std::atomic_flag _shouldQuit;
	Function<void(uint64_t, uint64_t)> _updateCallback;
};

}

#endif /* TESTS_XENILITH_CLI_SRC_MAINLOOP_H_ */
