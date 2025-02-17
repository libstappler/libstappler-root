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

#ifndef EXTRA_WEBSERVER_WEBSERVER_REQUEST_SPWEBSESSION_H_
#define EXTRA_WEBSERVER_WEBSERVER_REQUEST_SPWEBSESSION_H_

#include "SPWebRequest.h"
#include "SPDataWrapper.h"

namespace STAPPLER_VERSIONIZED stappler::web {

class SP_PUBLIC Session : public data::WrapperTemplate<Interface> {
public:
	using Token = stappler::string::Sha512::Buf;

	static bool hasCredentials(const Request &);

	Session(const Request &, bool silent = false);
	Session(const Request &, db::User *user, TimeInterval maxAge);
	~Session();

	bool init(db::User *user, TimeInterval maxAge);
	bool init(bool silent);

	bool isValid() const;
	const Token &getSessionToken() const;
	const Token &getCookieToken() const;
	const memory::uuid &getSessionUuid() const;

	db::User *getUser() const;
	TimeInterval getMaxAge() const;

	bool write();
	bool save();
	bool cancel();
	bool touch(TimeInterval maxAge = TimeInterval());

protected:
	static Value getStorageData(Request &, const Token &);
	static Value getStorageData(Request &, const Bytes &);
	static bool setStorageData(Request &, const Token &, const Value &, TimeInterval maxAge);
	static bool clearStorageData(Request &, const Token &);
	static db::User *getStorageUser(Request &, uint64_t);

	static Token makeSessionToken(Request &rctx, const memory::uuid & uuid, const StringView & userName);
	static Token makeCookieToken(Request &rctx, const memory::uuid & uuid, const StringView & userName, const Bytes & salt);

	Request _request;

	Token _sessionToken;
	Token _cookieToken;

	memory::uuid _uuid;
	TimeInterval _maxAge;

	db::User *_user = nullptr;
	bool _valid = false;
};

}

#endif /* EXTRA_WEBSERVER_WEBSERVER_REQUEST_SPWEBSESSION_H_ */
