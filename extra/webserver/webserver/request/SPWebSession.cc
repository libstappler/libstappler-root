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

#include "SPWebSession.h"
#include "SPValid.h"
#include "SPCrypto.h"
#include "SPDbUser.h"

namespace STAPPLER_VERSIONIZED stappler::web {

#define SA_SESSION_EXISTS(var, def) ((var == NULL)?def:var)

static constexpr auto SA_SESSION_TOKEN_NAME = "token";
static constexpr auto SA_SESSION_UUID_KEY = "uuid";
static constexpr auto SA_SESSION_USER_NAME_KEY = "userName";
static constexpr auto SA_SESSION_USER_ID_KEY = "userId";
static constexpr auto SA_SESSION_SALT_KEY = "salt";
static constexpr auto SA_SESSION_MAX_AGE_KEY = "maxAge";
static constexpr auto SA_SESSION_TOKEN_LEN = 64;

Session::Token Session::makeSessionToken(Request &rctx, const memory::uuid & uuid, const StringView & userName) {
	auto host = rctx.host();
	auto &info = rctx.getInfo();

	return crypto::hash512([&] ( const Callback<bool(const CoderSource &)> &upd ) {
		upd(uuid.view());
		upd(userName);
		upd(host.getSessionInfo().key);
		upd(info.url.host);
		upd(info.protocol);
		upd(rctx.getRequestHeader("User-Agent"));
	}, crypto::HashFunction::GOST_3411);
}

Session::Token Session::makeCookieToken(Request &rctx, const memory::uuid & uuid, const StringView & userName, const Bytes & salt) {
	auto host = rctx.host();
	auto &info = rctx.getInfo();

	return crypto::hash512([&] ( const Callback<bool(const CoderSource &)> &upd ) {
		upd(uuid.view());
		upd(userName);
		upd(host.getSessionInfo().key);
		upd(info.url.host);
		upd(info.protocol);
		upd(salt);
		upd(rctx.getRequestHeader("User-Agent"));
	}, crypto::HashFunction::GOST_3411);
}

Session::~Session() {
	if (isModified() && isValid() && _request) {
		save();
	}
}

Session::Session(const Request &rctx, bool silent) : _request(rctx) {
	_valid = init(silent);
}

Session::Session(const Request &rctx, db::User *user, TimeInterval maxAge) : _request(rctx) {
	_valid = init(user, maxAge);
}

bool Session::init(db::User *user, TimeInterval maxAge) {
	_maxAge = maxAge;
	_uuid = memory::uuid::generate();
	_user = user;

	auto &data = newDict("data");
	data.setString(user ? user->getName() : memory::uuid::generate().str(), SA_SESSION_USER_NAME_KEY);
	data.setInteger(user ? user->getObjectId() : 0, SA_SESSION_USER_ID_KEY);
	data.setInteger(maxAge.toMicros(), SA_SESSION_MAX_AGE_KEY);
	data.setBytes(_uuid.bytes(), SA_SESSION_UUID_KEY);

	Bytes salt; salt.resize(32);
	stappler::valid::makeRandomBytes(salt.data(), 32);

	_sessionToken = makeSessionToken(_request, _uuid, data.getString(SA_SESSION_USER_NAME_KEY));
	_cookieToken = makeCookieToken(_request, _uuid, user->getName(), salt);

	data.setBytes(std::move(salt), SA_SESSION_SALT_KEY);

	setModified(false);

	return write();
}

bool Session::init(bool silent) {
	auto host = _request.host();
	auto &info = _request.getInfo();

	auto &sessionTokenString = info.queryData.getString(SA_SESSION_TOKEN_NAME);

	/* token is a base64 encoded hash from sha512, so, it must have 88 bytes */
	if (sessionTokenString.size() != 86) {
		if (!silent) {
			_request.addDebug("Session", "Session token format is invalid");
		}
		return false;
	}

	Bytes sessionToken(stappler::base64url::decode<Interface>(sessionTokenString));
	auto sessionData = getStorageData(_request, sessionToken);
	auto &data = sessionData.getValue("data");
	if (!data && !silent) {
		_request.addDebug("Session", "Fail to extract session from storage");
	}

	auto &uuidData = data.getBytes(SA_SESSION_UUID_KEY);
	auto &userName = data.getString(SA_SESSION_USER_NAME_KEY);
	auto &salt = data.getBytes(SA_SESSION_SALT_KEY);
	if (uuidData.empty() || userName.empty()) {
		if (!silent) {
			_request.addError("Session", "Wrong authority data in session");
		}
		return false;
	}

	memory::uuid sessionUuid(uuidData);

	Token buf = makeSessionToken(_request, sessionUuid, userName);

	if (memcmp(buf.data(), sessionToken.data(), sizeof(Token)) != 0) {
		if (!silent) {
			_request.addError("Session", "Session token is invalid");
		}
		return false;
	}

	Bytes cookieToken(stappler::base64url::decode<Interface>(_request.getCookie(host.getSessionInfo().name, !silent)));
	if (cookieToken.empty() || cookieToken.size() != 64) {
		if (!silent) {
			_request.addError("Session", "Fail to read token from cookie", Value{
				std::make_pair("token", Value(cookieToken))
			});
		}
		return false;
	}

	buf = makeCookieToken(_request, sessionUuid, userName, salt);

	if (memcmp(buf.data(), cookieToken.data(), sizeof(Token)) != 0) {
		if (!silent) {
			_request.addError("Session", "Cookie token is invalid", Value{
				std::make_pair("token", Value(cookieToken)),
				std::make_pair("check", Value(Bytes(buf.begin(), buf.end())))
			});
		}
		return false;
	}

	memcpy(_cookieToken.data(), cookieToken.data(), SA_SESSION_TOKEN_LEN);
	memcpy(_sessionToken.data(), sessionToken.data(), SA_SESSION_TOKEN_LEN);
	_uuid = sessionUuid;
	_maxAge = TimeInterval::seconds(data.getInteger(SA_SESSION_MAX_AGE_KEY));

	uint64_t id = (uint64_t)data.getInteger(SA_SESSION_USER_ID_KEY);
	if (id) {
		_user = getStorageUser(_request, id);
		if (!_user) {
			if (!silent) {
				_request.addError("Session", "Invalid user id in session data");
			}
			return false;
		}
	}

	_data = std::move(sessionData);
	return _user != nullptr;
}

const Session::Token & Session::getCookieToken() const {
	return _cookieToken;
}

bool Session::isValid() const {
	return _valid;
}
const Session::Token & Session::getSessionToken() const {
	return _sessionToken;
}

const memory::uuid &Session::getSessionUuid() const {
	return _uuid;
}

bool Session::write() {
	if (!save()) {
		return false;
	}

	_request.setCookie(_request.host().getSessionInfo().name, stappler::base64url::encode<Interface>(_cookieToken), _maxAge);
	return true;
}

bool Session::save() {
	setModified(false);
	return setStorageData(_request, _sessionToken, _data, _maxAge);
}

bool Session::cancel() {
	clearStorageData(_request, _sessionToken);
	_request.removeCookie(_request.host().getSessionInfo().name);
	_valid = false;
	return true;
}

bool Session::touch(TimeInterval maxAge) {
	if (maxAge) {
		_maxAge = maxAge;
		setInteger(maxAge.toSeconds(), SA_SESSION_MAX_AGE_KEY);
	}

	return write();
}

db::User *Session::getUser() const {
	return _user;
}

TimeInterval Session::getMaxAge() const {
	return _maxAge;
}


Value Session::getStorageData(Request &rctx, const Token &key) {
	Value ret;
	rctx.performWithStorage([&] (const db::Transaction &t) {
		ret = t.getAdapter().get(key);
		return true;
	});
	return ret;
}

Value Session::getStorageData(Request &rctx, const Bytes &key) {
	Value ret;
	rctx.performWithStorage([&] (const db::Transaction &t) {
		ret = t.getAdapter().get(key);
		return true;
	});
	return ret;
}

bool Session::setStorageData(Request &rctx, const Token &key, const Value &d, TimeInterval maxAge) {
	bool ret = false;
	rctx.performWithStorage([&] (const db::Transaction &t) {
		ret = t.getAdapter().set(key, d, maxAge);
		return true;
	});
	return ret;
}

bool Session::clearStorageData(Request &rctx, const Token &key) {
	bool ret = false;
	rctx.performWithStorage([&] (const db::Transaction &t) {
		ret = t.getAdapter().clear(key);
		return true;
	});
	return ret;
}

db::User *Session::getStorageUser(Request &rctx, uint64_t oid) {
	db::User *ret = nullptr;
	rctx.performWithStorage([&] (const db::Transaction &t) {
		ret = db::User::get(t, oid);
		return true;
	});
	return ret;
}

}
