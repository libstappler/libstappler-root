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

#include "SPWebUnixRoot.h"
#include "SPFilesystem.h"
#include "SPWebUnixHost.h"
#include "SPWebUnixConnectionQueue.h"
#include "SPWebUnixRequest.h"
#include "SPWebRequest.h"
#include "SPThread.h"

namespace STAPPLER_VERSIONIZED stappler::web {

SharedRc<UnixRoot> UnixRoot::create(Config &&cfg) {
	return SharedRc<UnixRoot>::create(SharedRefMode::Allocator, move(cfg));
}

UnixRoot::~UnixRoot() { }

UnixRoot::UnixRoot(Ref *ref, pool_t *p) : Root(ref, p) { }

bool UnixRoot::init(Config &&config) {
	return perform([&, this] {
		size_t workers = std::thread::hardware_concurrency();
		if (config.nworkers >= 2 && config.nworkers <= 256) {
			workers = size_t(config.nworkers);
		}

		for (auto &it : config.hosts) {
			auto p = pool::create(_rootPool);
			pool::push(p);

			auto host = new (p) UnixHostController(this, p, it);

			_hosts.emplace(host->getHostInfo().hostname, host);

			pool::pop();
		}

		initDatabases();

		_queue = new (_rootPool) ConnectionQueue(this, _rootPool, workers, move(config));

		std::unique_lock<std::mutex> lock(_mutex);
		if (_queue->run()) {
			_running = true;

			for (auto &it : _hosts) {
				perform([&] {
					it.second->init(Host(it.second));
				}, _rootPool, config::TAG_HOST, it.second);
			}

			for (auto &it : _hosts) {
				perform([&] {
					Host(it.second).handleChildInit(_rootPool);
				}, _rootPool, config::TAG_HOST, it.second);
			}

			return true;
		}
		return false;
	}, _rootPool);
}

void UnixRoot::cancel() {
	if (!_running || !_queue) {
		return;
	}

	_running = false;
	_queue->cancel();
	_queue->release();
	_queue = nullptr;
}

bool UnixRoot::performTask(const Host &host, AsyncTask *task, bool performFirst) {
	if (_queue) {
		task->setHost(host);
		_queue->pushTask(task);
		return true;
	}
	return false;
}

bool UnixRoot::scheduleTask(const Host &host, AsyncTask *task, TimeInterval ival) {
	if (_queue) {
		task->setHost(host);
		_queue->pushTask(task);
		return true;
	}
	return false;
}

void UnixRoot::foreachHost(const Callback<void(Host &)> &cb) {
	for (auto &it : _hosts) {
		Host serv(it.second);
		cb(serv);
	}
}

Status UnixRoot::processRequest(RequestController *req) {
	Status ret = DECLINED;
	auto host = req->getInfo().url.host;

	auto it = _hosts.find(host);
	if (it != _hosts.end()) {
		req->bind(it->second);
	}

	if (!req->init()) {
		return ret;
	}

	Request rctx(req);

	ret = runPostReadRequest(rctx);
	switch (ret) {
	case OK:
		// continue processing
		break;
	case DECLINED:
		return runDefaultProcessing(rctx);
		break;
	default:
		return ret;
		break;
	}

	ret = runTranslateName(rctx);
	switch (ret) {
	case OK:
		// continue processing
		break;
	case DECLINED:
		return runDefaultProcessing(rctx);
		break;
	default:
		return ret;
		break;
	}

	ret = runCheckAccess(rctx);
	switch (ret) {
	case OK:
		// continue processing
		break;
	case DECLINED:
		return HTTP_FORBIDDEN;
		break;
	default:
		return ret;
		break;
	}

	ret = runQuickHandler(rctx, 0);
	switch (ret) {
	case OK:
	case DECLINED:
		// continue processing
		break;
	default:
		return ret;
		break;
	}

	runInsertFilter(rctx);

	switch (req->getInfo().status) {
	case OK:
	case DECLINED:
	case SUSPENDED:
		// continue processing
		break;
	default:
		return ret;
		break;
	}

	ret = runHandler(rctx);
	switch (ret) {
	case OK:
		if (rctx.getInputFilter()) {
			return SUSPENDED;
		}
		return DONE;
		break;
	case DECLINED:
		if (rctx.getInputFilter()) {
			return SUSPENDED;
		} else {
			return runDefaultProcessing(rctx);
		}
		break;
	default:
		break;
	}

	return ret;
}

bool UnixRoot::simulateWebsocket(UnixWebsocketSim *sim, StringView hostname, StringView url) {
	auto it = _hosts.find(hostname);
	if (it == _hosts.end()) {
		return false;
	}

	return it->second->simulateWebsocket(sim, url);
}

Status UnixRoot::runDefaultProcessing(Request &rctx) {
	auto &info = rctx.getInfo();
	auto filename = info.filename;
	if (filename.empty()) {
		rctx.setFilename(filepath::merge<Interface>(info.documentRoot, info.url.path), true);
	}

	if (info.filename.empty() || info.stat.type != filesystem::FileType::File) {
		return HTTP_NOT_FOUND;
	} else {
		if (info.contentType.empty()) {
			if (runTypeChecker(rctx) == DECLINED) {
				return HTTP_NOT_FOUND;
			}
		}

		if (runCheckAccess(rctx) != OK) {
			return HTTP_FORBIDDEN;
		}
		return DONE;
	}
}

}
