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

#include "SPWebTools.h"
#include "SPWebRoot.h"
#include "SPWebSession.h"
#include "SPDbUser.h"

namespace STAPPLER_VERSIONIZED stappler::web::tools {

bool AuthHandler::isRequestPermitted(Request &rctx) {
	if (_subPath != "/login" && _subPath != "/update" && _subPath != "/cancel" && _subPath != "/setup" && _subPath != "/basic" && _subPath != "/touch") {
		rctx.setStatus(HTTP_NOT_FOUND);
		return false;
	}
	if (_subPath != "/login" && _subPath != "/setup" && _subPath != "/basic" && rctx.getUser() == nullptr) {
		Root::getCurrent()->error("Auth", "You are not logged in");
		return false;
	}
	_allow = AllowMethod::Get;
	_transaction = rctx.acquireDbTransaction();
	return rctx.getInfo().method == RequestMethod::Get;
}

Status AuthHandler::onTranslateName(Request &rctx) {
	if (_subPath == "/basic") {
		if (rctx.getAuthorizedUser()) {
			auto r = rctx.getInfo().queryData.getString("redirect");
			return rctx.redirectTo(String(r));
		}
		return HTTP_UNAUTHORIZED;
	}
	return DataHandler::onTranslateName(rctx);
}

bool AuthHandler::processDataHandler(Request &rctx, Value &result, Value &input) {
	auto &queryData = rctx.getInfo().queryData;

	if (!_transaction) {
		Root::getCurrent()->error("ResourceHandler", "Database connection failed");
		rctx.setStatus(HTTP_INTERNAL_SERVER_ERROR);
		return false;
	}

	if (_subPath == "/login") {
		auto &name = queryData.getString("name");
		auto &passwd = queryData.getString("passwd");
		if (name.empty() || passwd.empty()) {
			Root::getCurrent()->error("Auth", "Name or password is not specified", Value{
				std::make_pair("Doc", Value("You should specify 'name' and 'passwd' variables in request"))
			});
			return false;
		}

		TimeInterval maxAge = TimeInterval::seconds(queryData.getInteger("maxAge"));
		if (!maxAge || maxAge > config::AUTH_MAX_TIME) {
			maxAge = config::AUTH_MAX_TIME;
		}

		auto user = db::User::get(_transaction, name, passwd);
		if (!user) {
			Root::getCurrent()->error("Auth", "Invalid username or password");
			return false;
		}

		auto &opts = getOptions();
		bool isAuthorized = false;
		if (!opts.getBool("AdminOnly") || user->isAdmin()) {
			isAuthorized = true;
		}

		if (isAuthorized) {
			Session *session = _request.authorizeUser(user, maxAge);
			if (session) {
				auto &token = session->getSessionToken();
				result.setString(base64url::encode<Interface>(CoderSource(token.data(), token.size())), "token");
				result.setInteger(maxAge.toSeconds(), "maxAge");
				result.setInteger(user->getObjectId(), "userId");
				result.setString(user->getName(), "userName");
				if (queryData.getBool("userdata")) {
					auto &val = result.emplace("userData");
					for (auto &it : *user) {
						val.setValue(it.second, it.first);
					}
					val.erase("password");
				}
				return true;
			}
		}

		Root::getCurrent()->error("Auth", "Fail to create session");

		return false;
	} else if (_subPath == "/touch") {
		if (auto u = rctx.getAuthorizedUser()) {
			result.setInteger(u->getObjectId(), "userId");
			result.setString(u->getName(), "userName");
			return true;
		}
		return false;
	} else if (_subPath == "/update") {
		if (auto session = rctx.getSession()) {
			db::User *user = session->getUser();
			if (!user) {
				return false;
			}
			TimeInterval maxAge = TimeInterval::seconds(rctx.getInfo().queryData.getInteger("maxAge"));
			if (!maxAge || maxAge > config::AUTH_MAX_TIME) {
				maxAge = config::AUTH_MAX_TIME;
			}

			if (session->touch(maxAge)) {
				auto &token = session->getSessionToken();
				result.setString(base64url::encode<Interface>(CoderSource(token.data(), token.size())), "token");
				result.setInteger(maxAge.toSeconds(), "maxAge");
				result.setInteger(user->getObjectId(), "userId");
				result.setString(user->getName(), "userName");
				if (queryData.getBool("userdata")) {
					auto &val = result.emplace("userData");
					for (auto &it : *user) {
						val.setValue(it.second, it.first);
					}
					val.erase("password");
				}
				return true;
			}
		}

		return false;
	} else if (_subPath == "/cancel") {
		if (auto session = rctx.getSession()) {
			return session->cancel();
		}
		return false;
	} else if (_subPath == "/setup") {
		auto &name = queryData.getString("name");
		auto &passwd = queryData.getString("passwd");

		if (name.empty() || passwd.empty()) {
			Root::getCurrent()->error("Auth", "Name or password is not specified", Value{
				std::make_pair("Doc", Value("You should specify 'name' and 'passwd' variables in request"))
			});
			return false;
		}

		auto user = db::User::setup(_transaction, name, passwd);
		if (user) {
			Value &u = result.emplace("user");
			for (auto &it : *user) {
				u.setValue(it.second, it.first);
			}
			return true;
		} else {
			Root::getCurrent()->error("Auth", "Setup failed");
			return false;
		}
	}
	return false;
}

}
