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

#include "SPWebUnixConnectionQueue.h"
#include "SPWebUnixConnectionWorker.h"

#include <sys/types.h>
#include <sys/eventfd.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>

namespace STAPPLER_VERSIONIZED stappler::web {

bool ConnectionQueue::setNonblocking(int fd) {
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1) {
		std::cout << toString("fcntl() fail to get flags for ", fd);
		return false;
	}
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
		std::cout << toString("fcntl() setNonblocking failed for ", fd);
		return false;
	}

	return true;
}

ConnectionQueue::~ConnectionQueue() {
	if (_pipe[0] > -1) { close(_pipe[0]); _pipe[0] = -1; }
	if (_pipe[1] > -1) { close(_pipe[1]); _pipe[1] = -1; }
	if (_eventFd > -1) { close(_eventFd); _eventFd = -1; }
	if (_socket > -1) { close(_socket); _socket = -1; }

	if (!_socketPath.empty()) {
		filesystem::native::unlink_fn(_socketPath);
	}
}

ConnectionQueue::ConnectionQueue(UnixRoot *r, pool_t *p, uint16_t w, UnixRoot::Config &&v)
: _root(r), _originPool(p), _nWorkers(w), _config(move(v)) {
	_eventFd = ::eventfd(0, EFD_NONBLOCK);
	_taskCounter.store(0);

	_inputQueue.setQueueLocking(_inputMutexQueue);
	_inputQueue.setFreeLocking(_inputMutexFree);
}

bool ConnectionQueue::run() {
	pool::context ctx(_originPool);

	auto addr = StringView(_config.listen);
	if (addr.starts_with("/")) {
		_socket = openUnixSocket(addr);
	} else if (addr.starts_with("unix:")) {
		_socket = openUnixSocket(addr.sub("unix:"_len));
	} else {
		auto a = addr.readUntil<StringView::Chars<':'>>();
		if (addr.is(':')) {
			++ addr;
			auto port = addr.readInteger(10).get(80);
			_socket = openNetworkSocket(a, uint16_t(port));
		} else {
			_socket = openNetworkSocket(a, uint16_t(80));
		}
	}

	if (_socket < 0) {
		return false;
	}

	_workers.reserve(_nWorkers);
	if (pipe2(_pipe, O_NONBLOCK) == 0) {
		setNonblocking(_pipe[0]);
		setNonblocking(_pipe[1]);

		retainSignals();

		for (uint32_t i = 0; i < _nWorkers; i++) {
			ConnectionWorker *worker = new (_originPool) ConnectionWorker(this, _root, _socket, _pipe[0], _eventFd);
			_workers.push_back(worker);
		}
		return true;
	}

	return false;
}

void ConnectionQueue::cancel() {
	_finalized = true;
	write(_pipe[1], "END!", 4);

	for (auto &it : _workers) {
		if (it->thread().joinable()) {
			it->thread().join();
			delete it;
		}
	}

	while (_sigCounter) {
		releaseSignals();
	}
}

void ConnectionQueue::retainSignals() {
	if (_sigCounter ++ == 0) {
		memset(&_sharedSigAction, 0, sizeof(_sharedSigAction));
		_sharedSigAction.sa_handler = SIG_IGN;
		sigemptyset(&_sharedSigAction.sa_mask);
		::sigaction(SIGUSR1, &_sharedSigAction, &_sharedSigOldUsr1Action);
		::sigaction(SIGUSR2, &_sharedSigAction, &_sharedSigOldUsr2Action);
		::sigaction(SIGPIPE, &_sharedSigAction, &_sharedSigOldPipeAction);

		::sigset_t mask;
		::sigemptyset(&mask);
		::sigaddset(&mask, SIGUSR1);
		::sigaddset(&mask, SIGUSR2);
		::sigaddset(&mask, SIGPIPE);
		::sigprocmask(SIG_BLOCK, &mask, &_oldmask);
	}
}

void ConnectionQueue::releaseSignals() {
	if (_sigCounter -- == 1) {
		::sigaction(SIGUSR1, &_sharedSigOldUsr1Action, nullptr);
		::sigaction(SIGUSR2, &_sharedSigOldUsr2Action, nullptr);
		::sigaction(SIGPIPE, &_sharedSigOldPipeAction, nullptr);
		::sigprocmask(SIG_SETMASK, &_oldmask, nullptr);
	}
}

