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

#include "SPWebHttpdHost.h"
#include "SPWebHostComponent.h"
#include "SPDso.h"
#include "http_config.h"

namespace STAPPLER_VERSIONIZED stappler::web {

HttpdHostController *HttpdHostController::create(HttpdRoot *root, server_rec *server) {
	if (auto c = get(server)) {
		return c;
	}

	auto pool = (pool_t *)server->process->pconf;

	return perform([&] () -> HttpdHostController * {
		return new (pool) HttpdHostController(root, pool, server);
	}, pool);
}

HttpdHostController *HttpdHostController::get(server_rec *server) {
	if (!server) {
		return nullptr;
	}

	auto cfg = (HttpdHostController *)ap_get_module_config(server->module_config, &stappler_web_module);
	if (cfg) {
		return cfg;
	}

	return nullptr;
}

HttpdHostController * HttpdHostController::merge(HttpdHostController *base, HttpdHostController *add) {
	if (!base->_sourceRoot.empty()) {
		if (add->_sourceRoot.empty()) {
			add->_sourceRoot = base->_sourceRoot;
		} else {
			auto pool = add->_rootPool;

			perform([&] {
				Set<StringView> strings;
				for (auto &it : base->_sourceRoot) {
					strings.emplace(it);
				}
				for (auto &it : add->_sourceRoot) {
					strings.erase(it);
				}
				if (!strings.empty()) {
					for (auto &it : strings) {
						add->_sourceRoot.emplace_back(it);
					}
				}
			}, pool);
		}
	}

	if (!base->_components.empty()) {
		if (add->_components.empty()) {
			add->_components = base->_components;
		} else {
			auto tmp = move(add->_components);
			add->_components = base->_components;

			for (auto &it : tmp) {
				add->_components.emplace(it.first, it.second);
			}
		}
	}

	if (!base->_allowedIps.empty()) {
		if (add->_allowedIps.empty()) {
			add->_allowedIps = base->_allowedIps;
		} else {
			auto tmp = move(add->_allowedIps);
			add->_allowedIps = base->_allowedIps;

			for (auto &it : tmp) {
				add->_allowedIps.emplace_back(move(it));
			}
		}
	}

	return add;
}

HttpdHostController::HttpdHostController(Root *root, pool_t *pool, server_rec *server)
: HostController(root, pool), _server(server) {
	_hostInfo.hostname = StringView(_server->server_hostname);
	core_server_config *sconf = (core_server_config *)ap_get_core_module_config(_server->module_config);
	_hostInfo.documentRoot = StringView(sconf->ap_document_root);
	_hostInfo.admin = StringView(_server->server_admin);
	_hostInfo.scheme = StringView(_server->server_scheme);

	_hostInfo.timeout = TimeInterval::microseconds(_server->timeout);
	_hostInfo.keepAlive = TimeInterval::microseconds(_server->keep_alive_timeout);
	_hostInfo.maxKeepAlives = _server->keep_alive_max;
	_hostInfo.useKeepAlive = _server->keep_alive;
	_hostInfo.port = _server->port;
	_hostInfo.isVirtual = _server->is_virtual;

	ap_set_module_config(server->module_config, &stappler_web_module, this);
}

void HttpdHostController::handleChildInit(const Host &host, pool_t *p) {
	_hostInfo.hostname = StringView(_server->server_hostname);
	core_server_config *sconf = (core_server_config *)ap_get_core_module_config(_server->module_config);
	_hostInfo.documentRoot = StringView(sconf->ap_document_root);
	_hostInfo.admin = StringView(_server->server_admin);
	_hostInfo.scheme = StringView(_server->server_scheme);

	_hostInfo.timeout = TimeInterval::microseconds(_server->timeout);
	_hostInfo.keepAlive = TimeInterval::microseconds(_server->keep_alive_timeout);
	_hostInfo.maxKeepAlives = _server->keep_alive_max;
	_hostInfo.useKeepAlive = _server->keep_alive;
	_hostInfo.port = _server->port;
	_hostInfo.isVirtual = _server->is_virtual;

	HostController::handleChildInit(host, p);
}

db::sql::Driver * HttpdHostController::openInternalDriver(db::sql::Driver::Handle db) {
	auto ret = db::sql::Driver::open(_rootPool, _root, apr_dbd_name(((ap_dbd_t *)db.get())->driver), ((ap_dbd_t *)db.get())->driver);
	ret->init(db, Vector<StringView>());
	return ret;
}

}
