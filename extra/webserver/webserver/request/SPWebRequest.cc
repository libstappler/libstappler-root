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

#include "SPWebRequest.h"

#include "SPDbAdapter.h"
#include "SPWebRequestController.h"
#include "SPWebOutput.h"
#include "SPWebSession.h"
#include "SPWebRoot.h"
#include "SPDbUser.h"

namespace STAPPLER_VERSIONIZED stappler::web {

static RequestController *getRequestFromContext(pool_t *p, uint32_t tag, const void *ptr) {
	switch (tag) {
	case uint32_t(config::TAG_REQUEST): return (RequestController *)ptr; break;
	}
	return nullptr;
}

Request Request::getCurrent() {
	RequestController *ret = nullptr;
	pool::foreach_info(&ret, [] (void *ud, pool_t *p, uint32_t tag, const void *data) -> bool {
		auto ptr = getRequestFromContext(p, tag, data);
		if (ptr) {
			*((RequestController **)ud) = ptr;
			return false;
		}
		return true;
	});

	return Request(ret);
}

Request::Request() : basic_ostream(&_buffer), _buffer(nullptr), _config(nullptr) { }

Request::Request(RequestController *cfg) : basic_ostream(&_buffer), _buffer(cfg), _config(cfg) {
	this->init(&_buffer);
}

Request & Request::operator =(RequestController *cfg) {
	_buffer = Buffer(cfg);
	_config = cfg;
	this->init(&_buffer);
	return *this;
}

Request::Request(Request &&other) : basic_ostream(&_buffer), _buffer(other._config), _config(other._config) {
	this->init(&_buffer);
}
Request & Request::operator =(Request &&other) {
	_buffer = Buffer(other._config);
	_config = other._config;
	this->init(&_buffer);
	return *this;
}

Request::Request(const Request &other) : basic_ostream(&_buffer), _buffer(other._config), _config(other._config) {
	this->init(&_buffer);
}

Request & Request::operator =(const Request &other) {
	_buffer = Buffer(other._config);
	_config = other._config;
	this->init(&_buffer);
	return *this;
}

const RequestInfo &Request::getInfo() const {
	return _config->getInfo();
}

StringView Request::getRequestHeader(StringView key) const {
	return _config->getRequestHeader(key);
}

void Request::foreachRequestHeaders(const Callback<void(StringView, StringView)> &cb) const {
	_config->foreachRequestHeaders(cb);
}

StringView Request::getResponseHeader(StringView key) const {
	return _config->getResponseHeader(key);
}

void Request::foreachResponseHeaders(const Callback<void(StringView, StringView)> &cb) const {
	_config->foreachResponseHeaders(cb);
}

void Request::setResponseHeader(StringView key, StringView value) const {
	_config->setResponseHeader(key, value);
}

void Request::clearResponseHeaders() const {
	_config->clearResponseHeaders();
}

StringView Request::getErrorHeader(StringView key) const {
	return _config->getErrorHeader(key);
}

void Request::foreachErrorHeaders(const Callback<void(StringView, StringView)> &cb) const {
	_config->foreachErrorHeaders(cb);
}

void Request::setErrorHeader(StringView key, StringView value) const {
	_config->setErrorHeader(key, value);
}

void Request::clearErrorHeaders() const {
	_config->clearErrorHeaders();
}

Request::Buffer::Buffer(RequestController *cfg) : _config(cfg) { }
Request::Buffer::Buffer(Buffer &&other) : _config(other._config) { }
Request::Buffer& Request::Buffer::operator=(Buffer &&other) { _config = other._config; return *this; }

Request::Buffer::Buffer(const Buffer &other) : _config(other._config) { }
Request::Buffer& Request::Buffer::operator=(const Buffer &other) { _config = other._config; return *this; }

Request::Buffer::int_type Request::Buffer::overflow(int_type c) {
	_config->putc(c);
	return c;
}

Request::Buffer::pos_type Request::Buffer::seekoff(off_type off, ios_base::seekdir way, ios_base::openmode) {
	return _config->getBytesSent();
}

Request::Buffer::pos_type Request::Buffer::seekpos(pos_type pos, ios_base::openmode mode) {
	return _config->getBytesSent();
}

int Request::Buffer::sync() {
	_config->flush();
	return 0;
}

Request::Buffer::streamsize Request::Buffer::xsputn(const char_type* s, streamsize n) {
	return _config->write((const uint8_t *)s, n);
}

void Request::setRequestHandler(RequestHandler *h) {
	_config->_handler = h;
}
RequestHandler *Request::getRequestHandler() const {
	return _config->_handler;
}

void Request::writeData(const Value &data, bool allowJsonP) {
	output::writeData(*this, data, allowJsonP);
}

/* request params setters */
void Request::setDocumentRoot(StringView str) {
	_config->setDocumentRoot(str);
}

void Request::setContentType(StringView str) {
	_config->setContentType(str);
}

void Request::setHandler(StringView str) {
	_config->setHandler(str);
}

void Request::setContentEncoding(StringView str) {
	_config->setContentEncoding(str);
}

void Request::setFilename(StringView str, bool updateStat, Time mtime) {
	_config->setFilename(str, updateStat, mtime);
}

void Request::setCookie(StringView name, StringView value, TimeInterval maxAge, CookieFlags flags) {
	_config->_cookies.emplace(name.pdup(pool()), CookieStorageInfo{value.str<Interface>(), flags, maxAge});
}

void Request::removeCookie(StringView name, CookieFlags flags) {
	_config->_cookies.emplace(name.pdup(pool()), CookieStorageInfo{String(), flags, TimeInterval::seconds(0)});
}

const Map<StringView, CookieStorageInfo> Request::getResponseCookies() const {
	return _config->_cookies;
}

StringView Request::getCookie(StringView name, bool removeFromHeadersTable) const {
	return _config->getCookie(name, removeFromHeadersTable);
}

Session *Request::authorizeUser(db::User *user, TimeInterval maxAge) {
	if (_config->_session) {
		_config->_session->cancel();
	}
	auto s = new Session(*this, user, maxAge);
	if (s->isValid()) {
		_config->_session = s;
		_config->_user = user;
		return s;
	}
	return nullptr;
}

void Request::setInputFilter(InputFilter *filter) {
	_config->setInputFilter(filter);
}

InputFilter *Request::getInputFilter() const {
	return _config->_filter;
}

void Request::setUser(db::User *u) {
	if (u) {
		_config->_user = u;
		auto newRole = _config->_accessRole;
		if (_config->_user->isAdmin()) {
			newRole = std::max(db::AccessRoleId::Admin, _config->_accessRole);
		} else {
			newRole = std::max(db::AccessRoleId::Authorized, _config->_accessRole);
		}
		if (newRole != _config->_accessRole) {
			setAccessRole(newRole);
		}
		_config->_userId = u->getObjectId();
	}
}

void Request::setUser(int64_t id) {
	_config->_userId = id;
}

Session *Request::getSession() {
	if (!_config->_session) {
		_config->_session = new Session(*this);
	}

	if (_config->_session->isValid()) {
		return _config->_session;
	}
	return nullptr;
}

db::User *Request::getUser() {
	if (!_config->_user) {
		if (auto s = getSession()) {
			_config->_user = s->getUser();
			if (_config->_user && _config->_user->isAdmin()) {
				setAccessRole(db::AccessRoleId::Admin);
			}
		}
	}
	return _config->_user;
}

db::User *Request::getAuthorizedUser() const {
	return _config->_user;
}

int64_t Request::getUserId() const {
	return _config->_userId;
}

void Request::setStatus(Status status, StringView str) {
	_config->setStatus(status, str);
}

const db::InputConfig & Request::getInputConfig() const {
	return _config->_inputConfig;
}

void Request::setInputConfig(const db::InputConfig &cfg) {
	_config->_inputConfig = cfg;
}

void Request::storeObject(void *ptr, const StringView &key, Function<void()> &&cb) const {
	pool::store(pool(), ptr, key, sp::move(cb));
}

bool Request::performWithStorage(const Callback<bool(const db::Transaction &)> &cb) const {
	auto ad = _config->acquireDatabase();
	return ad.performWithTransaction([&, this] (const db::Transaction &t) {
		t.setRole(_config->_accessRole);
		return cb(t);
	});
}

bool Request::isSecureConnection() const {
	return _config->isSecureConnection();
}

RequestController *Request::config() const {
	return _config;
}

Host Request::host() const {
	return Host(_config->getHost());
}

pool_t *Request::pool() const {
	return _config->getPool();
}

const Vector<Value> & Request::getDebugMessages() const {
	return _config->_debug;
}

const Vector<Value> & Request::getErrorMessages() const {
	return _config->_errors;
}

void Request::addErrorMessage(Value &&val) const {
	if (_config) {
		_config->_host->getRoot()->pushErrorMessage(sp::move(val));
	}
}

void Request::addDebugMessage(Value &&val) const {
	if (_config) {
		_config->_host->getRoot()->pushDebugMessage(sp::move(val));
	}
}

void Request::addCleanup(Function<void()> &&cb) const {
	pool::cleanup_register(pool(), sp::move(cb));
}

bool Request::isAdministrative() {
	return getAccessRole() == db::AccessRoleId::Admin;
}

db::AccessRoleId Request::getAccessRole() const {
	if (_config->_accessRole == db::AccessRoleId::Nobody) {
		if (_config->_user) {
			if (_config->_user->isAdmin()) {
				_config->_accessRole = db::AccessRoleId::Admin;
			} else {
				_config->_accessRole = db::AccessRoleId::Authorized;
			}
		} else {
			Session s(*this, true);
			if (s.isValid()) {
				auto u = s.getUser();
				if (u) {
					if (u->isAdmin()) {
						_config->_accessRole = db::AccessRoleId::Admin;
					} else {
						_config->_accessRole = db::AccessRoleId::Authorized;
					}
				}
			}
#ifdef DEBUG
			auto userIp = _config->_info.useragentIp;
			if ((strncmp(userIp.data(), "127.", 4) == 0 || userIp == "::1") && _config->_info.queryData.getBool("admin")) {
				_config->_accessRole = db::AccessRoleId::Admin;
			}
#endif
		}
		if (auto t = db::Transaction::acquireIfExists(pool())) {
			auto role = t.getRole();
			if (role != db::AccessRoleId::System && toInt(t.getRole()) > toInt(_config->_accessRole)) {
				_config->_accessRole = role;
			}
		}
	}
	return _config->_accessRole;
}

void Request::setAccessRole(db::AccessRoleId role) const {
	_config->_accessRole = role;
	if (auto t = db::Transaction::acquireIfExists(pool())) {
		t.setRole(role);
	}
}

db::Transaction Request::acquireDbTransaction() const {
	return db::Transaction::acquire(_config->acquireDatabase());
}

Status Request::redirectTo(StringView location) {
	setResponseHeader("Location", location);
	return HTTP_SEE_OTHER;
}

Status Request::sendFile(StringView file, size_t cacheTime) {
	setFilename(filesystem::writablePath<Interface>(file), true);
	if (cacheTime == 0) {
		setResponseHeader("Cache-Control", "no-cache, must-revalidate");
	} else if (cacheTime < SIZE_MAX) {
		setResponseHeader("Cache-Control", toString("max-age=", cacheTime, ", must-revalidate", cacheTime));
	}
	return OK;
}

Status Request::sendFile(StringView file, StringView contentType, size_t cacheTime) {
	if (!contentType.empty()) {
		setContentType(sp::move(contentType));
	}
	return sendFile(sp::move(file), cacheTime);
}

String Request::getFullHostname(int port) const {
	auto secure = _config->isSecureConnection();
	auto &info = _config->getInfo();
	if (port == -1) {
		if (!info.url.port.empty()) {
			port = StringView(_config->getInfo().url.port).readInteger(10).get(secure ? 443 : 80);
		} else {
			port = secure ? 443 : 80;
		}
	}

	StringStream ret;
	ret << (secure?"https":"http") << "://" << info.url.host;
	if (port && ((secure && port != 443) || (!secure && port != 80))) {
		ret << ':' << port;
	}

	return ret.str();
}

bool Request::checkCacheHeaders(Time t, const StringView &etag) {
	return output::checkCacheHeaders(*this, t, etag);
}

bool Request::checkCacheHeaders(Time t, uint32_t idHash) {
	return output::checkCacheHeaders(*this, t, idHash);
}

Status Request::runPug(const StringView & path, const Function<bool(pug::Context &, const pug::Template &)> &cb) {
	auto cache = host().getPugCache();
	if (cache->runTemplate(path, [&, this] (pug::Context &ctx, const pug::Template &tpl) -> bool {
		initScriptContext(ctx);

		if (cb(ctx, tpl)) {
			auto lm = getResponseHeader("Last-Modified");
			auto etag = getResponseHeader("ETag");
			setResponseHeader("Content-Type", "text/html; charset=UTF-8");
			if (lm.empty() && etag.empty()) {
				setResponseHeader("Cache-Control", "no-cache, no-store, must-revalidate");
				setResponseHeader("Pragma", "no-cache");
				setResponseHeader("Expires", Time::seconds(0).toHttp<Interface>());
			}
			return true;
		}
		return false;
	}, [&] (StringView str) { *this << str; })) {
		return DONE;
	}
	return HTTP_INTERNAL_SERVER_ERROR;
}

void Request::initScriptContext(pug::Context &ctx) {
	auto &info = getInfo();
	pug::VarClass serenityClass;
	serenityClass.staticFunctions.emplace("prettify", [] (pug::VarStorage &, pug::Var *var, size_t argc) -> pug::Var {
		if (var && argc == 1) {
			return pug::Var(Value(data::toString(var->readValue(), true)));
		}
		return pug::Var();
	});
	serenityClass.staticFunctions.emplace("timeToHttp", [] (pug::VarStorage &, pug::Var *var, size_t argc) -> pug::Var {
		if (var && argc == 1 && var->readValue().isInteger()) {
			return pug::Var(Value(Time::microseconds(var->readValue().asInteger()).toHttp<Interface>()));
		}
		return pug::Var();
	});
	serenityClass.staticFunctions.emplace("uuidToString", [] (pug::VarStorage &, pug::Var *var, size_t argc) -> pug::Var {
		if (var && argc == 1 && var->readValue().isBytes()) {
			return pug::Var(Value(memory::uuid(var->readValue().getBytes()).str()));
		}
		return pug::Var();
	});
	ctx.set("serenity", sp::move(serenityClass));
	ctx.set("window", Value{
		pair("location", Value({
			pair("href", Value(toString(getFullHostname(), info.unparserUri))),
			pair("hostname", Value(info.url.host)),
			pair("pathname", Value(info.url.path)),
			pair("protocol", Value(_config->isSecureConnection() ? "https:" : "http:")),
		}))
	});
}

}
