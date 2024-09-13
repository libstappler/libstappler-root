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

#ifndef EXTRA_WEBSERVER_UNIX_SPWEBUNIXROOT_H_
#define EXTRA_WEBSERVER_UNIX_SPWEBUNIXROOT_H_

#include "SPWebRoot.h"
#include "SPWebAsyncTask.h"
#include "SPWebWebsocket.h"

namespace STAPPLER_VERSIONIZED stappler::web {

class ConnectionQueue;
class UnixHostController;
class UnixWebsocketSim;

struct SP_PUBLIC UnixHostConfig {
	StringView hastname;
	StringView admin;
	StringView root;

	Vector<HostComponentInfo> components;
	Value db;
};

class SP_PUBLIC UnixRoot : public Root {
public:
	struct Config {
		StringView listen;
		Vector<UnixHostConfig> hosts;
		Value db;
		uint16_t nworkers = std::max(uint16_t(2), uint16_t(std::thread::hardware_concurrency() / 2));
	};

	static SharedRc<UnixRoot> create(Config &&);

	virtual ~UnixRoot();

	UnixRoot(pool_t *);

	// spawn thread to listen on unix socket
	bool init(Config &&);

	void cancel();

	virtual bool performTask(const Host &, AsyncTask *task, bool performFirst) override;
	virtual bool scheduleTask(const Host &, AsyncTask *task, TimeInterval) override;

	virtual void foreachHost(const Callback<void(Host &)> &) override;

	Status processRequest(RequestController *);

	bool simulateWebsocket(UnixWebsocketSim *sim, StringView hostname, StringView url);

protected:
	Status runDefaultProcessing(Request &);

	Map<StringView, UnixHostController *> _hosts;
	ConnectionQueue *_queue = nullptr;

	bool _running = false;
	std::mutex _mutex;
	std::condition_variable _cond;
};

}

#endif /* EXTRA_WEBSERVER_UNIX_SPWEBUNIXROOT_H_ */
