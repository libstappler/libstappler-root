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
#include "SPWebInputFilter.h"
#include "SPWebHostComponent.h"
#include "SPDbUser.h"
#include "SPDbContinueToken.h"
#include "SPDbQuery.h"
#include "SPDbAdapter.h"
#include "SPSqlHandle.h"

namespace STAPPLER_VERSIONIZED stappler::web::tools {

static String Tools_getCancelUrl(Request &rctx) {
	StringStream cancelUrl;
	bool isSecure = rctx.isSecureConnection();
	cancelUrl << (isSecure?"https":"http") << "://nobody@" << rctx.getInfo().url.host;
	auto port = rctx.getInfo().url.port;
	if (!port.empty() && ((isSecure && port != "443") || (!isSecure && port != "80"))) {
		cancelUrl << ':' << port;
	}
	cancelUrl << "/__server";
	return cancelUrl.str();
}

bool ServerGui::isRequestPermitted(Request &req) {
	_transaction = req.acquireDbTransaction();
	return _transaction ? true : false;
}

void ServerGui::defineBasics(pug::Context &exec, Request &req, db::User *u) {
	exec.set("version", Value(config::getWebserverVersionString()));
	exec.set("hasDb", Value(true));
	exec.set("setup", Value(true));
	if (u) {
		exec.set("user", true, &u->getData());
		exec.set("auth", Value({
			pair("id", Value(u->getObjectId())),
			pair("name", Value(u->getString("name"))),
			pair("cancel", Value(Tools_getCancelUrl(req)))
		}));

		req.performWithStorage([&] (const db::Transaction &t) {
			exec.set("dbName", Value(t.getAdapter().getBackendInterface()->getDatabaseName()));
			return true;
		});

		Value components;
		for (auto &it : req.host().getComponents()) {
			components.addValue(Value({
				pair("name", Value(it.second->getName())),
				pair("version", Value(it.second->getVersion())),
			}));
		}
		exec.set("components", move(components));
		exec.set("root", Value(req.host().getHostInfo().documentRoot));
	}
}

Status ServerGui::onTranslateName(Request &rctx) {
	if (rctx.getInfo().queryData.getBool("auth")) {
		if (rctx.getAuthorizedUser()) {
			return rctx.redirectTo(rctx.getInfo().url.path);
		} else {
			return HTTP_UNAUTHORIZED;
		}
	}

	if (rctx.getInfo().method == RequestMethod::Get) {
		auto userScheme = rctx.host().getUserScheme();
		size_t count = 0;
		bool hasDb = false;
		if (userScheme) {
			count = userScheme->count(_transaction);
			hasDb = true;
		}
		rctx.runPug("virtual://html/server.pug", [&] (pug::Context &exec, const pug::Template &) -> bool {
			exec.set("count", Value(count));
			if (auto u = rctx.getAuthorizedUser()) {
				defineBasics(exec, rctx, u);

				auto root = Root::getCurrent();
				auto stat = root->getStat();

				StringStream ret;
				ret << "\nResource stat:\n";
				ret << "\tRequests recieved: " << stat.requestsReceived << "\n";
				ret << "\tHeartbeat counter: " << stat.heartbeatCounter << "\n";
				ret << "\tDB queries performed: " << stat.dbQueriesPerformed << " (" << stat.dbQueriesReleased << " " << stat.dbQueriesPerformed - stat.dbQueriesReleased << ")\n";
				ret << "\n";

				exec.set("resStat", Value(ret.str()));
			} else {
				exec.set("setup", Value(count != 0));
				exec.set("hasDb", Value(hasDb));
				exec.set("version", Value(config::getWebserverVersionString()));
			}

			return true;
		});
		return DONE;
	} else {
		return DataHandler::onPostReadRequest(rctx);
	}
	return DECLINED;
}

void ServerGui::onFilterComplete(InputFilter *filter) {
	const auto data = filter->getData();
	Request rctx(filter->getRequest());
	_filter = filter;

	auto &name = data.getString("name");
	auto &passwd = data.getString("passwd");
	if (!name.empty() && !passwd.empty()) {
		_transaction.performAsSystem([&, this] () -> bool {
			return db::User::setup(_transaction, name, passwd) != nullptr;
		});
	}

	rctx.redirectTo(rctx.getInfo().unparserUri);
	rctx.setStatus(HTTP_SEE_OTHER);
}

}
