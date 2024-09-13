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

#ifndef EXTRA_WEBSERVER_WEBSERVER_WEBSOCKET_SPWEBWEBSOCKETCONNECTION_H_
#define EXTRA_WEBSERVER_WEBSERVER_WEBSOCKET_SPWEBWEBSOCKETCONNECTION_H_

#include "SPWebWebsocket.h"
#include "SPWebAsyncTask.h"

namespace STAPPLER_VERSIONIZED stappler::web {

class Request;
class WebserverHandler;

class SP_PUBLIC WebsocketConnection : public AllocBase {
public:
	static void destroy(WebsocketConnection *);

	virtual ~WebsocketConnection();

	bool isEnabled() const { return _enabled; }

	virtual bool write(WebsocketFrameType t, const uint8_t *bytes = nullptr, size_t count = 0) = 0;

	virtual bool run(WebsocketHandler *, const Callback<void()> &beginCb, const Callback<void()> &endCb) = 0;

	virtual void wakeup() = 0;

	virtual void terminate();

	db::AccessRoleId getAccessRole() const { return _accessRole; }
	void setAccessRole(db::AccessRoleId);

	void setStatusCode(WebsocketStatusCode, StringView = StringView());

	pool_t *getPool() const { return _pool; }

	pool_t *getHandlePool() const;

	Host getHost() const;

	bool performAsync(const Callback<void(AsyncTask &)> &cb) const;

protected:
	// updates with last read status
	WebsocketStatusCode resolveStatus(WebsocketStatusCode code);

	WebsocketConnection(allocator_t *, pool_t *, HostController *);

	allocator_t *_allocator = nullptr;
	pool_t *_pool = nullptr;
	Mutex _mutex;
	std::atomic_flag _shouldTerminate;

	bool _enabled = false;
	bool _connected = true;

	String _serverReason;
	WebsocketStatusCode _clientCloseCode = WebsocketStatusCode::None;
	WebsocketStatusCode _serverCloseCode = WebsocketStatusCode::Auto;

	db::AccessRoleId _accessRole = db::AccessRoleId::Nobody;

	mutable AsyncTaskGroup _group;

	WebsocketFrameReader *_commonReader = nullptr;
	WebsocketFrameWriter *_commonWriter = nullptr;
};

}

#endif /* EXTRA_WEBSERVER_WEBSERVER_WEBSOCKET_SPWEBWEBSOCKETCONNECTION_H_ */
