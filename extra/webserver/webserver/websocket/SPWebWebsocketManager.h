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

#ifndef EXTRA_WEBSERVER_WEBSERVER_WEBSOCKET_SPWEBWEBSOCKETMANAGER_H_
#define EXTRA_WEBSERVER_WEBSERVER_WEBSOCKET_SPWEBWEBSOCKETMANAGER_H_

#include "SPWebWebsocket.h"

namespace STAPPLER_VERSIONIZED stappler::web {

class WebsocketConnection;

class SP_PUBLIC WebsocketManager : public AllocBase {
public:
	static String makeAcceptKey(StringView key);

	WebsocketManager(const Host &);
	virtual ~WebsocketManager();

	virtual WebsocketHandler * onAccept(const Request &, pool_t *);
	virtual bool onBroadcast(const Value &);

	size_t size() const;

	void receiveBroadcast(const Value &);
	Status accept(Request &);

	void run(WebsocketHandler *);

	const Host &host() const { return _host; }

protected:
	void addHandler(WebsocketHandler *);
	void removeHandler(WebsocketHandler *);

	pool_t *_pool;
	Mutex _mutex;
	std::atomic<size_t> _count;
	Vector<WebsocketHandler *> _handlers;
	Host _host;
};

class SP_PUBLIC WebsocketHandler : public AllocBase {
public:
	WebsocketHandler(WebsocketManager *m, pool_t *p, StringView url,
			TimeInterval ttl = config::WEBSOCKET_DEFAULT_TTL,
			size_t max = config::WEBSOCKET_DEFAULT_MAX_FRAME_SIZE);
	virtual ~WebsocketHandler();

	// Client just connected
	virtual void handleBegin() { }

	// Data frame was recieved from network
	virtual bool handleFrame(WebsocketFrameType, const Bytes &);

	// Message was recieved from broadcast
	virtual bool handleMessage(const Value &);

	// Client is about disconnected
	// You can not send any frames in this call, because 'close' frame was already sent
	virtual void handleEnd() { }

	// Send system-wide broadcast, that can be received by any other websocket with same servername and url
	// This socket also receive this broadcast
	void sendBroadcast(Value &&) const;

	void setEncodeFormat(const data::EncodeFormat &);

	bool send(StringView);
	bool send(BytesView);
	bool send(const Value &);

	// get default storage adapter, that binds to current call context
	WebsocketManager *manager() const { return _manager; }
	WebsocketConnection *connection() const { return _conn; }
	pool_t *pool() const;

	StringView getUrl() const { return _url; }
	TimeInterval getTtl() const { return _ttl; }
	size_t getMaxInputFrameSize() const { return _maxInputFrameSize; }

	bool isEnabled() const;

	void sendPendingNotifications(pool_t *);

	void performWithStorage(const Callback<void(const db::Transaction &)> &cb) const;

	bool performAsync(const Callback<void(AsyncTask &)> &cb) const;

	bool processBroadcasts();

protected:
	friend class WebsocketManager;
	friend class WebsocketConnection;

	void setConnection(WebsocketConnection *);
	virtual void receiveBroadcast(const Value &);

	pool_t *_pool = nullptr;
	WebsocketManager *_manager = nullptr;

	data::EncodeFormat _format = data::EncodeFormat::Json;
	StringView _url;
	TimeInterval _ttl = config::WEBSOCKET_DEFAULT_TTL;
	size_t _maxInputFrameSize = config::WEBSOCKET_DEFAULT_MAX_FRAME_SIZE;

	Mutex _broadcastMutex;
	pool_t *_broadcastsPool = nullptr;
	Vector<Value> *_broadcastsMessages = nullptr;

	WebsocketConnection *_conn = nullptr;
};

}

#endif /* EXTRA_WEBSERVER_WEBSERVER_WEBSOCKET_SPWEBWEBSOCKETMANAGER_H_ */
