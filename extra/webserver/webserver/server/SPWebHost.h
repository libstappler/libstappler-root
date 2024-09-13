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

#ifndef EXTRA_WEBSERVER_WEBSERVER_SERVER_SPWEBHOST_H_
#define EXTRA_WEBSERVER_WEBSERVER_SERVER_SPWEBHOST_H_

#include "SPWebInfo.h"
#include "SPPugCache.h"
#include "SPPugTemplate.h"
#include "SPCrypto.h"
#include "SPSqlDriver.h"

namespace STAPPLER_VERSIONIZED stappler::web {

class Root;
class AsyncTask;
class Request;
class HostController;
class HostComponent;
class WebsocketManager;

class SP_PUBLIC Host final : public AllocBase {
public:
	using HandlerCallback = Function<RequestHandler *()>;

	static Host getCurrent();

	Host();
	Host(HostController *);
	Host & operator =(HostController *);

	Host(Host &&);
	Host & operator =(Host &&);

	Host(const Host &);
	Host & operator =(const Host &);

	void checkBroadcasts();

	void handleChildInit(pool_t *rootPool);
	void handleHeartBeat(pool_t *);
	void handleBroadcast(const Value &);
	void handleBroadcast(const BytesView &);
	Status handleRequest(Request &);

	void initTransaction(db::Transaction &);

	void addSourceRoot(StringView file);
	void addComponentByParams(StringView w);
	void addWasmComponentByParams(StringView path, StringView command);
	void addAllow(StringView);
	void setSessionParams(StringView w);
	void setHostSecret(StringView w);
	void setWebHookParams(StringView w);
	void setForceHttps();
	void setProtectedList(StringView w);
	void setDbParams(StringView w);

	void addProtectedLocation(const StringView &);

	template <typename Component = HostComponent>
	auto getComponent(const StringView &) const -> Component *;

	template <typename Component>
	auto getComponent() const -> Component *;

	template <typename Component>
	auto addComponent(Component *) -> Component *;

	const Map<StringView, HostComponent *> &getComponents() const;

	void addPreRequest(Function<Status(Request &)> &&) const;

	void addHandler(StringView, const HandlerCallback &, const Value & = Value::Null) const;
	void addHandler(std::initializer_list<StringView>, const HandlerCallback &, const Value & = Value::Null) const;
	void addHandler(StringView, const RequestHandlerMap *) const;
	void addHandler(std::initializer_list<StringView>, const RequestHandlerMap *) const;

	void addResourceHandler(StringView, const db::Scheme &) const;
	void addResourceHandler(StringView, const db::Scheme &, const Value &val) const;
	void addMultiResourceHandler(StringView, std::initializer_list<Pair<const StringView, const db::Scheme *>> &&) const;

	void addWebsocket(StringView, WebsocketManager *) const;

	const db::Scheme * exportScheme(const db::Scheme &) const;

	const db::Scheme * getScheme(const StringView &) const;
	const db::Scheme * getFileScheme() const;
	const db::Scheme * getUserScheme() const;
	const db::Scheme * getErrorScheme() const;

	db::Scheme * getMutable(const db::Scheme *) const;

	StringView getResourcePath(const db::Scheme &) const;

	const Map<StringView, const db::Scheme *> &getSchemes() const;
	const Map<const db::Scheme *, ResourceSchemeInfo> &getResources() const;
	const Map<StringView, RequestSchemeInfo> &getRequestHandlers() const;

	void reportError(const Value &);

	bool performTask(AsyncTask *task, bool performFirst = false) const;
	bool scheduleTask(AsyncTask *task, TimeInterval) const;

	void performWithStorage(const Callback<void(const db::Transaction &)> &cb, bool openNewConnecton = false) const;

	db::BackendInterface *acquireDbForRequest(const Request &) const;

	explicit operator bool () const { return _config != nullptr; }

	pool_t *getThreadPool() const;

	Host next() const;

	const HostInfo &getHostInfo() const;
	const SessionInfo &getSessionInfo() const;

	HostController *getController() const { return _config; }

	Root *getRoot() const;

	pug::Cache *getPugCache() const;
	db::sql::Driver *getDbDriver() const;

	bool setHostKey(BytesView priv) const;
	void setHostKey(crypto::PrivateKey &&) const;

	const crypto::PublicKey &getHostPublicKey() const;
	const crypto::PrivateKey &getHostPrivateKey() const;
	BytesView getHostSecret() const;

	bool isSecureAuthAllowed(const Request &rctx) const;

	CompressionInfo *getCompressionConfig() const;

	String getDocumentRootPath(StringView) const;

protected:
	void processReports() const;

	void addComponentWithName(const StringView &, HostComponent *);

	HostComponent *getHostComponent(const StringView &name) const;
	HostComponent *getHostComponent(std::type_index name) const;

	void runErrorReportTask(const Request &, const Vector<Value> &);

	HostController *_config = nullptr;
};


template <typename Component>
inline auto Host::getComponent(const StringView &name) const -> Component * {
	return dynamic_cast<Component *>(getHostComponent(name));
}

template <typename Component>
inline auto Host::getComponent() const -> Component * {
	if (auto c = getHostComponent(std::type_index(typeid(Component)))) {
		return static_cast<Component *>(c);
	}

	auto &cmp = getComponents();
	for (auto &it : cmp) {
		if (auto c = dynamic_cast<Component *>(it.second)) {
			return c;
		}
	}

	return nullptr;
}

template <typename Component>
auto Host::addComponent(Component *c) -> Component * {
	addComponentWithName(c->getName(), c);
	return c;
}

}

#endif /* EXTRA_WEBSERVER_WEBSERVER_SERVER_SPWEBHOST_H_ */
