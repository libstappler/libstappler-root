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

#ifndef EXTRA_WEBSERVER_UNIX_SPWEBUNIXCONNECTIONWORKER_H_
#define EXTRA_WEBSERVER_UNIX_SPWEBUNIXCONNECTIONWORKER_H_

#include "SPWebUnixRoot.h"
#include "SPWebRequestController.h"
#include "SPThread.h"

#include <sys/epoll.h>

namespace STAPPLER_VERSIONIZED stappler::web {

class UnixRequestController;
class ConnectionQueue;

class SP_PUBLIC ConnectionWorker : public thread::Thread {
public:
	struct Buffer;
	struct Client;
	struct Generation;

	struct BufferFile {
		filesystem::Stat stat;
		int fd = -1;
		uint8_t *extraBuffer;
	};

	struct Buffer : AllocBase {
		enum Flags {
			None = 0,
			Eos = 1 << 0,
			IsOutFile = 1 << 1,
		};

		Buffer *next = nullptr;
		pool_t *pool = nullptr;

		uint8_t *buf = nullptr;
		size_t capacity = 0;
		size_t size = 0;
		size_t offset = 0;
		size_t absolute = 0;
		Flags flags = Flags::None;

		static Buffer *create(pool_t *, size_t = 0);
		static Buffer *create(pool_t *, StringView path, off_t rangeStart, size_t rangeLen = maxOf<size_t>(), size_t = 0);

		void release();

		StringView str() const;

		size_t availableForWrite() const;
		size_t availableForRead() const;

		uint8_t *writeTarget() const;
		uint8_t *readSource() const;

		size_t write(const uint8_t *, size_t);

		BufferFile *getFile() const;

		bool isOutFile() const { return (flags & IsOutFile) != None; }
	};

	struct BufferChain {
		Buffer *front = nullptr;
		Buffer *back = nullptr;
		Buffer **tail = nullptr;

		bool eos = false;

		explicit operator bool() const { return front != nullptr; }

		bool isSingle() const { return front != nullptr && front == back; }

		bool isEos() const;

		bool empty() const;

		size_t size() const;

		Buffer *getWriteTarget(pool_t *p);

		bool write(pool_t *, const uint8_t *, size_t, Buffer::Flags flags = Buffer::None);
		bool write(Buffer *);
		bool write(BufferChain &);
		bool readFromFd(pool_t *, int);

		Status read(const Callback<int(const Buffer *, const uint8_t *, size_t)> &, bool release);
		Status writeToFd(int, size_t &);

		size_t getBytesRead() const;

		BytesView extract(pool_t *, size_t initOffset, size_t blockSize) const;

		void releaseEmpty();
		void clear();
	};

	struct Client : AllocBase {
		enum RequestReadState {
			RequestLine,
			RequestHeaders,
			RequestProcess,
			RequestInput,
			ReqeustClosed,
			ReqeustInvalid,
		};

		Client *next = nullptr;
		Client *prev = nullptr;

		Generation *gen = nullptr;
		pool_t *pool = nullptr;

		BufferChain input;
		BufferChain output;
		BufferChain response;

		StringView addr;
		uint16_t port = 0;

		int fd = -1;
		struct epoll_event event;
		bool system = false;
		bool valid = true;
		bool shutdownReadSend = false;
		bool shutdownWriteSend = false;

		RequestReadState requestState = RequestReadState::RequestLine;
		UnixRequestController *request = nullptr;
		size_t bytesSent = 0;
		size_t bytesRead = 0;

		Client(Generation *, pool_t *, int, StringView addr, uint16_t port);
		Client(int fd, int mode);

		void init(int);
		void shutdownRead();
		void shutdownWrite();
		void shutdownAll();
		void release();

		bool performRead();
		bool performWrite();

		bool write(BufferChain &, BufferChain &);
		bool write(BufferChain &, const uint8_t *, size_t, Buffer::Flags = Buffer::None);
		bool write(BufferChain &, StringView, Buffer::Flags = Buffer::None);
		bool write(BufferChain &, BytesView, Buffer::Flags = Buffer::None);
		bool writeFile(BufferChain &, StringView filename, size_t offset = 0, size_t size = maxOf<size_t>(), Buffer::Flags = Buffer::None);

		Status runInputFilter(BufferChain &);

		Status checkForReqeust(BufferChain &);
		Status checkForHeader(BufferChain &);

		void cancelWithResult(Status);
	};

	struct Generation : AllocBase {
		Generation *prev = nullptr;
		Generation *next = nullptr;
		Client *active = nullptr;
		size_t activeClients = 0;

		pool_t *pool = nullptr;
		ConnectionWorker *worker = nullptr;
		bool endOfLife = false;

		Generation(ConnectionWorker *, pool_t *);

		Client *pushFd(int, StringView addr, uint16_t port);
		void releaseClient(Client *);
		void releaseAll();
	};

	static constexpr size_t MaxEvents = 16;

	ConnectionWorker(ConnectionQueue *queue, UnixRoot *, int socket, int pipe, int event);
	~ConnectionWorker();

	virtual void threadInit() override;
	virtual void threadDispose() override;
	virtual bool worker() override;

	bool poll(int);

	Root *getRoot() const { return _root; }

	std::thread & thread() { return _thisThread; }

	void runTask(AsyncTask *);

	UnixRequestController *readRequest(Client *, BufferChain &chain);
	Status parseRequestHeader(UnixRequestController *, Client *, BufferChain &chain);
	Status processRequest(UnixRequestController *);

protected:
	Generation *makeGeneration();
	void pushFd(int epollFd, int fd, StringView addr, uint16_t port);

	bool addClient(Client &);
	void removeClient(Client &);

	ConnectionQueue *_queue;

	UnixRoot *_root = nullptr;

	Client _inputClient;
	Client _cancelClient;
	Client _eventClient;

	bool _shouldClose = false;
	int _epollFd = -1;
	int _signalFd = -1;

	size_t _fdCount = 0;

	Generation *_generation = nullptr;
};

SP_DEFINE_ENUM_AS_MASK(ConnectionWorker::Buffer::Flags)

}

#endif /* EXTRA_WEBSERVER_UNIX_SPWEBUNIXCONNECTIONWORKER_H_ */
