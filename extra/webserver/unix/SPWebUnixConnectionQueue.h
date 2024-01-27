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

#ifndef EXTRA_WEBSERVER_UNIX_SPWEBUNIXCONNECTIONQUEUE_H_
#define EXTRA_WEBSERVER_UNIX_SPWEBUNIXCONNECTIONQUEUE_H_

#include "SPWebUnixRoot.h"
#include "SPMemPriorityQueue.h"

#include <signal.h>

namespace stappler::web {

class ConnectionWorker;

class ConnectionQueue : public AllocBase {
public:
	static bool setNonblocking(int fd);

	virtual ~ConnectionQueue();

	ConnectionQueue(UnixRoot *h, pool_t *p, uint16_t w, UnixRoot::Config &&);

	bool run();
	void cancel();

	void retainSignals();
	void releaseSignals();

	void retain();
	void release();
	void finalize();

	void pushTask(AsyncTask *);
	AsyncTask * popTask();
	void releaseTask(AsyncTask *);

	bool hasTasks();

	size_t getWorkersCount() const { return _workers.size(); }

protected:
	int openUnixSocket(StringView);
	int openNetworkSocket(StringView, uint16_t port);

	UnixRoot *_root = nullptr;

	pool_t *_originPool = nullptr;

	uint16_t _nWorkers = uint16_t(std::thread::hardware_concurrency());
	UnixRoot::Config _config;

	std::atomic<bool> _finalized;
	std::atomic<int32_t> _refCount = 1;

	Vector<ConnectionWorker *> _workers;

	std::atomic<size_t> _taskCounter;
	std::mutex _inputMutexQueue;
	std::mutex _inputMutexFree;
	memory::PriorityQueue<AsyncTask *> _inputQueue;
	std::thread _thread;

	int _pipe[2] = { -1, -1 };
	int _eventFd = -1;
	int _socket = -1;
	Time _start = Time::now();

	uint32_t _sigCounter = 0;
	struct sigaction _sharedSigAction;
	struct sigaction _sharedSigOldUsr1Action;
	struct sigaction _sharedSigOldUsr2Action;
	struct sigaction _sharedSigOldPipeAction;
	sigset_t _oldmask;

	StringView _socketPath;
};

}

#endif /* EXTRA_WEBSERVER_UNIX_SPWEBUNIXCONNECTIONQUEUE_H_ */
