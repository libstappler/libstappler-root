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

#include "SPWebHost.h"
#include "SPWebHostController.h"
#include "SPWebHostComponent.h"
#include "SPWebRequestController.h"
#include "SPWebResourceHandler.h"
#include "SPWebWebsocketConnection.h"
#include "SPWebAsyncTask.h"
#include "SPWebRoot.h"
#include "SPWebRequest.h"
#include "SPWebTools.h"

#include "SPGost3411-2012.h"
#include "SPDbUser.h"
#include "SPValid.h"

namespace STAPPLER_VERSIONIZED stappler::web {

static HostController *getHostFromContext(pool_t *p, uint32_t tag, const void *ptr) {
	switch (tag) {
	case uint32_t(config::TAG_HOST): return (HostController *)ptr; break;
	case uint32_t(config::TAG_REQUEST): return ((RequestController *)ptr)->getHost(); break;
	case uint32_t(config::TAG_WEBSOCKET): return ((WebsocketConnection *)ptr)->getHost().getController(); break;
	}
	return nullptr;
}

Host Host::getCurrent() {
	HostController *ret = nullptr;
	pool::foreach_info(&ret, [] (void *ud, pool_t *p, uint32_t tag, const void *data) -> bool {
		auto ptr = getHostFromContext(p, tag, data);
		if (ptr) {
			*((HostController **)ud) = ptr;
			return false;
		}
		return true;
	});

	return Host(ret);
}

Host::Host() : _config(nullptr) { }
Host::Host(HostController *cfg) : _config(cfg) { }

Host & Host::operator =(HostController *cfg) {
	_config = cfg;
	return *this;
}

Host::Host(Host &&other) : _config(other._config) { }

Host & Host::operator =(Host &&other) {
	_config = other._config;
	return *this;
}

Host::Host(const Host &other) : _config(other._config) { }

Host & Host::operator =(const Host &other) {
	_config = other._config;
	return *this;
}

void Host::handleChildInit(pool_t *rootPool) {
	perform([&] {
		_config->init(*this);
		_config->handleChildInit(*this, rootPool);

		filesystem::mkdir(filepath::merge<Interface>(_config->_hostInfo.documentRoot, ".reports"));
		filesystem::mkdir(filepath::merge<Interface>(_config->_hostInfo.documentRoot, "uploads"));

		_config->_currentComponent = StringView("root");
		tools::registerTools(config::TOOLS_SERVER_PREFIX, *this);
		_config->_currentComponent = StringView();

		addProtectedLocation("/.reports");
		addProtectedLocation("/uploads");

		AsyncTask::perform(*this, [&] (AsyncTask &task) {
			task.addExecuteFn([serv = *this] (const AsyncTask &task) -> bool {
				serv.processReports();
				return true;
			});
		});
	}, rootPool, config::TAG_HOST, _config);
}

enum class HostReportType {
	Crash,
	Update,
	Error,
};

template <typename Callback> static
void Host_prepareEmail(HostController *cfg, Callback &&cb, HostReportType type) {
	/*StringStream data;
	auto &webhookInfo = cfg->getWebhookInfo();
	if (!webhookInfo.url.empty() && !webhookInfo.name.empty()) {
		auto &from = webhookInfo.name;
		auto &to = webhookInfo.extra.getString("to");
		auto &title = webhookInfo.extra.getString("title");

		NetworkHandle notify;
		notify.init(network::Method::Smtp, webhookInfo.url);
		notify.setAuthority(from, webhookInfo.extra.getString("password"));
		notify.setMailFrom(from);
		notify.addMailTo(to);

		data << "From: " << from << " <" << from << ">\r\n"
			<< "Content-Type: text/plain; charset=utf-8\r\n"
			<< "To: " << to << " <" << to << ">\r\n";

		switch (type) {
		case HostReportType::Crash:
			data << "Subject: Serenity Crash report";
			break;
		case HostReportType::Update:
			data << "Subject: Serenity Update report";
			break;
		case HostReportType::Error:
			data << "Subject: Serenity Error report";
			break;
		}

		if (!title.empty()) {
			data << " (" << title << ")";
		}
		data << "\r\n\r\n";

		cb(data);

		StringView r(data.data(), data.size());
		notify.setSendCallback([&] (char *buf, size_t size) -> size_t {
			auto writeSize = std::min(size, r.size());
			memcpy(buf, r.data(), writeSize);
			r.offset(writeSize);
			return writeSize;
		}, data.size());

		notify.perform();
	}*/
}

void Host::processReports() const {
	if (_config->getWebhookInfo().format != "email") {
		return;
	}

	Vector<Pair<StringView, HostReportType>> crashFiles;
	String path = filepath::absolute<Interface>(".reports", true);
	filesystem::ftw(path, [&] (const StringView &view, bool isFile) {
		if (isFile) {
			StringView r(view);
			r.skipString(path);
			if (r.starts_with("/crash.")) {
				crashFiles.emplace_back(view, HostReportType::Crash);
			} else if (r.starts_with("/update.")) {
				crashFiles.emplace_back(view, HostReportType::Update);
			}
		}
	});

	Vector<Pair<String, HostReportType>> crashData;
	for (auto &it : crashFiles) {
		crashData.emplace_back(filesystem::readTextFile<Interface>(it.first), it.second);
		filesystem::remove(it.first);
	}

	for (auto &it : crashData) {
		Host_prepareEmail(_config, [&] (StringStream &data) {
			data << it.first << "\r\n";
		}, it.second);
	}
}

void Host::performWithStorage(const Callback<void(const db::Transaction &)> &cb, bool openNewConnecton) const {
	if (!openNewConnecton) {
		if (auto t = db::Transaction::acquireIfExists()) {
			cb(t);
			return;
		}
	}

	auto targetPool = pool::acquire();
	perform([&] {
		auto handle = _config->openConnection(targetPool, false);
		if (handle.get()) {
			_config->_dbDriver->performWithStorage(handle, [&] (const db::Adapter &a) {
				if (auto t = db::Transaction::acquire(a)) {
					cb(t);
					t.release();
				}
			});
			_config->closeConnection(handle);
		}
	}, targetPool);
}

db::BackendInterface *Host::acquireDbForRequest(const Request &req) const {
	if (_config->_customDbd) {
		auto handle = _config->_customDbd->openConnection(req.pool());
		if (handle.get()) {
			pool::cleanup_register(req.pool(), [handle, dbd = _config->_customDbd] {
				dbd->closeConnection(handle);
			});

			return _config->_dbDriver->acquireInterface(handle, req.pool());
		}
	} else {
		auto handle = getRoot()->dbdAcquire(req);
		if (handle.get()) {
			return _config->_dbDriver->acquireInterface(handle, req.pool());
		}
	}
	return nullptr;
}

bool Host::setHostKey(BytesView priv) const {
	auto key = crypto::PrivateKey(priv);
	if (key) {
		setHostKey(move(key));
		return true;
	}
	return false;
}

void Host::setHostKey(crypto::PrivateKey &&priv) const {
	_config->setHostKey(move(priv));
}

const crypto::PublicKey &Host::getHostPublicKey() const {
	return _config->_hostPubKey;
}

const crypto::PrivateKey &Host::getHostPrivateKey() const {
	return _config->_hostPrivKey;
}

BytesView Host::getHostSecret() const {
	return _config->_hostSecret;
}

void Host::addSourceRoot(StringView file) {
	_config->_sourceRoot.emplace_back(file.pdup(_config->_rootPool));
}

void Host::addComponentByParams(StringView str) {
	StringView r(str);
	r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();

	StringView handlerParams;
	if (r.is('"')) {
		++ r;
		handlerParams = r.readUntil<StringView::Chars<'"'>>();
		if (r.is('"')) {
			++ r;
		}
	} else {
		handlerParams = r.readUntil<StringView::CharGroup<CharGroupId::WhiteSpace>>();
	}

	StringView args[3];
	int64_t idx = 0;
	while (!handlerParams.empty() && idx < 3) {
		r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
		args[idx] = handlerParams.readUntil<StringView::Chars<':'>>();
		if (handlerParams.is(':')) {
			++ handlerParams;
		}
		++ idx;
		r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
	}

	if (idx == 1) {
		HostComponentInfo h;
		h.symbol = args[0];
		_config->_componentsToLoad.emplace_back(std::move(h));
	} else if (idx >= 2) {
		HostComponentInfo h;
		h.symbol = args[idx - 1].pdup(_config->_rootPool);
		h.file = args[idx - 2].pdup(_config->_rootPool);

		if (idx == 3) {
			h.name = args[0].pdup(_config->_rootPool);
		}

		while (!r.empty()) {
			r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
			StringView params, n, v;
			if (r.is('"')) {
				++ r;
				params = r.readUntil<StringView::Chars<'"'>>();
				if (r.is('"')) {
					++ r;
				}
			} else {
				params = r.readUntil<StringView::CharGroup<CharGroupId::WhiteSpace>>();
			}

			if (!params.empty()) {
				n = params.readUntil<StringView::Chars<'='>>();
				++ params;
				v = params;

				if (!n.empty()) {
					if (v.empty()) {
						h.data.setBool(true, n);
					} else {
						h.data.setString(v, n);
					}
				}
			}
		}

		_config->_componentsToLoad.emplace_back(std::move(h));
	}
}

void Host::addAllow(StringView ips) {
	ips.split<StringView::CharGroup<CharGroupId::WhiteSpace>>([&] (StringView r) {
		_config->addAllowed(r);
	});

}

void Host::setSessionParams(StringView str) {
	StringView r(str);
	r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
	while (!r.empty()) {
		StringView params, n, v;
		if (r.is('"')) {
			++ r;
			params = r.readUntil<StringView::Chars<'"'>>();
			if (r.is('"')) {
				++ r;
			}
		} else {
			params = r.readUntil<StringView::CharGroup<CharGroupId::WhiteSpace>>();
		}

		if (!params.empty()) {
			n = params.readUntil<StringView::Chars<'='>>();
			++ params;
			v = params;

			if (!n.empty() && ! v.empty()) {
				_config->setSessionParam(n, v);
			}
		}

		r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
	}
}

void Host::setHostSecret(StringView w) {
	if (!w.empty()) {
		_config->setHostSecret(w);
	}
}

void Host::setWebHookParams(StringView str) {
	StringView r(str);
	r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
	while (!r.empty()) {
		StringView params, n, v;
		if (r.is('"')) {
			++ r;
			params = r.readUntil<StringView::Chars<'"'>>();
			if (r.is('"')) {
				++ r;
			}
		} else {
			params = r.readUntil<StringView::CharGroup<CharGroupId::WhiteSpace>>();
		}

		if (!params.empty()) {
			n = params.readUntil<StringView::Chars<'='>>();
			++ params;
			v = params;

			if (!n.empty() && ! v.empty()) {
				_config->setWebhookParam(n, v);
			}
		}

		r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
	}
}

void Host::setProtectedList(StringView str) {
	str.split<StringView::Chars<' '>>([&] (StringView &value) {
		addProtectedLocation(value);
	});
}

void Host::setDbParams(StringView w) {
	_config->setDbParams(w);
}

void Host::addProtectedLocation(const StringView &value) {
	_config->_protectedList.emplace(value.pdup(_config->_rootPool));
}

void Host::setForceHttps() {
	_config->setForceHttps();
}

pool_t *Host::getThreadPool() const {
	return _config->_rootPool;
}

const HostInfo &Host::getHostInfo() const {
	return _config->getHostInfo();
}

const SessionInfo &Host::getSessionInfo() const {
	return _config->getSessionInfo();
}

Root *Host::getRoot() const {
	return _config->getRoot();
}

pug::Cache *Host::getPugCache() const {
	return &_config->_pugCache;
}

db::sql::Driver *Host::getDbDriver() const {
	return _config->_dbDriver;
}

template <typename T>
auto Host_resolvePath(Map<StringView, T> &map, const StringView &path) -> typename Map<StringView, T>::iterator {
	auto it = map.begin();
	auto ret = map.end();
	for (; it != map.end(); it ++) {
		auto &p = it->first;
		if (p.size() - 1 <= path.size()) {
			if (p.back() == '/') {
				if (p.size() == 1 || (path.starts_with(StringView(p).sub(0, p.size() - 1))
						&& (path.size() == p.size() - 1 || path.at(p.size() - 1) == '/' ))) {
					if (ret == map.end() || ret->first.size() < p.size()) {
						ret = it;
					}
				}
			} else if (p == path) {
				ret = it;
				break;
			}
		}
	}
	return ret;
}

void Host::checkBroadcasts() {
	AsyncTask::perform(*this, [&] (AsyncTask &task) {
		task.addExecuteFn([this] (const AsyncTask &task) -> bool {
			task.performWithStorage([this] (const db::Transaction &t) {
				_config->_broadcastId = t.getAdapter().getBackendInterface()->processBroadcasts([&] (BytesView bytes) {
					handleBroadcast(bytes);
				}, _config->_broadcastId);
			});
			return true;
		});
	});
}

void Host::handleHeartBeat(pool_t *pool) {
	perform([&] {
		auto now = Time::now();
		if (!_config->_loadingFalled) {
			if (now - _config->_lastDatabaseCleanup > config::DEFAULT_DATABASE_CLEANUP_INTERVAL) {
				db::sql::Driver::Handle handle = _config->openConnection(pool, false);
				if (handle.get()) {
					_config->_dbDriver->performWithStorage(handle, [&] (const db::Adapter &a) {
						_config->_lastDatabaseCleanup = now;
						a.makeSessionsCleanup();
					});
					_config->closeConnection(handle);
				}
			}

			for (auto &it : _config->_components) {
				it.second->handleHeartbeat(*this);
			}
		}
		if (now - _config->_lastTemplateUpdate > config::DEFAULT_PUG_UPDATE_INTERVAL) {
			if (!_config->_pugCache.isNotifyAvailable()) {
				_config->_pugCache.update(pool);
			}
			_config->_lastTemplateUpdate = now;
		}
	}, pool, config::TAG_HOST, _config);
}

void Host::handleBroadcast(const Value &val) {
	if (val.getBool("system")) {
		_config->_root->handleBroadcast(val);
		return;
	}

	if (!val.hasValue("data")) {
		return;
	}

	/*if (val.getBool("message") && !val.getBool("exclusive")) {
		String url = String(config::getServerToolsPrefix()) + config::getServerToolsShell();
		auto it = Host_resolvePath(_config->_websockets, url);
		if (it != _config->_websockets.end() && it->second) {
			it->second->receiveBroadcast(val);
		}
	}

	auto &url = val.getString("url");
	if (!url.empty()) {
		auto it = Host_resolvePath(_config->_websockets, url);
		if (it != _config->_websockets.end() && it->second) {
			it->second->receiveBroadcast(val.getValue("data"));
		}
	}*/
}

void Host::handleBroadcast(const BytesView &bytes) {
	handleBroadcast(data::read<Interface>(bytes));
}

bool Host::isSecureAuthAllowed(const Request &rctx) const {
	auto userIp = rctx.getInfo().useragentIp;
	if (rctx.isSecureConnection() || strncmp(userIp.data(), "127.", 4) == 0 || userIp == "::1") {
		return true;
	}

	if (auto ip = valid::readIp(userIp)) {
		for (auto &it : _config->_allowedIps) {
			if (ip >= it.first && ip <= it.second) {
				return true;
			}
		}
	}

	return false;
}

static bool Host_processAuth(Request &rctx, StringView auth) {
	StringView r(auth);
	r.skipChars<StringView::WhiteSpace>();
	auto method = r.readUntil<StringView::WhiteSpace>().str<Interface>();
	string::apply_tolower_c(method);
	if (method == "basic" && rctx.config()->isSecureAuthAllowed()) {
		r.skipChars<StringView::WhiteSpace>();
		auto str = stappler::base64::decode<Interface>(r);
		StringView source((const char *)str.data(), str.size());
		StringView user = source.readUntil<StringView::Chars<':'>>();
		if (source.is(':')) {
			++ source;

			if (!user.empty() && !source.empty()) {
				if (rctx.performWithStorage([&] (const db::Transaction &t) {
					auto u = db::User::get(t, user, source);
					if (u) {
						rctx.setUser(u);
						if (u->isAdmin()) {
							auto &args = rctx.getInfo().queryData;
							if (args.hasValue("__FORCE_ROLE__")) {
								auto role = args.getInteger("__FORCE_ROLE__");
								rctx.setAccessRole(db::AccessRoleId(role));
							}
						}
						return true;
					}
					return false;
				})) {
					return true;
				}
			}
		}
	} else if (method == "pkey") {
		r.skipChars<StringView::WhiteSpace>();
		auto d = stappler::data::read<Interface>(stappler::base64::decode<Interface>(r));
		if (d.isArray() && d.size() == 2 && d.isBytes(0) && d.isBytes(1)) {
			auto &key = d.getBytes(0);
			auto &sig = d.getBytes(1);

			crypto::PublicKey pk;

			do {
				if (key.size() < 128 || sig.size() < 128) {
					break;
				}

				if (memcmp(key.data(), "ssh-", 4) == 0) {
					if (!pk.importOpenSSH(StringView((const char *)key.data(), key.size()))) {
						break;
					}
				} else {
					if (!pk.import(key)) {
						break;
					}
				}

				if (!pk.verify(key, sig, crypto::SignAlgorithm::RSA_SHA512)) {
					break;
				}

				bool complete = false;
				pk.exportDer([&] (BytesView data) {
					rctx.performWithStorage([&] (const db::Transaction &t) {
						if (auto u = db::User::get(t, *rctx.host().getUserScheme(), data)) {
							rctx.setUser(u);
							complete = true;
						}
						return false;
					});
				});
				if (complete) {
					return true;
				}
			} while (0);
		}
	}
	return false;
}

static Status Host_onRequestRecieved(Request &rctx, RequestHandler &h) {
	auto auth = rctx.getRequestHeader("Authorization");
	if (!auth.empty()) {
		Host_processAuth(rctx, auth);
	}

	auto origin = rctx.getRequestHeader("Origin");
	if (origin.empty()) {
		return OK;
	}

	if (rctx.getInfo().method != RequestMethod::Options) {
		// non-preflightted request
		if (h.isCorsPermitted(rctx, origin)) {
			rctx.setResponseHeader("Access-Control-Allow-Origin", origin);
			rctx.setResponseHeader("Access-Control-Allow-Credentials", "true");

			rctx.setErrorHeader("Access-Control-Allow-Origin", origin);
			rctx.setErrorHeader("Access-Control-Allow-Credentials", "true");
			return OK;
		} else {
			return HTTP_METHOD_NOT_ALLOWED;
		}
	} else {
		auto method = rctx.getRequestHeader("Access-Control-Request-Method");
		auto headers = rctx.getRequestHeader("Access-Control-Request-Headers");

		if (h.isCorsPermitted(rctx, origin, true, method, headers)) {
			rctx.setResponseHeader("Access-Control-Allow-Origin", origin);
			rctx.setResponseHeader("Access-Control-Allow-Credentials", "true");

			auto c_methods = h.getCorsAllowMethods(rctx);
			if (!c_methods.empty()) {
				rctx.setResponseHeader("Access-Control-Allow-Methods", c_methods);
			} else if (!method.empty()) {
				rctx.setResponseHeader("Access-Control-Allow-Methods", method);
			}

			auto c_headers = h.getCorsAllowHeaders(rctx);
			if (!c_headers.empty()) {
				rctx.setResponseHeader("Access-Control-Allow-Headers", c_headers);
			} else if (!headers.empty()) {
				rctx.setResponseHeader("Access-Control-Allow-Headers", headers);
			}

			auto c_maxAge = h.getCorsMaxAge(rctx);
			if (!c_maxAge.empty()) {
				rctx.setResponseHeader("Access-Control-Max-Age", c_maxAge);
			}

			return DONE;
		} else {
			return HTTP_METHOD_NOT_ALLOWED;
		}
	}
}

Status Host::handleRequest(Request &req) {
	if (_config->_forceHttps) {
		StringView uri(req.getInfo().url.path);
		if (uri.starts_with("/.well-known/acme-challenge/")) {
			auto path = filepath::merge<Interface>(_config->getHostInfo().documentRoot, uri);
			if (filesystem::exists(path)) {
				req.setFilename(path);
				return DONE;
			}
		}

		if (!req.isSecureConnection()) {
			auto p = req.getInfo().url.port;
			if (p.empty() || p == "80") {
				return req.redirectTo(toString("https://", req.getInfo().url.host, req.getInfo().unparserUri));
			} else if (p == "8080") {
				return req.redirectTo(toString("https://", req.getInfo().url.host, ":8443", req.getInfo().unparserUri));
			} else {
				return req.redirectTo(toString("https://", req.getInfo().url.host, ":", p, req.getInfo().unparserUri));
			}
		}
	}

	if (_config->_loadingFalled) {
		return HTTP_SERVICE_UNAVAILABLE;
	}

	auto path = req.getInfo().url.path;

	if (!_config->_protectedList.empty()) {
		StringView path_v(path);
		auto lb_it = _config->_protectedList.lower_bound(path);
		if (lb_it != _config->_protectedList.end() && path_v == *lb_it) {
			return HTTP_NOT_FOUND;
		} else {
			-- lb_it;
			StringView lb_v(*lb_it);
			if (path_v.is(lb_v)) {
				if (path_v.size() == lb_v.size() || lb_v.back() == '/' || (path_v.size() > lb_v.size() && path_v[lb_v.size()] == '/')) {
					return HTTP_NOT_FOUND;
				}
			}
		}
	}

	// Websocket handshake
	auto connection = req.getRequestHeader("connection").ptolower_c(req.pool());
	auto upgrade = req.getRequestHeader("upgrade").ptolower_c(req.pool());
	if (connection.find("upgrade") != String::npos && upgrade == "websocket") {
		// try websocket
		auto it = Host_resolvePath(_config->_websockets, path);
		if (it != _config->_websockets.end() && it->second) {
			auto auth = req.getRequestHeader("Authorization");
			if (!auth.empty()) {
				Host_processAuth(req, auth);
			}
			return it->second->accept(req);
		}
		return DECLINED;
	}

	for (auto &it : _config->_preRequest) {
		auto ret = it(req);
		if (ret == DONE || ret > 0) {
			return ret;
		}
	}

	auto ret = Host_resolvePath(_config->_requests, path);
	if (ret != _config->_requests.end() && (ret->second.callback || ret->second.map)) {
		StringView subPath((ret->first.back() == '/')?path.sub(ret->first.size() - 1):"");
		StringView originPath = subPath.size() == 0 ? StringView(path) : StringView(ret->first);
		if (originPath.back() == '/' && !subPath.empty()) {
			originPath = StringView(originPath).sub(0, originPath.size() - 1);
		}

		RequestHandler *h = nullptr;
		if (ret->second.map) {
			h = ret->second.map->onRequest(req, subPath);
		} else if (ret->second.callback) {
			h = ret->second.callback();
		}
		if (h) {
			auto role = h->getAccessRole();
			if (role != db::AccessRoleId::Nobody) {
				req.setAccessRole(role);
			}

			Status preflight = h->onRequestRecieved(req, move(originPath), move(subPath), ret->second.data);
			if (preflight > 0 || preflight == DONE) {
				req.getController()->startResponseTransmission();
				return preflight;
			}

			preflight = Host_onRequestRecieved(req, *h);
			if (preflight > 0 || preflight == DONE) {
				req.getController()->startResponseTransmission();
				return preflight;
			}
			req.setRequestHandler(h);
		}
	} else {
		if (path.size() > 1 && path.back() == '/') {
			auto name = path.sub(0, path.size() - 1);
			auto it = _config->_requests.find(name);
			if (it != _config->_requests.end()) {
				return req.redirectTo(name);
			}
		}
	}

	auto &data = req.getInfo().queryData;
	if (data.hasValue("basic_auth")) {
		if (req.getController()->isSecureAuthAllowed()) {
			if (req.getAuthorizedUser()) {
				return req.redirectTo(req.getInfo().url.url);
			}
			return HTTP_UNAUTHORIZED;
		}
	}

	return OK;
}

void Host::initTransaction(db::Transaction &t) {
	_config->initTransaction(t);
}

CompressionInfo *Host::getCompressionConfig() const {
	return &_config->_compression;
}

String Host::getDocumentRootPath(StringView sub) const {
	if (sub.empty()) {
		return _config->_hostInfo.documentRoot.str<Interface>();
	} else {
		return filepath::merge<Interface>(_config->_hostInfo.documentRoot, sub);
	}
}

HostComponent *Host::getHostComponent(const StringView &name) const {
	auto it = _config->_components.find(name);
	if (it != _config->_components.end()) {
		return it->second;
	}
	return nullptr;
}

HostComponent *Host::getHostComponent(std::type_index name) const {
	auto it = _config->_typedComponents.find(name);
	if (it != _config->_typedComponents.end()) {
		return it->second;
	}
	return nullptr;
}

void Host::addComponentWithName(const StringView &name, HostComponent *comp) {
	_config->_components.emplace(name, comp);
	_config->_typedComponents.emplace(std::type_index(typeid(*comp)), comp);
	if (_config->_childInit) {
		comp->handleChildInit(*this);
	}
}

const Map<StringView, HostComponent *> &Host::getComponents() const {
	return _config->_components;
}

void Host::addPreRequest(Function<Status(Request &)> &&req) const {
	_config->_preRequest.emplace_back(std::move(req));
}

void Host::addHandler(StringView path, const HandlerCallback &cb, const Value &d) const {
	if (!path.empty() && path.front() == '/') {
		_config->_requests.emplace(path.pdup(_config->_rootPool),
				RequestSchemeInfo{_config->_currentComponent, cb, d});
	}
}
void Host::addResourceHandler(StringView path, const db::Scheme &scheme) const {
	path = path.pdup(_config->_rootPool);
	if (!path.empty() && path.front() == '/') {
		_config->_requests.emplace(path,
				RequestSchemeInfo{_config->_currentComponent,
				[s = &scheme] () -> RequestHandler * {
			return new ResourceHandler(*s, Value());
		}, Value(), &scheme});
	}
	auto it = _config->_resources.find(&scheme);
	if (it == _config->_resources.end()) {
		_config->_resources.emplace(&scheme, ResourceSchemeInfo{path, Value()});
	}
}

void Host::addResourceHandler(StringView path, const db::Scheme &scheme, const Value &val) const {
	path = path.pdup(_config->_rootPool);
	if (!path.empty() && path.front() == '/') {
		_config->_requests.emplace(path,
				RequestSchemeInfo{_config->_currentComponent,
				[s = &scheme, val] () -> RequestHandler * {
			return new ResourceHandler(*s, val);
		}, Value(), &scheme});
	}
	auto it = _config->_resources.find(&scheme);
	if (it == _config->_resources.end()) {
		_config->_resources.emplace(&scheme, ResourceSchemeInfo{path, val});
	}
}

void Host::addMultiResourceHandler(StringView path, std::initializer_list<Pair<const StringView, const db::Scheme *>> &&schemes) const {
	if (!path.empty() && path.front() == '/') {
		path = path.pdup(_config->_rootPool);
		_config->_requests.emplace(path,
				RequestSchemeInfo{_config->_currentComponent,
				[s = Map<StringView, const db::Scheme *>(move(schemes))] () -> RequestHandler * {
			return new ResourceMultiHandler(s);
		}, Value()});
	}
}

void Host::addHandler(std::initializer_list<StringView> paths, const HandlerCallback &cb, const Value &d) const {
	for (auto &it : paths) {
		if (!it.empty() && it.front() == '/') {
			_config->_requests.emplace(it.pdup(_config->_rootPool),
					RequestSchemeInfo{_config->_currentComponent, cb, d});
		}
	}
}

void Host::addHandler(StringView path, const RequestHandlerMap *map) const {
	if (!path.empty() && path.front() == '/') {
		path = path.pdup(_config->_rootPool);
		_config->_requests.emplace(path,
				RequestSchemeInfo{_config->_currentComponent, nullptr, Value(), nullptr, map});
	}
}

void Host::addHandler(std::initializer_list<StringView> paths, const RequestHandlerMap *map) const {
	for (auto &it : paths) {
		if (!it.empty() && it.front() == '/') {
			_config->_requests.emplace(it.pdup(_config->_rootPool),
					RequestSchemeInfo{_config->_currentComponent, nullptr, Value(), nullptr, map});
		}
	}
}

void Host::addWebsocket(StringView str, WebsocketManager *m) const {
	_config->_websockets.emplace(str.pdup(_config->_rootPool), m);
}

const db::Scheme * Host::exportScheme(const db::Scheme &scheme) const {
	_config->_schemes.emplace(scheme.getName(), &scheme);
	return &scheme;
}

const db::Scheme * Host::getScheme(const StringView &name) const {
	auto it = _config->_schemes.find(name);
	if (it != _config->_schemes.end()) {
		return it->second;
	}
	return nullptr;
}

const db::Scheme * Host::getFileScheme() const {
	return getScheme(config::FILE_SCHEME_NAME);
}

const db::Scheme * Host::getUserScheme() const {
	return getScheme(config::USER_SCHEME_NAME);
}

const db::Scheme * Host::getErrorScheme() const {
	return getScheme(config::ERROR_SCHEME_NAME);
}

db::Scheme * Host::getMutable(const db::Scheme *s) const {
	if (!_config->_childInit) {
		return const_cast<db::Scheme *>(s);
	}
	return nullptr;
}

StringView Host::getResourcePath(const db::Scheme &scheme) const {
	auto it = _config->_resources.find(&scheme);
	if (it != _config->_resources.end()) {
		return it->second.path;
	}
	return String();
}

const Map<StringView, const db::Scheme *> &Host::getSchemes() const {
	return _config->_schemes;
}
const Map<const db::Scheme *, ResourceSchemeInfo> &Host::getResources() const {
	return _config->_resources;
}

const Map<StringView, RequestSchemeInfo> &Host::getRequestHandlers() const {
	return _config->_requests;
}

struct Host_ErrorList {
	RequestController *request;
	Vector<Value> errors;
};

struct Host_ErrorReporterFlags {
	bool isProtected = false;
};

void Host::reportError(const Value &d) {
	if (auto obj = pool::get<Host_ErrorReporterFlags>("Host_ErrorReporterFlags")) {
		if (obj->isProtected) {
			return;
		}
	}
	if (auto req = Request::getCurrent()) {
		perform([&] {
			if (auto objVal = Request(req).getObject<Host_ErrorList>("Host_ErrorList")) {
				objVal->errors.emplace_back(d);
			} else {
				auto obj = new Host_ErrorList{req.config()};
				obj->errors.emplace_back(d);
				Request rctx(req);
				rctx.storeObject(obj, "Host_ErrorList", [obj] {
					Host(obj->request->getHost()).runErrorReportTask(Request(obj->request), obj->errors);
				});
			}
		}, req.pool());
	} else {
		runErrorReportTask(Request(nullptr), Vector<Value>{d});
	}
}

bool Host::performTask(AsyncTask *task, bool performFirst) const {
	return _config->_root->performTask(*this, task, performFirst);
}

bool Host::scheduleTask(AsyncTask *task, TimeInterval t) const {
	return _config->_root->scheduleTask(*this, task, t);
}

void Host::runErrorReportTask(const Request &req, const Vector<Value> &errors) {
	if (errors.empty()) {
		return;
	}
	if (!_config->_childInit) {
		for (auto &it : errors) {
			std::cout << "[Error]: " << data::EncodeFormat::Pretty << it << "\n";
		}
		return;
	}

	if (_config->_loadingFalled) {
		return;
	}

	AsyncTask::perform(Host(*this), [&, c = req.getController()] (AsyncTask &task) {
		Value *err = nullptr;
		if (c) {
			err = new Value {
				pair("documentRoot", Value(getHostInfo().documentRoot)),
				pair("name", Value(getHostInfo().hostname)),
				pair("url", Value(toString(req.getInfo().url.host, req.getInfo().unparserUri))),
				pair("request", Value(req.getInfo().requestLine)),
				pair("ip", Value(req.getInfo().useragentIp)),
				pair("time", Value(Time::now().toMicros()))
			};

			c->foreachRequestHeaders([&] (StringView key, StringView value) {
				err->emplace("headers").setString(value, key);
			});
		} else {
			err = new Value {
				pair("documentRoot", Value(getHostInfo().documentRoot)),
				pair("name", Value(getHostInfo().hostname)),
				pair("time", Value(Time::now().toMicros()))
			};
		}
		auto &d = err->emplace("data");
		for (auto &it : errors) {
			d.addValue(it);
		}
		task.addExecuteFn([err, driver = _config->_dbDriver] (const AsyncTask &task) -> bool {
			Host_ErrorReporterFlags *obj = pool::get<Host_ErrorReporterFlags>("Host_ErrorReporterFlags");
			if (obj) {
				obj->isProtected = true;
			} else {
				obj = new Host_ErrorReporterFlags{true};
				pool::store(obj, "Host_ErrorReporterFlags");
			}

			if (task.getHost()._config->_loadingFalled) {
				return false;
			}

			task.performWithStorage([&] (const db::Transaction &t) {
				t.performAsSystem([&] () -> bool {
					if (auto errScheme = task.getHost().getErrorScheme()) {
						if (errScheme->create(t, *err)) {
							return true;
						}
					}
					std::cout << "Fail to report error: " << *err << "\n";
					return false;
				});
			});
			return true;
		});
	});
}

}
