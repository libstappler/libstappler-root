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

#include "SPWebUnixConfig.h"
#include "SPWebUnixConnectionWorker.h"
#include "SPWebUnixConnectionQueue.h"
#include "SPWebUnixRequest.h"
#include "SPWebRequestFilter.h"

#include <sys/signalfd.h>
#include <sys/ioctl.h>
#include <sys/sendfile.h>

namespace stappler::web {

static StringView s_getSignalName(int sig) {
	switch (sig) {
	case SIGINT: return "SIGINT";
	case SIGILL: return "SIGILL";
	case SIGABRT: return "SIGABRT";
	case SIGFPE: return "SIGFPE";
	case SIGSEGV: return "SIGSEGV";
	case SIGTERM: return "SIGTERM";
	case SIGHUP: return "SIGHUP";
	case SIGQUIT: return "SIGQUIT";
	case SIGTRAP: return "SIGTRAP";
	case SIGKILL: return "SIGKILL";
	case SIGBUS: return "SIGBUS";
	case SIGSYS: return "SIGSYS";
	case SIGPIPE: return "SIGPIPE";
	case SIGALRM: return "SIGALRM";
	case SIGURG: return "SIGURG";
	case SIGSTOP: return "SIGSTOP";
	case SIGTSTP: return "SIGTSTP";
	case SIGCONT: return "SIGCONT";
	case SIGCHLD: return "SIGCHLD";
	case SIGTTIN: return "SIGTTIN";
	case SIGTTOU: return "SIGTTOU";
	case SIGPOLL: return "SIGPOLL";
	case SIGXCPU: return "SIGXCPU";
	case SIGXFSZ: return "SIGXFSZ";
	case SIGVTALRM: return "SIGVTALRM";
	case SIGPROF: return "SIGPROF";
	case SIGUSR1: return "SIGUSR1";
	case SIGUSR2: return "SIGUSR2";
	default: return "(unknown)";
	}
	return StringView();
}

ConnectionWorker::ConnectionWorker(ConnectionQueue *queue, UnixRoot *h, int socket, int pipe, int event)
: _queue(queue), _root(h)
, _inputClient(socket, EPOLLIN | EPOLLEXCLUSIVE)
, _cancelClient(pipe, EPOLLIN | EPOLLET)
, _eventClient(event, EPOLLIN | EPOLLET | EPOLLEXCLUSIVE)
,_thread(ConnectionWorker::workerThread, this, queue) {
	_queue->retain();
}

ConnectionWorker::~ConnectionWorker() {
	_queue->release();
}

void ConnectionWorker::threadInit() {
	_threadAlloc = allocator::create();
	_threadPool = pool::create(_threadAlloc);

	_threadId = std::this_thread::get_id();

	sigset_t sigset;
	sigfillset(&sigset);

	_signalFd = ::signalfd(-1, &sigset, 0);
	ConnectionQueue::setNonblocking(_signalFd);

	_epollFd = epoll_create1(0);

	addClient(_inputClient);
	addClient(_cancelClient);
	addClient(_eventClient);
}

void ConnectionWorker::threadDispose() {
	if (_signalFd >= 0) {
		close(_signalFd);
		_signalFd = -1;
	}
	if (_epollFd >= 0) {
		close(_epollFd);
		_epollFd = -1;
	}

	pool::destroy(_threadPool);
	allocator::destroy(_threadAlloc);
}

bool ConnectionWorker::worker() {
	if (_shouldClose) {
		return false;
	}

	while (poll(_epollFd)) {
		struct signalfd_siginfo si;
		int nr = ::read(_signalFd, &si, sizeof si);
		while (nr == sizeof si) {
			if (si.ssi_signo != SIGINT) {
				log::error("ConnectionWorker", "epoll_wait() exit with signal: ", si.ssi_signo, " ", s_getSignalName(si.ssi_signo));
			}
			nr = ::read(_signalFd, &si, sizeof si);
		}
	}

	return true;
}

bool ConnectionWorker::poll(int epollFd) {
	std::array<struct epoll_event, ConnectionWorker::MaxEvents> _events;

	while (!_shouldClose) {
		int nevents = epoll_wait(epollFd, _events.data(), ConnectionWorker::MaxEvents, -1);
		if (nevents == -1 && errno != EINTR) {
			char buf[256] = { 0 };
			log::error("ConnectionWorker", "epoll_wait() failed with errno ", errno, " (", strerror_r(errno, buf, 255), ")");
			return false;
		} else if (errno == EINTR) {
			return true;
		}

		/// process high-priority events
		for (int i = 0; i < nevents; i++) {
			Client *client = static_cast<Client *>(_events[i].data.ptr);

			if ((_events[i].events & EPOLLIN)) {
				if (client == &_eventClient) {
					uint64_t value = 0;
					auto sz = read(_eventClient.fd, &value, sizeof(uint64_t));
					if (sz == 8 && value) {
						auto ev = _queue->popTask();
						if (value > 1) {
							value -= 1;
							// forward event to another worker
							write(_eventClient.fd, &value, sizeof(uint64_t));
						}
						if (ev) {
							runTask(ev);
						}
					}
				}
			}
		}

		for (int i = 0; i < nevents; i++) {
			Client *client = static_cast<Client *>(_events[i].data.ptr);
			if ((_events[i].events & EPOLLERR)) {
				if (client == &_inputClient) {
					log::error("ConnectionWorker", "epoll error on server socket ", client->fd);
					_shouldClose = true;
				} else {
					log::error("ConnectionWorker", "epoll error on socket ", client->fd);
					client->gen->releaseClient(client);
				}
				continue;
			}

			if ((_events[i].events & EPOLLIN)) {
				if (client == &_inputClient) {
					char str[INET_ADDRSTRLEN + 1];

					struct sockaddr client_addr;
					socklen_t client_addr_len = sizeof(client_addr);
					int newClient = ::accept(_inputClient.fd, &client_addr, &client_addr_len);
					while (newClient != -1) {
						struct sockaddr_in* pV4Addr = (struct sockaddr_in*)&client_addr;
						struct in_addr ipAddr = pV4Addr->sin_addr;

						memset(str, 0, INET_ADDRSTRLEN + 1);
						inet_ntop( AF_INET, &ipAddr, str, INET_ADDRSTRLEN );

						pushFd(epollFd, newClient, StringView(str), pV4Addr->sin_port);
						newClient = ::accept(_inputClient.fd, &client_addr, &client_addr_len);
					}
					if (errno == EAGAIN || errno == EWOULDBLOCK) {
						// we processed all of the connections
						break;
					} else {
						log::error("ConnectionWorker", "accept() failed");
						break;
					}
				} else if (client == &_cancelClient) {
					//onError("Received end signal");
					_shouldClose = true;
				} else if (client == &_eventClient) {
					// do nothing
				} else {
					client->performRead();
				}
			}

			if ((_events[i].events & EPOLLOUT) && !client->system) {
				client->performWrite();
			}

			if ((_events[i].events & EPOLLHUP) || (_events[i].events & EPOLLRDHUP)) {
				if (client != &_eventClient && client != &_cancelClient) {
					removeClient(*client);
				}
			}

			if (!client->system && !client->valid) {
				client->shutdownAll();
			}
		}
	}

	if (_shouldClose) {
		auto gen = _generation;
		while (gen) {
			gen->releaseAll();
			gen = _generation->prev;
		}
	}

	return !_shouldClose;
}

void ConnectionWorker::runTask(AsyncTask *task) {
	auto host = task->getHost();
	perform([&] {
		AsyncTask::run(task);
	}, task->pool(), config::TAG_HOST, host.getConfig());
	_queue->releaseTask(task);
}

UnixRequestController *ConnectionWorker::readRequest(Client *client, BufferChain &chain) {
	RequestInfo info;

	StringView str;
	pool_t *tmpPool = nullptr;

	if (chain.isSingle()) {
		str = StringView((const char *)chain.front->buf, chain.getBytesRead());
	} else {
		tmpPool = pool::create(client->pool);

		auto data = chain.extract(tmpPool, 0, chain.getBytesRead());
		str = StringView(data.readString(chain.getBytesRead()));
	}

	if (!RequestFilter::readRequestLine(str, info)) {
		if (tmpPool) {
			pool::destroy(tmpPool);
		}
		client->cancelWithResult(HTTP_INTERNAL_SERVER_ERROR);
		return nullptr;
	}

	UnixRequestController *cfg = nullptr;
	auto p = pool::create(_threadPool);
	perform([&] {
		cfg = new (p) UnixRequestController(p, info.clone(p), client);
	}, p);

	if (tmpPool) {
		pool::destroy(tmpPool);
	}

	client->bytesRead = chain.getBytesRead();
	chain.releaseEmpty();

	return cfg;
}

Status ConnectionWorker::parseRequestHeader(UnixRequestController *cfg, Client *client, BufferChain &chain) {
	StringView name;
	StringView value;

	StringView str;
	pool_t *tmpPool = nullptr;
	size_t headerLen = chain.getBytesRead() - client->bytesRead;

	if (chain.isSingle()) {
		str = StringView((const char *)chain.front->buf + (client->bytesRead - chain.front->absolute), headerLen);
	} else {
		tmpPool = pool::create(client->pool);

		auto data = chain.extract(tmpPool, 0, headerLen);
		str = StringView(data.readString(headerLen));
	}

	if (str.size() == 2 && str == "\r\n") {
		client->bytesRead += headerLen;
		chain.releaseEmpty();
		return DONE;
	}

	if (!RequestFilter::readRequestHeader(str, name, value)) {
		if (tmpPool) {
			pool::destroy(tmpPool);
		}
		return HTTP_INTERNAL_SERVER_ERROR;
	}

	perform([&] {
		cfg->setRequestHeader(name, value);
	}, cfg->getPool());

	if (tmpPool) {
		pool::destroy(tmpPool);
	}

	client->bytesRead = chain.getBytesRead();
	chain.releaseEmpty();
	return OK;
}

Status ConnectionWorker::processRequest(UnixRequestController *req) {
	return perform([&] {
		auto ret = _root->processRequest(req);
		switch (ret) {
		case DONE:
			req->submitResponse(ret);
			break;
		case DECLINED:
			break;
		case SUSPENDED:
		case OK:
		default:
			break;
		}
		return ret;
	}, req->getPool(), config::TAG_REQUEST, req);
}

void ConnectionWorker::pushFd(int epollFd, int fd, StringView addr, uint16_t port) {
	if (!_generation) {
		_generation = makeGeneration();
	}

	auto c = _generation->pushFd(fd, addr, port);
	if (addClient(*c)) {
		++ _fdCount;
	} else {
		c->gen->releaseClient(c);
	}
}

bool ConnectionWorker::addClient(Client &client) {
	if (client.fd < 0) {
		return false;
	}

	int err = ::epoll_ctl(_epollFd, EPOLL_CTL_ADD, client.fd, &client.event);
	if (err == -1) {
		char buf[256] = { 0 };
		log::error("ConnectionWorker", "Failed to add client epoll_ctl("
				, client.fd,  ", EPOLL_CTL_ADD): ",  strerror_r(errno, buf, 255));
		return false;
	}
	return true;
}

void ConnectionWorker::removeClient(Client &client) {
	if (client.fd < 0) {
		return;
	}

	int err = ::epoll_ctl(_epollFd, EPOLL_CTL_DEL, client.fd, &client.event);
	if (err == -1) {
		char buf[256] = { 0 };
		log::error("ConnectionWorker", "Failed to remove client epoll_ctl("
				, client.fd,  ", EPOLL_CTL_DEL): ",  strerror_r(errno, buf, 255));
	}

	client.release();
}

ConnectionWorker::Buffer *ConnectionWorker::Buffer::create(pool_t *p, size_t abs) {
	auto requestSize = config::UNIX_CLIENT_BUFFER_SIZE;
	auto block = pool::palloc(p, requestSize);

	auto b = new (block) Buffer();

	b->next = nullptr;
	b->pool = p;
	b->buf = (uint8_t *)block + sizeof(Buffer);
	b->capacity = requestSize - sizeof(Buffer);
	b->size = 0;
	b->offset = 0;
	b->absolute = abs;

	return b;
}

ConnectionWorker::Buffer *ConnectionWorker::Buffer::create(pool_t *p, StringView path, off_t rangeStart, size_t rangeLen, size_t abs) {
	if (!filesystem::exists(path)) {
		return nullptr;
	}

	auto requestSize = config::UNIX_CLIENT_BUFFER_SIZE;
	auto block = pool::palloc(p, requestSize);

	auto b = new (block) Buffer();

	b->next = nullptr;
	b->pool = p;
	b->buf = (uint8_t *)block + sizeof(Buffer);
	b->capacity = requestSize - sizeof(Buffer);
	b->offset = rangeStart;
	b->absolute = abs;
	b->flags |= IsOutFile;

	BufferFile *file = new (b->buf) BufferFile;
	filesystem::stat(path, file->stat);
	file->fd = ::open(path.data(), O_RDONLY);
	file->extraBuffer = b->buf + sizeof(BufferFile);
	b->capacity -= sizeof(BufferFile);

	b->size = std::min(rangeLen, file->stat.size - rangeStart);

	return b;
}

void ConnectionWorker::Buffer::release() {
	if (auto f = getFile()) {
		if (f->fd >= 0) {
			::close(f->fd);
			f->fd = -1;
		}
	}
	pool::free(pool, this, capacity + sizeof(this));
}

StringView ConnectionWorker::Buffer::str() const {
	if (isOutFile()) {
		return StringView();
	}

	return StringView(reinterpret_cast<const char *>(buf + offset), size - offset);
}

size_t ConnectionWorker::Buffer::availableForWrite() const {
	if (isOutFile()) {
		return 0;
	}

	return capacity - size;
}

size_t ConnectionWorker::Buffer::availableForRead() const {
	return size - offset;
}

uint8_t *ConnectionWorker::Buffer::writeTarget() const {
	if (isOutFile()) {
		return nullptr;
	}

	return buf + size;
}

uint8_t *ConnectionWorker::Buffer::readSource() const {
	if (isOutFile()) {
		return nullptr;
	}

	return buf + offset;
}

size_t ConnectionWorker::Buffer::write(const uint8_t *b, size_t s) {
	if (isOutFile()) {
		return 0;
	}

	auto writeSize = std::min(availableForWrite(), s);
	memcpy(writeTarget(), b, writeSize);
	size += writeSize;
	return writeSize;
}

ConnectionWorker::BufferFile *ConnectionWorker::Buffer::getFile() const {
	if (isOutFile()) {
		return (ConnectionWorker::BufferFile *)buf;
	}
	return nullptr;
}

bool ConnectionWorker::BufferChain::isEos() const {
	if (eos || (back && (back->flags & Buffer::Eos) != Buffer::None)) {
		return true;
	}
	return false;
}

bool ConnectionWorker::BufferChain::empty() const {
	if (!back || back->availableForRead() == 0) {
		return true;
	}
	return false;
}

size_t ConnectionWorker::BufferChain::size() const {
	size_t ret = 0;
	auto b = front;
	while (b) {
		ret += b->size;
		b = b->next;
	}
	return ret;
}

ConnectionWorker::Buffer *ConnectionWorker::BufferChain::getWriteTarget(pool_t *pool) {
	if (back && back->availableForWrite() > 0) {
		return back;
	} else {
		auto b = Buffer::create(pool, back ? back->absolute + back->capacity : 0);

		if (back) {
			back->next = b;
			back = b;
			tail = &(back->next);
		} else {
			front = back = b;
			tail = &(back->next);
		}

		return b;
	}
}

bool ConnectionWorker::BufferChain::write(pool_t *p, const uint8_t *buf, size_t size, Buffer::Flags flags) {
	Buffer *targetBuffer = nullptr;
	size_t written = 0;

	if (isEos()) {
		return false;
	}

	if (size == 0 && flags != Buffer::Flags::None) {
		if (back) {
			back->flags |= flags;
		} else {
			if ((flags & Buffer::Eos) != Buffer::None) {
				eos = true;
			}
		}
		return true;
	}

	while (size > 0) {
		targetBuffer = getWriteTarget(p);

		written = targetBuffer->write(buf, size);
		buf += written; size -= written;
	}

	if (targetBuffer && flags != Buffer::Flags::None) {
		targetBuffer->flags |= flags;
	}

	return true;
}

bool ConnectionWorker::BufferChain::write(Buffer *newBuf) {
	if (back) {
		back->next = newBuf;
		back = newBuf;
	} else {
		front = back = newBuf;
	}
	tail = &(back->next);
	return true;
}

bool ConnectionWorker::BufferChain::write(BufferChain &chain) {
	if (back) {
		back->next = chain.front;
		back = chain.back;
	} else {
		front = chain.front;
		back = chain.back;
	}
	tail = &(back->next);

	chain.front = chain.back = nullptr;
	chain.tail = nullptr;

	return true;
}

bool ConnectionWorker::BufferChain::readFromFd(pool_t *p, int fd) {
	int sz = 0;
	::ioctl (fd, FIONREAD, &sz);

	while (sz > 0) {
		Buffer *targetBuffer = getWriteTarget(p);

		sz = ::read(fd, targetBuffer->writeTarget(), targetBuffer->availableForWrite());
		if (sz > 0) {
			targetBuffer->size += sz;

			sz = ::read(fd, nullptr, 0);
			if (sz == 0) {
				::ioctl (fd, FIONREAD, &sz);
			}
		}

		if (sz == -1) {
			if (errno != EAGAIN && errno != EWOULDBLOCK) {
				char tmp[256] = { 0 };
				log::error("ConnectionWorker", "Fail to read from client: ", strerror_r(errno, tmp, 255));
				return false;
			}
		}
	}

	return true;
}

Status ConnectionWorker::BufferChain::read(const Callback<int(const Buffer *, const uint8_t *, size_t)> &cb, bool release) {
	auto buf = front;
	while (buf) {
		auto size = buf->availableForRead();
		auto ret = cb(buf, buf->readSource(), size);
		while (size > 0 && ret > 0) {
			buf->offset += ret;
			size = buf->availableForRead();
			if (size > 0) {
				ret = cb(buf, buf->readSource(), size);
			}
		}

		if (size == 0) {
			if ((buf->flags & Buffer::Eos) != Buffer::None) {
				eos = true;
			}
			if (release) {
				auto f = front;
				front = front->next;
				if (&f->next ==tail) {
					tail = nullptr;
				}
				if (f == back) {
					back = nullptr;
				}
				f->release();
				buf = front;
			} else {
				buf = buf->next;
			}
		}

		if (ret == DONE || eos) {
			return DONE;
		} else if (ret == SUSPENDED) {
			return SUSPENDED;
		} else if (ret == DECLINED) {
			return DECLINED;
		}
	}
	return OK;
}

Status ConnectionWorker::BufferChain::writeToFd(int fd, size_t &bytesWritten) {
	return read([&] (const Buffer *source, const uint8_t *buf, size_t size) {
		ssize_t ret = 0;
		if (auto f = source->getFile()) {
			off_t offset = source->offset;
			ret = ::sendfile(fd, f->fd, &offset, source->size);
		} else {
			ret = ::write(fd, buf, size);
		}
		if (ret >= 0) {
			bytesWritten += ret;
			return int(ret);
		} else {
			if (errno != EAGAIN && errno != EWOULDBLOCK) {
				return int(SUSPENDED);
			}
			return int(DECLINED);
		}
	}, true);
}

size_t ConnectionWorker::BufferChain::getBytesRead() const {
	if (back) {
		return back->absolute + back->offset;
	}
	return 0;
}

BytesView ConnectionWorker::BufferChain::extract(pool_t *pool, size_t initOffset, size_t blockSize) const {
	if (!front) {
		return BytesView();
	}

	if (initOffset + blockSize > getBytesRead()) {
		return BytesView();
	}

	if (front && front->absolute > initOffset) {
		return BytesView();
	}

	size_t targetSize = blockSize;
	size_t writeSize = blockSize;
	auto block = (uint8_t *)pool::alloc(pool, targetSize);
	auto target = block;

	Buffer *first = front;
	while (first->absolute + first->capacity < initOffset) {
		first = first->next;
	}

	initOffset -= first->absolute;

	while (writeSize > 0 && first) {
		auto copySize = std::min(first->capacity - initOffset, writeSize);
		::memcpy(target, first->buf + initOffset, copySize);
		initOffset = 0;
		writeSize -= copySize;
		target += copySize;
		first = first->next;
	}

	return BytesView(block, targetSize);
}

void ConnectionWorker::BufferChain::releaseEmpty() {
	while (front && front->availableForRead() == 0) {
		auto f = front;
		front = front->next;
		if (&f->next ==tail) {
			tail = nullptr;
		}
		if (f == back) {
			back = nullptr;
		}
		f->release();
	}
}

ConnectionWorker::Client::Client(Generation *g, pool_t *p, int ifd, StringView inAddr, uint16_t inPort)
: gen(g), pool(p), addr(inAddr.pdup(p)), port(inPort) {
	memset(&event, 0, sizeof(event));
	event.data.ptr = this;
	event.events = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLRDHUP;
	fd = ifd;

	ConnectionQueue::setNonblocking(fd);
}

ConnectionWorker::Client::Client(int f, int mode) {
	memset(&event, 0, sizeof(event));
	fd = f;
	event.data.ptr = this;
	event.events = mode;
	system = true;
}

void ConnectionWorker::Client::shutdownRead() {
	if (!shutdownReadSend) {
		shutdownReadSend = true;
		::shutdown(event.data.fd, SHUT_RD);
	}
}

void ConnectionWorker::Client::shutdownWrite() {
	if (!shutdownWriteSend) {
		shutdownWriteSend = true;
		::shutdown(event.data.fd, SHUT_WR);
	}
}

void ConnectionWorker::Client::shutdownAll() {
	if (!shutdownReadSend && !shutdownWriteSend) {
		shutdownWriteSend = true;
		shutdownReadSend = true;
		::shutdown(event.data.fd, SHUT_RDWR);
	} else {
		shutdownRead();
		shutdownWrite();
	}
}

void ConnectionWorker::Client::release() {
	close(event.data.fd);
	if (gen) {
		gen->releaseClient(this);
	}

	if (pool) {
		pool::destroy(pool);
	}
}

bool ConnectionWorker::Client::performRead() {
	if (shutdownReadSend) {
		return false;
	}

	input.readFromFd(pool, fd);

	if (input) {
		auto ret = runInputFilter(input);
		switch (ret) {
		case OK:
		case DONE:
		case DECLINED:
		case SUSPENDED:
			break;
		default:
			cancelWithResult(ret);
			return false;
		}
	}

	return true;
}

bool ConnectionWorker::Client::performWrite() {
	if (!output || !valid) {
		return true;
	}

	auto ret = output.writeToFd(fd, bytesSent);
	switch (ret) {
	case SUSPENDED:
		return true;
	case DONE: // eos found
	case DECLINED: // unrecoverable error
		shutdownWrite();
		return false;
	default:
		break;
	}

	return true;
}

bool ConnectionWorker::Client::write(BufferChain &target, BufferChain &source) {
	if (output.isEos() || shutdownWriteSend) {
		return false;
	}

	if (!target) {
		auto ret = source.writeToFd(fd, bytesSent);
		switch (ret) {
		case DONE:
			shutdownWrite();
			return true;
			break;
		case OK:
		case SUSPENDED:
			return true;
			break;
		case DECLINED:
			shutdownWrite();
			return false;
			break;
		default:
			break;
		}

		if (source) {
			target.write(source);
		}
	} else {
		target.write(source);
	}
	return true;
}

bool ConnectionWorker::Client::write(BufferChain &chain, const uint8_t *buf, size_t size, Buffer::Flags flags) {
	if (output.isEos() || shutdownWriteSend) {
		return false;
	}

	if (!chain && &chain == &output) {
		auto ret = ::write(fd, buf, size);
		while (ret > 0) {
			buf += ret; size -= ret;
			if (size > 0) {
				ret = ::write(fd, buf, size);
			} else {
				ret = 0;
			}
		}

		if (ret < 0) {
			if (errno != EAGAIN && errno != EWOULDBLOCK) {
				log::error("ConnectionWorker", "fail to write to client");
				size = 0;
				flags = Buffer::Flags::Eos;
			}
		} else {
			bytesSent += ret;
		}
	}

	if (size > 0) {
		return chain.write(pool, buf, size, flags);
	} else if (size == 0 && flags != Buffer::None) {
		if ((flags & Buffer::Eos) != Buffer::None) {
			shutdownWrite();
			return true;
		} else {
			return chain.write(pool, nullptr, 0, flags);
		}
	}
	return true;
}

bool ConnectionWorker::Client::write(BufferChain &chain, StringView data, Buffer::Flags flags) {
	return write(chain, (const uint8_t *)data.data(), data.size(), flags);
}

bool ConnectionWorker::Client::write(BufferChain &chain, BytesView data, Buffer::Flags flags) {
	return write(chain, data.data(), data.size(), flags);
}

bool ConnectionWorker::Client::writeFile(BufferChain &chain, StringView filename, size_t offset, size_t size, Buffer::Flags flags) {
	if (output.isEos() || shutdownWriteSend) {
		return false;
	}

	auto buf = Buffer::create(pool, filename, offset, size, bytesSent);
	buf->flags |= flags;
	if (!buf) {
		return false;
	}

	if (!chain.write(buf)) {
		buf->release();
		return false;
	}

	if (!chain && &chain == &output) {
		auto ret = chain.writeToFd(fd, bytesSent);
		switch (ret) {
		case DONE:
		case SUSPENDED:
			return true;
			break;
		case DECLINED:
			// eos found
			shutdownWrite();
			return false;
			break;
		default:
			break;
		}
	}

	return true;
}

Status ConnectionWorker::Client::runInputFilter(BufferChain &chain) {
	while (!chain.empty()) {
		switch (requestState) {
		case RequestLine: {
			auto ret = checkForReqeust(chain);
			switch (ret) {
			case DECLINED:
				return HTTP_REQUEST_ENTITY_TOO_LARGE;
				break;
			case DONE:
				request = gen->worker->readRequest(this, chain);
				if (!request) {
					shutdownRead();
					return HTTP_BAD_REQUEST;
				} else {
					requestState = RequestHeaders;
				}
				break;
			default:
				break;
			}
			break;
		}
		case RequestHeaders: {
			auto ret = checkForHeader(chain);
			switch (ret) {
			case DECLINED:
				return HTTP_REQUEST_ENTITY_TOO_LARGE;
				break;
			case DONE:
				ret = gen->worker->parseRequestHeader(request, this, chain);
				switch (ret) {
				case DECLINED:
					return HTTP_INTERNAL_SERVER_ERROR;
					break;
				case OK:
					break;
				case DONE:
					requestState = RequestProcess;
					break;
				default:
					return ret;
					break;
				}
				break;
			default:
				break;
			}
			break;
		}
		case RequestProcess: {
			return HTTP_INTERNAL_SERVER_ERROR;
			break;
		}
		case RequestInput: {
			auto ret = request->processInput(chain);
			switch (ret) {
			case DECLINED:
				return HTTP_BAD_REQUEST;
				break;
			case OK:
			case SUSPENDED:
				break;
			case DONE:
				write(response, nullptr, 0, Buffer::Flags::Eos);
				requestState = ReqeustClosed;
				return DONE;
				break;
			default:
				return ret;
				break;
			}
			break;
		}
		case ReqeustClosed: {
			return DONE;
			break;
		}
		}
		if (requestState == RequestProcess) {
			auto ret = gen->worker->processRequest(request);
			switch (ret) {
			case OK:
				if (request->getInputFilter() && request->getInfo().contentLength > 0) {
					requestState = RequestInput;
				}
				break;
			default:
				return ret;
				break;
			}
		}
	}
	return SUSPENDED;
}

Status ConnectionWorker::Client::checkForReqeust(BufferChain &chain) {
	bool found = false;
	return chain.read([&] (const Buffer *buf, const uint8_t *b, size_t s) {
		if (found) {
			return int(DONE);
		}

		if (buf->absolute > config::UNIX_MAX_REQUEST_LINE) {
			return int(DECLINED);
		}

		StringView r((const char *)b, std::min(s, config::UNIX_MAX_REQUEST_LINE - buf->absolute));
		r.skipUntil<StringView::Chars<'\n'>>();
		if (r.is('\n')) {
			found = true;
			++ r;
			return int((const uint8_t *)r.data() - b);
		}

		if (buf->absolute + s >= config::UNIX_MAX_REQUEST_LINE) {
			return int(DECLINED);
		}

		return int((const uint8_t *)r.data() - b);
	}, false);
}

Status ConnectionWorker::Client::checkForHeader(BufferChain &chain) {
	bool found = false;
	auto ret = chain.read([&] (const Buffer *buf, const uint8_t *b, size_t s) {
		if (found) {
			return int(DONE);
		}

		if (buf->absolute > config::UNIX_MAX_HEADER_LINE + bytesRead) {
			return int(DECLINED);
		}

		StringView r((const char *)b, std::min(s, config::UNIX_MAX_REQUEST_LINE - buf->absolute));
		r.skipUntil<StringView::Chars<'\n'>>();
		if (r.is('\n')) {
			found = true;
			++ r;
			return int((const uint8_t *)r.data() - b);
		}

		if ((buf->absolute + s) >= config::UNIX_MAX_REQUEST_LINE + bytesRead) {
			return int(DECLINED);
		}

		return int((const uint8_t *)r.data() - b);
	}, false);
	if (ret == OK) {
		return found ? DONE : OK;
	}
	return ret;
}

void ConnectionWorker::Client::cancelWithResult(Status status) {
	if (output.isEos() || shutdownWriteSend) {
		return;
	}

	perform([&] {
		Time date = Time::now();
		Value result {
			pair("date", Value(date.toMicros())),
			pair("status", Value(toInt(status)))
		};

		auto data = data::write<Interface>(result, data::EncodeFormat::Json);

		StringView crlf("\r\n");
		StringView statusLine = getStatusLine(status);
		if (statusLine.empty()) {
			statusLine = getStatusLine(HTTP_INTERNAL_SERVER_ERROR);
		}

		sp_time_exp_t xt(date);
		char dateBuf[30] = { 0 };
		xt.encodeRfc822(dateBuf);

		auto outFn = [&] (StringView str) {
			write(output, str);
		};

		auto out = Callback<void(StringView)>(outFn);

		out << StringView("HTTP/1.1 ") << statusLine << crlf;
		out << StringView("Date: ") << dateBuf << crlf;
		out << StringView("Connection: close\r\n");
		out << StringView("Server: ") << gen->worker->getRoot()->getServerNameLine() << crlf;
		out << StringView("Content-Length: ") << data.size() << crlf;
		out << StringView("Content-Type: application/json; charset=utf-8\r\n");
		out << crlf;

		write(output, data, Buffer::Flags::Eos);
	}, pool);

}

ConnectionWorker::Generation::Generation(ConnectionWorker *w, pool_t *p)
: pool(p), worker(w) { }

ConnectionWorker::Client *ConnectionWorker::Generation::pushFd(int fd, StringView addr, uint16_t port) {
	auto clientPool = pool::create(pool);
	ConnectionWorker::Client *ret = nullptr;

	auto memBlock = pool::palloc(clientPool, sizeof(Client));
	ret = new (memBlock) Client(this, clientPool, fd, addr, port);

	ret->next = active;
	ret->prev = nullptr;
	if (active) { active->prev = ret; }
	active = ret;

	++ activeClients;

	return ret;
}

void ConnectionWorker::Generation::releaseClient(Client *client) {
	if (client == active) {
		active = client->next;
		if (active) { active->prev = nullptr; }
	} else {
		if (client->prev) { client->prev->next = client->next; }
		if (client->next) { client->next->prev = client->prev; }
	}

	-- activeClients;
}

void ConnectionWorker::Generation::releaseAll() {
	while (active) {
		releaseClient(active);
	}
}

ConnectionWorker::Generation *ConnectionWorker::makeGeneration() {
	auto p = pool::create(_threadPool);
	return new (p) Generation(this, p);
}

}
