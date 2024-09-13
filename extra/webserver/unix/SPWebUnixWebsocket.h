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

#ifndef EXTRA_WEBSERVER_UNIX_SPWEBUNIXWEBSOCKET_H_
#define EXTRA_WEBSERVER_UNIX_SPWEBUNIXWEBSOCKET_H_

#include "SPWebWebsocket.h"
#include "SPWebWebsocketConnection.h"

namespace STAPPLER_VERSIONIZED stappler::web {

class UnixRequestController;
class UnixWebsocketConnectionSim;

class SP_PUBLIC UnixWebsocketSim {
public:
	virtual ~UnixWebsocketSim();

	virtual void attachRequest(allocator_t *, pool_t *, UnixRequestController *);
	virtual void attachSocket(UnixWebsocketConnectionSim *);

	virtual bool read(WebsocketFrameType t, const uint8_t *bytes, size_t count);
	virtual bool write(BytesView frame);

	virtual void send(const mem_std::Value &);

	virtual void onStarted();
	virtual void onEnded();

protected:
	allocator_t *_alloc = nullptr;
	pool_t *_pool = nullptr;
	UnixRequestController *_request = nullptr;
	UnixWebsocketConnectionSim *_socket = nullptr;
};

class SP_PUBLIC UnixWebsocketConnectionSim : public WebsocketConnection {
public:
	UnixWebsocketConnectionSim(allocator_t *, pool_t *, HostController *c, UnixWebsocketSim *sim);

	virtual bool write(WebsocketFrameType t, const uint8_t *bytes = nullptr, size_t count = 0) override;
	void receive(const mem_std::Value &);

	virtual bool run(WebsocketHandler *, const Callback<void()> &beginCb, const Callback<void()> &endCb) override;

	virtual void wakeup() override;

	void read(BytesView);

protected:
	bool processFrame(BytesView);

	UnixWebsocketSim *_sim = nullptr;
	WebsocketFrameReader _reader;
	WebsocketFrameWriter _writer;
	WebsocketHandler *_handler = nullptr;

	std::mutex _waitMutex;
	std::condition_variable _waitCond;
	std::vector<mem_std::Bytes> _inputFrames;
	std::vector<mem_std::Value> _inputValues;
};

}

#endif /* EXTRA_WEBSERVER_UNIX_SPWEBUNIXWEBSOCKET_H_ */
