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

#include "SPWebUnixHost.h"
#include "SPWebHostComponent.h"
#include "SPWebRequestFilter.h"
#include "SPWebWebsocketManager.h"
#include "SPWebUnixWebsocket.h"
#include "SPDso.h"
#include "SPValid.h"

namespace STAPPLER_VERSIONIZED stappler::web {

UnixHostController::~UnixHostController() { }

UnixHostController::UnixHostController(Root *root, pool_t *p, UnixHostConfig &cfg)
: HostController(root, p) {
	_hostInfo.hostname = cfg.hastname.pdup(p);
	_hostInfo.admin = cfg.admin.pdup(p);
	_hostInfo.documentRoot = cfg.root.pdup(p);

	_componentsToLoad = move(cfg.components);
	for (auto &it : _componentsToLoad) {
		if (!it.file.empty()) { it.file = it.file.pdup(_rootPool); }
		if (!it.name.empty()) { it.name = it.name.pdup(_rootPool); }
		if (!it.version.empty()) { it.version = it.version.pdup(_rootPool); }
		if (!it.symbol.empty()) { it.symbol = it.symbol.pdup(_rootPool); }
	}

	if (cfg.db) {
		for (auto &it : cfg.db.asDict()) {
			_dbParams.emplace(StringView(it.first).pdup(p), StringView(it.second.getString()).pdup(p));
		}
	}
}

bool UnixHostController::simulateWebsocket(UnixWebsocketSim *sim, StringView url) {
	auto tmp = url;
	auto sub = tmp.readUntil<StringView::Chars<'?'>>();
	auto it = _websockets.find(sub);
	if (it == _websockets.end()) {
		return false;
	}

	UnixRequestController *cfg = nullptr;
	allocator_t *alloc = nullptr;
	pool_t *pool = nullptr;

	alloc = allocator::create();
	pool = pool::create(alloc, memory::PoolFlags::None);

	return perform([&] {
		auto key = base64::encode<Interface>(valid::makeRandomBytes<Interface>(16));
		auto requestSource = toString("GET ", url, " HTTP/1.1\r\n"
				"Host: ", _hostInfo.hostname, "\r\n"
				"sec-websocket-version: 13\r\n"
				"sec-websocket-key: ", key, "\r\n");
		StringView source(requestSource);
		RequestInfo info;
		RequestFilter::readRequestLine(source, info);

		cfg = new (pool) UnixRequestController(pool, info.clone(pool), sim);
		cfg->bind(this);
		cfg->init();

		BytesView bytes((const uint8_t *)source.data(), source.size());
		while (!bytes.empty()) {
			StringView name;
			StringView value;

			if (RequestFilter::readRequestHeader(bytes, name, value) > 0) {
				cfg->setRequestHeader(name, value);
			}
		}

		sim->attachRequest(alloc, pool, cfg);

		Request req(cfg);
		perform([&] {
			it->second->accept(req);
		}, pool, config::TAG_REQUEST, cfg);
		return true;
	}, pool);
}

}
