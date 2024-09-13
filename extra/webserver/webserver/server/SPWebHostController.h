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

#ifndef EXTRA_WEBSERVER_WEBSERVER_SERVER_SPWEBHOSTCONTROLLER_H_
#define EXTRA_WEBSERVER_WEBSERVER_SERVER_SPWEBHOSTCONTROLLER_H_

#include "SPWebInfo.h"
#include "SPCrypto.h"
#include "SPPugCache.h"
#include "SPSqlDriver.h"

#if MODULE_STAPPLER_WASM
#include "SPWasm.h"
#endif

namespace STAPPLER_VERSIONIZED stappler::web {

class Root;
class WebsocketManager;
class Request;
class HostComponent;
class Host;
class DbdModule;

class SP_PUBLIC HostController : public AllocBase {
public:
	virtual ~HostController();

	HostController(Root *, pool_t *);

	virtual bool loadComponent(const Host &serv, const HostComponentInfo &);

	virtual db::Scheme makeUserScheme() const;
	virtual db::Scheme makeFileScheme() const;
	virtual db::Scheme makeErrorScheme() const;

	void initComponents(const Host &serv, const Vector<HostComponentInfo> &val);

	void initSession(const Value &val);
	void initWebhook(const Value &val);

	void setSessionParam(StringView n, StringView v);
	void setWebhookParam(StringView n, StringView v);

	void setForceHttps();

	void setHostSecret(StringView w);
	void setHostKey(crypto::PrivateKey &&);

	void addAllowed(StringView r);

	void init(const Host &serv);

	bool initKeyPair(const Host &serv, const db::Adapter &a, BytesView fp);

	void initHostKeys(const Host &serv, const db::Adapter &a);

	virtual void handleChildInit(const Host &serv, pool_t *p);

	virtual void initTransaction(db::Transaction &t);

	void setDbParams(StringView str);

	const SessionInfo &getSessionInfo() const { return _session; }
	const WebhookInfo &getWebhookInfo() const { return _webhook; }
	const HostInfo &getHostInfo() const { return _hostInfo; }

	Root *getRoot() const { return _root; }
	pool_t *getRootPool() const { return _rootPool; }

	const Map<StringView, StringView> &getDbParams() const { return _dbParams; }

	virtual db::sql::Driver::Handle openConnection(pool_t *, bool bindConnection) const;
	virtual void closeConnection(db::sql::Driver::Handle) const;

protected:
	virtual db::sql::Driver * openInternalDriver(db::sql::Driver::Handle);

	virtual bool loadDsoComponent(const Host &serv, const HostComponentInfo &);
	virtual bool loadWasmComponent(const Host &serv, const HostComponentInfo &);

	virtual String resolvePath(StringView path) const;

	void handleTemplateError(const StringView &str);

	friend class Host;

	Root *_root = nullptr;
	pool_t *_rootPool = nullptr;

	db::Scheme _defaultUserScheme = db::Scheme(config::USER_SCHEME_NAME);
	db::Scheme _defaultFileScheme = db::Scheme(config::FILE_SCHEME_NAME);
	db::Scheme _defaultErrorScheme = db::Scheme(config::ERROR_SCHEME_NAME);

	crypto::PrivateKey _hostPrivKey;
	crypto::PublicKey _hostPubKey;
	string::Sha512::Buf _hostSecret;

	Vector<HostComponentInfo> _componentsToLoad;

	Vector<StringView> _sourceRoot;
	StringView _currentComponent;
	Vector<Function<Status(Request &)>> _preRequest;
	Map<StringView, HostComponent *> _components;
	Map<std::type_index, HostComponent *> _typedComponents;
	Map<StringView, RequestSchemeInfo> _requests;
	Map<const db::Scheme *, ResourceSchemeInfo> _resources;
	Map<StringView, const db::Scheme *> _schemes;
	Map<StringView, WebsocketManager *> _websockets;
	Set<StringView> _protectedList;

	HostInfo _hostInfo;
	SessionInfo _session;
	WebhookInfo _webhook;
	CompressionInfo _compression;

	bool _childInit = false;
	bool _loadingFalled = false;
	bool _forceHttps = false;

	Time _lastDatabaseCleanup;
	Time _lastTemplateUpdate;
	int64_t _broadcastId = 0;
	pug::Cache _pugCache;

	Vector<Pair<uint32_t, uint32_t>> _allowedIps;
	Map<StringView, StringView> _dbParams;
	DbdModule *_customDbd = nullptr;
	db::sql::Driver *_dbDriver = nullptr;

#if MODULE_STAPPLER_WASM
protected:
	virtual wasm::Module *loadWasmModule(StringView name, StringView path);

	Map<StringView, Rc<wasm::Module>> _wasmModules;
#endif
};


}

#endif /* EXTRA_WEBSERVER_WEBSERVER_SERVER_SPWEBHOSTCONTROLLER_H_ */
