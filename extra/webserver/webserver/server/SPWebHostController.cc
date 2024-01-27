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

#include "SPWebHostController.h"
#include "SPWebHostComponent.h"
#include "SPWebHost.h"
#include "SPWebRoot.h"
#include "SPWebDbd.h"

#include "SPValid.h"
#include "SPDbFieldTextArray.h"
#include "SPJsonWebToken.h"
#include "SPDso.h"

namespace stappler::web {

HostController::~HostController() { }

HostController::HostController(Root *root, pool_t *pool)
: _root(root), _rootPool(pool)
#if DEBUG
, _pugCache(pug::Template::Options::getPretty(), [this] (const StringView &str) { handleTemplateError(str); })
#else
, _pugCache(pug::Template::Options::getDefault(), [this] (const StringView &str) { handleTemplateError(str); })
#endif

{
	_rootPool = memory::pool::acquire();
	// add virtual files to template engine
	/*size_t count = 0;
	auto d = tools::VirtualFile::getList(count);
	for (size_t i = 0; i < count; ++ i) {
		if (d[i].name.ends_with(".pug") || d[i].name.ends_with(".spug")) {
			_pugCache.addTemplate(toString("virtual:/", d[i].name), d[i].content.str());
		} else {
			_pugCache.addContent(toString("virtual:/", d[i].name), d[i].content.str());
		}
	}*/

	memset(_hostSecret.data(), 0, _hostSecret.size());
}

db::Scheme HostController::makeUserScheme() const {
	return db::Scheme(config::USER_SCHEME_NAME, {
		db::Field::Text("name", db::Transform::Alias, db::Flags::Required),
		db::Field::Bytes("pubkey", db::Transform::PublicKey, db::Flags::Indexed),
		db::Field::Password("password", db::PasswordSalt(config::DEFAULT_PASSWORD_SALT), db::Flags::Required | db::Flags::Protected),
		db::Field::Boolean("isAdmin", Value(false)),
		db::Field::Extra("data", Vector<db::Field>({
			db::Field::Text("email", db::Transform::Email),
			db::Field::Text("public"),
			db::Field::Text("desc"),
		})),
		db::Field::Text("email", db::Transform::Email, db::Flags::Unique),
	});
}

db::Scheme HostController::makeFileScheme() const {
	return db::Scheme(config::FILE_SCHEME_NAME, {
		db::Field::Text("location", db::Transform::Url),
		db::Field::Text("type", db::Flags::ReadOnly),
		db::Field::Integer("size", db::Flags::ReadOnly),
		db::Field::Integer("mtime", db::Flags::AutoMTime | db::Flags::ReadOnly),
		db::Field::Extra("image", Vector<db::Field>{
			db::Field::Integer("width"),
			db::Field::Integer("height"),
		})
	});
}

db::Scheme HostController::makeErrorScheme() const {
	return db::Scheme(config::ERROR_SCHEME_NAME, {
		db::Field::Boolean("hidden", Value(false)),
		db::Field::Boolean("delivered", Value(false)),
		db::Field::Text("name"),
		db::Field::Text("documentRoot"),
		db::Field::Text("url"),
		db::Field::Text("request"),
		db::Field::Text("ip"),
		db::Field::Data("headers"),
		db::Field::Data("data"),
		db::Field::Integer("time"),
		db::Field::Custom(new db::FieldTextArray("tags", db::Flags::Indexed,
				db::DefaultFn([&] (const Value &data) -> Value {
			Vector<String> tags;
			for (auto &it : data.getArray("data")) {
				auto text = it.getString("source");
				if (!text.empty()) {
					emplace_ordered(tags, text);
				}
			}

			Value ret;
			for (auto &it : tags) {
				ret.addString(it);
			}
			return ret;
		})))
	});
}

bool HostController::loadComponent(const Host &serv, const HostComponentInfo &info) {
	if (info.file.empty()) {
		auto h = Dso(StringView(), DsoFlags::Self);
		if (h) {
			auto sym = h.sym<HostComponent::Symbol>(info.symbol);
			if (sym) {
				auto comp = sym(serv, info);
				if (comp) {
					_components.emplace(comp->getName().str<Interface>(), comp);
					_typedComponents.emplace(std::type_index(typeid(*comp)), comp);
					return true;
				} else {
					log::error("web::HostController", "Symbol ", info.symbol ," not found in DSO '", info.file, "'");
				}
			} else {
				log::error("web::HostController", "DSO: ", h.getError());
			}
		} else {
			log::error("web::HostController", "DSO: ", h.getError());
		}
	}

	return false;
}

void HostController::initComponents(const Host &serv, const Vector<HostComponentInfo> &val) {
	for (auto &it : val) {
		loadComponent(serv, it);
	}
}

void HostController::initSession(const Value &val) {
	_session.init(val);
}

void HostController::initWebhook(const Value &val) {
	_webhook.init(val);
}

void HostController::setSessionParam(StringView n, StringView v) {
	_session.setParam(n, v);
}
void HostController::setWebhookParam(StringView n, StringView v) {
	_webhook.setParam(n, v);
}

void HostController::setForceHttps() {
	_forceHttps = true;
}

void HostController::setHostSecret(StringView w, crypto::HashFunction fn) {
	switch (fn) {
	case crypto::HashFunction::SHA_2:
		_hostSecret = crypto::Sha512::hmac(w, w);
		break;
	case crypto::HashFunction::GOST_3411:
		_hostSecret = crypto::Gost3411_512::hmac(w, w);
		break;

	}
}

void HostController::setHostKey(crypto::PrivateKey &&key) {
	_hostPrivKey = move(key);
	_hostPubKey = _hostPrivKey.exportPublic();
}

void HostController::addAllowed(StringView r) {
	auto p = valid::readIpRange(r);
	if (p.first && p.second) {
		_allowedIps.emplace_back(p.first, p.second);
	}
}

void HostController::init(const Host &serv) {
	_defaultUserScheme = makeUserScheme();
	_defaultFileScheme = makeFileScheme();
	_defaultErrorScheme = makeErrorScheme();

	_schemes.emplace(_defaultUserScheme.getName(), &_defaultUserScheme);
	_schemes.emplace(_defaultFileScheme.getName(), &_defaultFileScheme);
	_schemes.emplace(_defaultErrorScheme.getName(), &_defaultErrorScheme);

	if (!_componentsToLoad.empty()) {
		initComponents(serv, _componentsToLoad);
	}
}

bool HostController::initKeyPair(const Host &serv, const db::Adapter &a, BytesView fp) {
	if (_hostPrivKey.generate(crypto::KeyType::GOST3410_2012_512)) {
		_hostPubKey = _hostPrivKey.exportPublic();
	}

	Bytes priv;
	_hostPrivKey.exportPem([&] (BytesView data) {
		priv = data.bytes<Interface>();
	});

	crypto::PrivateKey tmpKey(config::INTERNAL_PRIVATE_KEY);

	auto tok = AesToken<Interface>::create(AesToken<Interface>::Keys{ nullptr, &tmpKey, BytesView(_hostSecret) });
	tok.setBytes(move(priv), "priv");
	if (auto d = tok.exportData(AesToken<Interface>::Fingerprint(crypto::HashFunction::GOST_3411, fp))) {
		std::array<uint8_t, string::Sha512::Length + 4> data;
		memcpy(data.data(), "srv:", 4);
		memcpy(data.data() + 4, fp.data(), fp.size());

		a.set(data, d, TimeInterval::seconds(60 * 60 * 365 * 100)); // 100 years
		return true;
	}
	return false;
}

void HostController::initHostKeys(const Host &serv, const db::Adapter &a) {
	if (_hostPrivKey) {
		return;
	}

	auto fp = crypto::Gost3411_512::hmac(_hostInfo.hostname, _hostSecret);

	std::array<uint8_t, string::Sha512::Length + 4> data;
	memcpy(data.data(), "srv:", 4);
	memcpy(data.data() + 4, fp.data(), fp.size());

	if (auto d = a.get(data)) {
		crypto::PrivateKey tmpKey(config::INTERNAL_PRIVATE_KEY);
		if (auto tok = AesToken<Interface>::parse(d,
				AesToken<Interface>::Fingerprint(crypto::HashFunction::GOST_3411, BytesView(fp)),
				AesToken<Interface>::Keys{ nullptr, &tmpKey, BytesView(_hostSecret) })) {
			auto &d = tok.getBytes("priv");
			_hostPrivKey.import(d);
			if (_hostPrivKey) {
				_hostPubKey = _hostPrivKey.exportPublic();
			}
		}
	}

	if (!_hostPubKey) {
		initKeyPair(serv, a, fp);
	}
}

void HostController::handleChildInit(const Host &host, pool_t *p) {
	_rootPool = p;
	for (auto &it : _components) {
		_currentComponent = it.second->getName();
		it.second->handleChildInit(host);
		_currentComponent = StringView();
	}

	_childInit = true;

	auto pool = getCurrentPool();

	db::sql::Driver::Handle db;
	if (!_dbParams.empty()) {
		// run custom dbd
		_customDbd = DbdModule::create(_rootPool, _root, move(_dbParams));
		_dbDriver = _customDbd->getDriver();
		db = _customDbd->openConnection(pool);
	} else {
		// setup apache httpd dbd
		db = _root->dbdOpen(pool, host);
		if (db.get()) {
			_dbDriver = openInternalDriver(db);
		} else {
			log::error("web::HostController", "fail to open internal DB connection");
		}
	}

	if (!_schemes.empty() && !db.get()) {
		_loadingFalled = true;
	}

	if (!_loadingFalled) {
		db::Scheme::initSchemes(_schemes);

		perform_temporary([&] {
			_dbDriver->performWithStorage(db, [&] (const db::Adapter &storage) {
				storage.init(db::BackendInterface::Config{StringView(_hostInfo.hostname), host.getFileScheme()}, _schemes);

				if (_hostSecret != string::Sha512::Buf{0}) {
					initHostKeys(host, storage);
				}

				for (auto &it : _components) {
					_currentComponent = it.second->getName();
					it.second->handleStorageInit(host, storage);
					_currentComponent = String();
				}
			});
		}, p);
	}

	if (db.get()) {
		if (_customDbd) {
			_customDbd->closeConnection(db);
		} else {
			_root->dbdClose(host, db);
		}
	}
}

void HostController::initTransaction(db::Transaction &t) {
	for (auto &it : _components) {
		it.second->initTransaction(t);
	}
}

void HostController::setDbParams(StringView str) {
	perform([&] {
		Root::parseParameterList(_dbParams, str);
	}, _rootPool);
}

db::sql::Driver::Handle HostController::openConnection(pool_t *pool, bool bindConnection) const {
	auto h = _customDbd->openConnection(pool);
	if (bindConnection) {
		pool::cleanup_register(pool, [h, dbd = _customDbd] {
			dbd->closeConnection(h);
		});
	}
	return h;
}

void HostController::closeConnection(db::sql::Driver::Handle h) const {
	_customDbd->closeConnection(h);
}

db::sql::Driver *HostController::openInternalDriver(db::sql::Driver::Handle) {
	log::error("web::HostController", "VirtualServerConfig::openInternalDriver is not implemented");
	return nullptr;
}

void HostController::handleTemplateError(const StringView &str) {
#if DEBUG
	std::cout << "[Template]: " << str << "\n";
#endif
	log::error("Template", "Template compilation error", Value(str));
}

}