void ConnectionQueue::retain() {
	_refCount ++;
}

void ConnectionQueue::release() {
	if (--_refCount <= 0) {
		delete this;
	}
}

void ConnectionQueue::pushTask(AsyncTask *task) {
	if (auto g = task->getGroup()) {
		g->onAdded(task);
	}
	uint64_t value = 1;

	_inputQueue.push(task->getPriority(), false, task);

	++ _taskCounter;
	write(_eventFd, &value, sizeof(uint64_t));
}

AsyncTask * ConnectionQueue::popTask() {
	AsyncTask *task = nullptr;

	_inputQueue.pop_direct([&] (int32_t, AsyncTask *t) {
		task = t;
	});

	return task;
}

void ConnectionQueue::releaseTask(AsyncTask *task) {
	if (!task->getGroup()) {
		AsyncTask::destroy(task);
	} else {
		task->getGroup()->onPerformed(task);
	}
	-- _taskCounter;
}

bool ConnectionQueue::hasTasks() {
	return _taskCounter.load();
}

int ConnectionQueue::openUnixSocket(StringView addr) {
	if (filesystem::native::access_fn(addr, filesystem::Access::Exists)) {
		// try unlink;
		if (!filesystem::native::unlink_fn(addr)) {
			log::error("Root:Socket", "Socket file exists, fail to unlink: ", addr);
			return -1;
		}
	}

	int socket = ::socket(AF_UNIX, SOCK_STREAM, 0);
	if (socket == -1) {
		log::error("Root:Socket", "Fail to open socket: ", addr);
		return -1;
	}

	int enable = 1;
	if (setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) == -1) {
		log::error("Root:Socket", "Fail to set socket option: ", addr);
		close(socket);
		return -1;
	}

	struct sockaddr_un sockaddr;
	memset(&sockaddr, 0, sizeof(struct sockaddr_un));
	sockaddr.sun_family = AF_UNIX;
	strncpy(sockaddr.sun_path, addr.data(), addr.size());

	if (::bind(socket, (struct sockaddr *) &sockaddr, sizeof(sockaddr)) < 0) {
		log::error("Root:Socket", "Fail to bind socket: ", addr);
		close(socket);
		return -1;
	}

	if (!setNonblocking(socket)) {
		log::error("Root:Socket", "Fail to set socket nonblock: ", addr);
		filesystem::native::unlink_fn(addr);
		close(socket);
		return -1;
	}

	if (::listen(socket, SOMAXCONN) < 0) {
		log::error("Root:Socket", "Fail to listen on socket: ", addr);
		filesystem::native::unlink_fn(addr);
		close(socket);
		return -1;
	}

	_socketPath = addr;

	return socket;
}

int ConnectionQueue::openNetworkSocket(StringView addr, uint16_t port) {
	int socket = ::socket(AF_INET, SOCK_STREAM, 0);
	if (socket == -1) {
		log::error("Root:Socket", "Fail to open socket");
		return -1;
	}

	int enable = 1;
	if (setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) == -1) {
		log::error("Root:Socket", "Fail to set socket option");
		close(socket);
		return -1;
	}

	struct sockaddr_in sockaddr;
	memset(&addr, 0, sizeof(addr));
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_addr.s_addr = !addr.empty() ? inet_addr(addr.data()) : htonl(INADDR_LOOPBACK);
	sockaddr.sin_port = htons(port);
	if (::bind(socket, (struct sockaddr *) &sockaddr, sizeof(sockaddr)) < 0) {
		log::error("Root:Socket", "Fail to bind socket");
		close(socket);
		return -1;
	}

	if (!setNonblocking(socket)) {
		log::error("Root:Socket", "Fail to set socket nonblock");
		close(socket);
		return -1;
	}

	if (::listen(socket, SOMAXCONN) < 0) {
		log::error("Root:Socket", "Fail to listen on socket");
		close(socket);
		return -1;
	}
	return socket;
}

}
