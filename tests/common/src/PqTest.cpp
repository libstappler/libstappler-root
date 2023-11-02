/**
 Copyright (c) 2023 Stappler LLC <admin@stappler.dev>

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

#include "SPCommon.h"
#include "Test.h"

#if MODULE_STAPPLER_DB

#include "SPData.h"
#include "STSqlDriver.h"
#include "STStorageScheme.h"
#include "STFieldTextArray.h"
#include "STPqHandle.h"

namespace stappler::db::test::detail {

using namespace stappler::mem_pool;

class Server;

class ServerScheme : public RefBase<memory::StandartInterface> {
public:
	ServerScheme(memory::pool_t *p, uint32_t version);

	virtual void fillSchemes(db::Map<StringView, const db::Scheme *> &);

	const db::Scheme &getFileScheme() const { return _users; }
	const db::Scheme &getUserScheme() const { return _files; }
	const db::Scheme &getErrorScheme() const { return _errors; }

	bool runAccessTest(const db::Transaction &t, db::AccessRoleId);
	bool runCompressionTest(const db::Transaction &t);
	bool runDeltaTest(Server &server);
	bool runRelationTest(Server &server);
	bool runVirtualTest(const db::Transaction &t);

protected:
	memory::pool_t *_pool = nullptr;
	db::Scheme _users = db::Scheme("__users");
	db::Scheme _files = db::Scheme("__files");
	db::Scheme _errors = db::Scheme("__error");

	db::Scheme _accessTest = db::Scheme("access_test");
	db::Scheme _compressionTest = db::Scheme("compression_test");
	db::Scheme _deltaTest = db::Scheme("delta_test", db::Scheme::Options::WithDelta);

	db::Scheme _objects = db::Scheme("objects");
	db::Scheme _refs = db::Scheme("refs");
	db::Scheme _subobjects = db::Scheme("subobjects");
	db::Scheme _images = db::Scheme("images");
	db::Scheme _hierarchy = db::Scheme("hierarchy");
	db::Scheme _pages = db::Scheme("pages");

	db::Scheme _updateTestScheme = db::Scheme("update_test_scheme");
	db::Scheme _virtualTest = db::Scheme("virtual_test");
};

class Server : public db::StorageRoot {
public:
	virtual ~Server();
	Server(const mem_std::Value &, const Callback<Rc<ServerScheme>(memory::pool_t *)> &, db::AccessRoleId);

	virtual void scheduleAyncDbTask(const db::Callback<db::Function<void(const db::Transaction &)>(db::pool_t *)> &setupCb) override;
	virtual db::String getDocuemntRoot() const override;
	virtual const db::Scheme *getFileScheme() const override;
	virtual const db::Scheme *getUserScheme() const override;

	virtual void onLocalBroadcast(const db::Value &) override;
	virtual void onStorageTransaction(db::Transaction &) override;

	void update();
	void perform(const Callback<bool(const db::Transaction &)> &cb);

protected:
	memory::pool_t *_staticPool = nullptr;
	memory::pool_t *_contextPool = nullptr;
	Rc<ServerScheme> _scheme;

	db::AccessRoleId _defaultRole;
	db::sql::Driver *_driver = nullptr;
	db::sql::Driver::Handle _handle;
	db::BackendInterface::Config interfaceConfig;
	db::Vector<db::Function<void(const db::Transaction &)>> *asyncTasks = nullptr;
};

ServerScheme::ServerScheme(memory::pool_t *p, uint32_t version) : _pool(p) {
	using namespace db;

	_users.define({
		Field::Text("name", Transform::Alias, Flags::Required),
		Field::Bytes("pubkey", Transform::PublicKey, Flags::Indexed),
		Field::Password("password", PasswordSalt("TestPasswordSalt"), Flags::Required | Flags::Protected),
		Field::Boolean("isAdmin", Value(false)),
		Field::Extra("data", Vector<Field>({
			Field::Text("email", Transform::Email),
			Field::Text("public"),
			Field::Text("desc"),
		})),
		Field::Text("email", Transform::Email, Flags::Unique),
	});

	_files.define({
		Field::Text("location", Transform::Url),
		Field::Text("type", Flags::ReadOnly),
		Field::Integer("size", Flags::ReadOnly),
		Field::Integer("mtime", Flags::AutoMTime | Flags::ReadOnly),
		Field::Extra("image", Vector<Field>{
			Field::Integer("width"),
			Field::Integer("height"),
		})
	});

	_errors.define({
		Field::Boolean("hidden", Value(false)),
		Field::Boolean("delivered", Value(false)),
		Field::Text("name"),
		Field::Text("documentRoot"),
		Field::Text("url"),
		Field::Text("request"),
		Field::Text("ip"),
		Field::Data("headers"),
		Field::Data("data"),
		Field::Integer("time"),
		Field::Custom(new FieldTextArray("tags", db::Flags::Indexed,
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

	_accessTest.define(Vector<Field>({
		Field::Text("atime", Flags::AutoCTime),
		Field::Text("name", MinLength(3)),
		Field::Integer("role", Flags::Indexed)
	}),
	AccessRole::Empty(AccessRoleId::Nobody,
		AccessRole::OnCreate([] (Worker &, Value &obj) {
			obj.setString("Unauthorized", "name");
			obj.setInteger(0, "role");
			return true;
		}),
		AccessRole::OnReturn([] (const Scheme &, Value &) {
			return false;
		})
	),
	AccessRole::Empty(AccessRoleId::Authorized,
		AccessRole::OnSelect([] (Worker &, const Query &) {
			return true;
		}),
		AccessRole::OnCreate([] (Worker &, Value &obj) {
			obj.setInteger(1, "role");
			return true;
		}),
		AccessRole::OnReturn([] (const Scheme &, Value &val) {
			return val.getInteger("role") == 1;
		})
	),
	AccessRole::Admin(AccessRoleId::Admin,
		AccessRole::OnCreate([] (Worker &, Value &obj) {
			obj.setInteger(toInt(AccessRoleId::Admin), "role");
			return true;
		})
	));

	_compressionTest.define({
		Field::Text("name", MinLength(3)),
		Field::Data("message", Flags::Compressed),
	});

	_deltaTest.define({
		Field::Text("name", MinLength(3)),
	});

	_objects.define(Vector<Field>({
		Field::Text("text", MinLength(3)),
		Field::Extra("data", Vector<Field>{
			Field::Array("strings", Field::Text("")),
		}),
		Field::Set("subobjects", _subobjects),
		Field::File("image", MaxFileSize(1_MiB)),
		Field::Text("alias", Transform::Alias),
		Field::Integer("mtime", Flags::AutoMTime | Flags::Indexed),
		Field::Integer("index", Flags::Indexed),
		Field::View("refs", _refs, ViewFn([this] (const Scheme &objScheme, const Value &obj) -> bool {
			return true;
		}), FieldView::Delta),

		Field::Set("images", _images, Flags::Composed),
	}),
	AccessRole::Admin(AccessRoleId::Authorized));

	_refs.define({
		Field::Text("alias", Transform::Alias),
		Field::Text("text", MinLength(3)),
		Field::Set("features", _objects, RemovePolicy::StrongReference), // objects, that will be removed when ref is removed
		Field::Set("optionals", _objects, RemovePolicy::Reference),
		Field::Integer("mtime", Flags::AutoMTime | Flags::Indexed),
		Field::Integer("index", Flags::Indexed),
		Field::File("file", MaxFileSize(100_KiB)),
		Field::Array("array", Field::Text("", MaxLength(10))),
		Field::Object("objectRef", _objects, Flags::Reference),

		Field::Image("cover", MaxImageSize(1080, 1080, ImagePolicy::Resize), Vector<Thumbnail>({
			Thumbnail("thumb", 160, 160),
			Thumbnail("cover512", 512, 512),
			Thumbnail("cover256", 256, 256),
			Thumbnail("cover128", 128, 128),
			Thumbnail("cover64", 64, 64),
		})),

		Field::Data("data")
	});

	_subobjects.define({
		Field::Text("text", MinLength(3)),
		Field::Object("object", _objects, RemovePolicy::Cascade),
		Field::Integer("mtime", Flags::AutoMTime | Flags::Indexed),
		Field::Integer("index", Flags::Indexed),
	});

	_images.define(Vector<Field>({
		Field::Integer("ctime", Flags::ReadOnly | Flags::AutoCTime | Flags::ForceInclude),
		Field::Integer("mtime", Flags::ReadOnly | Flags::AutoMTime | Flags::ForceInclude),

		Field::Text("name", Transform::Identifier, Flags::Required | Flags::Indexed | Flags::ForceInclude),

		Field::Image("content", MaxImageSize(2048, 2048, ImagePolicy::Resize), Vector<Thumbnail>({
			Thumbnail("thumb", 380, 380)
		})),
	}),
		AccessRole::Admin(AccessRoleId::Authorized)
	);

	_hierarchy.define(Vector<Field>({
		Field::Text("name", MinLength(3)),
		Field::Integer("id", Flags::Indexed),
		Field::Object("root", _hierarchy, Linkage::Manual, ForeignLink("sections")),
		Field::Set("sections", _hierarchy, Linkage::Manual, ForeignLink("root")),

		Field::View("pages", _pages, ViewFn([this] (const Scheme &, const Value &obj) -> bool {
			return obj.getBool("hidden") ? false : true;
		}), Vector<String>({ "hidden" })),

		Field::Set("all_pages", _pages)
	}));

	_pages.define(Vector<Field>({
		Field::Text("name", MinLength(3)),
		Field::Boolean("hidden"),
		Field::Object("root", _hierarchy),
	}));

	switch (version) {
	case 0:
		_updateTestScheme.define({
			Field::Text("name", Transform::Alias, Flags::Required),
			Field::Text("email", Transform::Email, Flags::Unique),
			Field::Integer("score"),
		});
		break;
	case 1:
		_updateTestScheme.define({
			Field::Text("name", Transform::Alias, Flags::Required),
			Field::Text("email", Transform::Email),
			Field::Integer("score"),
		});
		break;
	case 2:
		_updateTestScheme.define({
			Field::Text("name", Transform::Alias, Flags::Required),
			Field::Text("email", Transform::Email, Flags::Unique),
			Field::Text("desc"),
		});
		break;
	default:
		_updateTestScheme.define({
			Field::Text("name", Transform::Alias, Flags::Required),
			Field::Text("email", Transform::Email),
		});
		break;
	}

	_virtualTest.define({
		Field::Integer("ctime", Flags::AutoCTime),
		Field::Text("name", MinLength(3)),
		Field::Text("defaulted", Value("default"), ReadFilterFn([] (const Scheme &, const Value &obj, Value &value) {
			if (value.getString() != "default") {
				value.setString("non-default");
			}
			return true;
		}), WriteFilterFn([] (const Scheme &, const Value &patch, Value &value, bool isCreate) {
			if (value.getString() != "default") {
				value.setString("non-default");
			}
			return true;
		})),
		Field::Data("callback", DefaultFn([] (const Value &val) {
			return Value({
				pair("name", val.getValue("name")),
				pair("time", Value(Time::now().toMicros()))
			});
		})),
		Field::Virtual("computed", VirtualReadFn([] (const Scheme &, const Value &value) {
			Value tmp(value);
			tmp.erase("__oid");
			tmp.setInteger(Time::now().toMicros(), "time");
			return tmp;
		}), Vector<String>({"name"})),

		Field::Virtual("virtual", VirtualReadFn([] (const Scheme &, const Value &value) {
			auto path = filesystem::writablePath<Interface>(toString(value.getString("name"), ".cbor"));
			if (filesystem::exists(path)) {
				return data::readFile<Interface>(path);
			}
			return Value();
		}), VirtualWriteFn([] (const Scheme &objScheme, const Value &obj, Value &data) {
			auto path = filesystem::writablePath<Interface>(toString(obj.getString("name"), ".cbor"));
			data::save<Interface>(data, path);
			return true;
		}), Vector<String>({"name"}))
	});
}

void ServerScheme::fillSchemes(db::Map<StringView, const db::Scheme *> &schemes) {
	schemes.emplace(_users.getName(), &_users);
	schemes.emplace(_files.getName(), &_files);
	schemes.emplace(_errors.getName(), &_errors);

	schemes.emplace(_accessTest.getName(), &_accessTest);
	schemes.emplace(_compressionTest.getName(), &_compressionTest);
	schemes.emplace(_deltaTest.getName(), &_deltaTest);

	schemes.emplace(_objects.getName(), &_objects);
	schemes.emplace(_refs.getName(), &_refs);
	schemes.emplace(_subobjects.getName(), &_subobjects);
	schemes.emplace(_images.getName(), &_images);
	schemes.emplace(_hierarchy.getName(), &_hierarchy);
	schemes.emplace(_pages.getName(), &_pages);

	schemes.emplace(_updateTestScheme.getName(), &_updateTestScheme);
	schemes.emplace(_virtualTest.getName(), &_virtualTest);
}

bool ServerScheme::runAccessTest(const db::Transaction &t, db::AccessRoleId role) {
	using namespace db;
	auto v = _accessTest.select(t, Query());

	auto ret = _accessTest.create(t, Value({
		pair("name", Value(toString("Access.", Time::now().toMicros()))),
		pair("role", Value(256))
	}));

	bool r = false;
	switch (role) {
	case db::AccessRoleId::Nobody:
		r = ret == Value(true);
		break;
	case db::AccessRoleId::Authorized:
		r = ret.getInteger("role") == 1;
		break;
	case db::AccessRoleId::Admin:
		r = ret.getInteger("role") == 13;
		break;
	case db::AccessRoleId::System:
		r = ret.getInteger("role") == 256;
		break;
	default:
		break;
	}

	return r;
}

bool ServerScheme::runCompressionTest(const db::Transaction &t) {
	using namespace db;
	return t.performAsSystem([&] {
		auto file = filesystem::currentDir<Interface>("lipsum.txt");
		if (!filesystem::exists(file)) {
			return false;
		}

		auto data = filesystem::readIntoMemory<Interface>(file);

		auto val = _compressionTest.create(t, Value({
			pair("name", Value(toString("Text", Time::now().toMicros()))),
			pair("message", Value({
				pair("text", Value(data))
			}))
		}));

		size_t messageSize = data.size();

		auto handle = (pq::Handle *)t.getAdapter().interface();
		auto query = toString("SELECT LENGTH(message) FROM ", _compressionTest.getName(), " WHERE __oid=", val.getInteger("__oid"));
		handle->performSimpleSelect(query, [&] (db::sql::Result &res) {
			messageSize = size_t(res.readId());
		});

		return messageSize < data.size();
	});
}

bool ServerScheme::runDeltaTest(Server &server) {
	using namespace db;

	auto runTest = [&] (const db::Transaction &t, int64_t id) {
		t.perform([&] {
			if (id) {
				_deltaTest.remove(t, id);
			}
			for (size_t i = 0; i < 3; ++ i) {
				id = _deltaTest.create(t, Value({
					pair("name", Value(toString("Test-B-", Time::now().toMicros())))
				})).getInteger("__oid");
			}

			return true;
		});
		return id;
	};

	auto checkTest = [&] (const db::Transaction &t, Time time) {
		t.perform([&] {
			QueryList list(&_deltaTest);
			list.setDelta(time);
			list.setAll();
			list.resolve(Vector<String>{"name"});

			auto objs = t.performQueryList(list);
			return objs.size() == 4 && objs.getValue(3).getValue("__delta").getString("action") == "delete";
		});
	};

	int64_t id;
	server.perform([&] (const db::Transaction &t) {
		id = runTest(t, 0);
		return true;
	});
	auto time = Time::now();
	server.perform([&] (const db::Transaction &t) {
		runTest(t, id);
		return true;
	});
	server.update();
	server.perform([&] (const db::Transaction &t) {
		checkTest(t, time);
		return true;
	});

	return true;
}

bool ServerScheme::runRelationTest(Server &server) {
	using namespace db;

	bool ret = false;

	auto fillTest = [&] (const db::Transaction &t, int64_t id) {
		using namespace db;

		t.performAsSystem([&] () {
			auto cat = _hierarchy.create(t, Value({
				pair("name", Value("TestCategory")),
				pair("id", Value(id)),
			}));

			_pages.create(t, Value({
				pair("name", Value("Page1")),
				pair("root", Value(cat.getInteger("__oid"))),
				pair("hidden", Value(false))
			}));

			_pages.create(t, Value({
				pair("name", Value("Page2")),
				pair("root", Value(cat.getInteger("__oid"))),
				pair("hidden", Value(false))
			}));

			_pages.create(t, Value({
				pair("name", Value("Page3")),
				pair("root", Value(cat.getInteger("__oid"))),
				pair("hidden", Value(true))
			}));

			return true;
		});
	};

	auto checkTest = [&] (const db::Transaction &t, int64_t id) {
		using namespace db;

		return t.performAsSystem([&] () {
			auto cat = _hierarchy.select(t, Query().select("id", Value(id))).getValue(0);
			if (!cat) {
				return false;
			}

			auto all = _hierarchy.getProperty(t, cat, "all_pages");

			auto pages = _hierarchy.getProperty(t, cat, "pages");

			return pages.size() == 2 && all.size() == 3;
		});
	};

	int64_t id = int64_t(Time::now().toMicros());

	server.perform([&] (const db::Transaction &t) {
		fillTest(t, id);
		return true;
	});
	server.update();
	server.perform([&] (const db::Transaction &t) {
		ret = checkTest(t, id);
		return true;
	});

	return ret;
}

bool ServerScheme::runVirtualTest(const db::Transaction &t) {
	using namespace db;

	t.performAsSystem([&] {
		auto data = _virtualTest.create(t, Value({
			pair("name", Value(toString("ValueName.", Time::now().toMicros())))
		}));

		auto data1 = _virtualTest.get(t, data.getInteger("__oid"));

		auto tmp = data;
		tmp.erase("__oid");

		auto tmp1 = data1.getValue("computed");
		tmp1.erase("time");
		tmp1.erase("path");

		_virtualTest.update(t, data.getInteger("__oid"), Value({
			pair("virtual",  data1.getValue("computed"))
		}));

		auto virtualTest = _virtualTest.get(t, data.getInteger("__oid"), {"virtual"});

		_virtualTest.remove(t, data.getInteger("__oid"));

		return true;
	});
	return true;
}

Server::~Server() {
	_scheme = nullptr;
	db::setStorageRoot(nullptr);
	if (_staticPool) {
		memory::pool::destroy(_staticPool);
	}
	memory::pool::terminate();
}

Server::Server(const mem_std::Value &params, const Callback<Rc<ServerScheme>(memory::pool_t *)> &cb, db::AccessRoleId role)
: _defaultRole(role) {
	db::setStorageRoot(this);

	memory::pool::initialize();
	_staticPool = memory::pool::create();
	_contextPool = memory::pool::create();

	db::Map<StringView, StringView> initParams;

	mem_pool::perform([&] {
		_scheme = cb(_staticPool);

		StringView driver;

		for (auto &it : params.asDict()) {
			if (it.first == "driver") {
				driver = StringView(it.second.getString());
			} else {
				initParams.emplace(it.first, it.second.getString());
			}
		}

		if (driver.empty()) {
			driver = StringView("sqlite");
		}

		_driver = db::sql::Driver::open(_staticPool, driver);
	}, _staticPool);

	if (!_driver || !_scheme) {
		return;
	}

	mem_pool::perform([&] {
		_handle = _driver->connect(initParams);

		if (!_handle.get()) {
			db::StringStream out;
			for (auto &it : initParams) {
				out << "\n\t" << it.first << ": " << it.second;
			}
			log::error("Server", "Fail to initialize DB with params: ", out.str());
		}
	}, _staticPool);

	if (!_handle.get()) {
		return;
	}


	auto dr = static_cast<pq::Driver *>(_driver);
	auto conn = dr->getConnection(_handle);

	auto res = dr->exec(conn,
		"DROP SCHEMA public CASCADE;"
		"CREATE SCHEMA public;"
		"GRANT ALL ON SCHEMA public TO postgres;"
		"GRANT ALL ON SCHEMA public TO public;");

	if (res.get()) {
		dr->clearResult(res);
	}

	mem_pool::perform([&] {
		_driver->init(_handle, db::Vector<db::StringView>());

		_driver->performWithStorage(_handle, [&] (const db::Adapter &adapter) {
			db::Map<StringView, const db::Scheme *> predefinedSchemes;
			_scheme->fillSchemes(predefinedSchemes);

			db::Scheme::initSchemes(predefinedSchemes);
			interfaceConfig.name = adapter.getDatabaseName();
			interfaceConfig.fileScheme = getFileScheme();
			adapter.init(interfaceConfig, predefinedSchemes);
		});
	}, _contextPool);
	memory::pool::clear(_contextPool);
}

void Server::scheduleAyncDbTask(const db::Callback<db::Function<void(const db::Transaction &)>(db::pool_t *)> &setupCb) {
	if (!asyncTasks) {
		asyncTasks = new (_contextPool) db::Vector<db::Function<void(const db::Transaction &)>>;
	}
	asyncTasks->emplace_back(setupCb(_contextPool));
}

db::String Server::getDocuemntRoot() const {
	return StringView(filesystem::writablePath<db::Interface>()).str<db::Interface>();
}

const db::Scheme *Server::getFileScheme() const {
	return &_scheme->getFileScheme();
}

const db::Scheme *Server::getUserScheme() const {
	return &_scheme->getUserScheme();
}

void Server::onLocalBroadcast(const db::Value &val) {
	//onBroadcast(nullptr, Value(val));
}

void Server::onStorageTransaction(db::Transaction &t) {
	t.setRole(_defaultRole);
}

void Server::update() {
	mem_pool::perform([&] {
		while (asyncTasks && _driver->isValid(_handle)) {
			auto tmp = asyncTasks;
			asyncTasks = nullptr;

			_driver->performWithStorage(_handle, [&] (const db::Adapter &adapter) {
				adapter.performWithTransaction([&] (const db::Transaction &t) {
					for (auto &it : *tmp) {
						it(t);
					}
					return true;
				});
			});
		}
	}, _contextPool);
	memory::pool::clear(_contextPool);
}

void Server::perform(const Callback<bool(const db::Transaction &)> &cb) {
	mem_pool::perform([&] {
		_driver->performWithStorage(_handle, [&] (const db::Adapter &adapter) {
			adapter.performWithTransaction([&] (const db::Transaction &t) {
				return cb(t);
			});
		});
	}, _contextPool);
	memory::pool::clear(_contextPool);
}

}

namespace stappler::app::test {

struct PqTest : MemPoolTest {
	struct Options {
		db::AccessRoleId role = db::AccessRoleId::Admin;
		String driver = "pgsql";
		String host = "localhost";
		String dbname = "dbtest";
		String user = "admin";
		String password = "admin";

		void loadFromFile(StringView fileName) {
			if (auto d = data::readFile<memory::PoolInterface>(fileName)) {
				for (auto &it : d.asDict()) {
					if (it.first == "host") {
						host = it.second.getString();
					} else if (it.first == "dbname") {
						dbname = it.second.getString();
					} else if (it.first == "user") {
						user = it.second.getString();
					} else if (it.first == "password") {
						password = it.second.getString();
					} else if (it.first == "role") {
						if (it.second.isString()) {
							if (it.second.getString() == "admin") {
								role = db::AccessRoleId::Admin;
							} else if (it.second.getString() == "nobody") {
								role = db::AccessRoleId::Nobody;
							} else if (it.second.getString() == "authorized") {
								role = db::AccessRoleId::Authorized;
							} else if (it.second.getString() == "system") {
								role = db::AccessRoleId::System;
							}
						} else {
							role = db::AccessRoleId(it.second.getInteger());
						}
					}
				}
			}
		}

		Value encode() const {
			Value ret;
			ret.setString(driver, "driver");
			ret.setString(host, "host");
			ret.setString(dbname, "dbname");
			ret.setString(user, "user");
			ret.setString(password, "password");
			ret.setInteger(toInt(role), "role");
			return ret;
		}
	};

	PqTest() : MemPoolTest("PqTest") { }

	virtual bool run(pool_t *pool) {
		StringStream stream;
		stream << "\n";

		// open db driver interface
		// driver sqlite3 build with static library
		// for pqsql, you can manually specify path to libpq with pqsql:<path>
		auto driver = db::sql::Driver::open(pool, "pgsql");
		if (!driver) {
			return false;
		}

		Options opts;

		auto conf = filesystem::currentDir<Interface>(".db.conf.json");
		if (filesystem::exists(conf)) {
			opts.loadFromFile(conf);
		}

		Value params = opts.encode();

		filesystem::mkdir_recursive(".db", false);

		Rc<db::test::detail::ServerScheme> test;
		db::test::detail::Server server(params, [&] (memory::pool_t *pool) -> Rc<db::test::detail::ServerScheme> {
			test = Rc<db::test::detail::ServerScheme>::alloc(pool, 0);
			return test;
		}, opts.role);

		size_t count = 0;
		size_t passed = 0;

/*
		bool runAccessTest(const db::Transaction &t);
		bool runCompressionTest(const db::Transaction &t);
		bool runDeltaTest(Server &server);
		bool runRelationTest(Server &server);
		bool runVirtualTest(const db::Transaction &t);*/

		runTest(stream, "AccessTest", count, passed, [&] () -> bool {
			auto ret = true;
			server.perform([&] (const db::Transaction &t) {
				t.setRole(db::AccessRoleId::System);
				ret &= test->runAccessTest(t, db::AccessRoleId::System);
				return true;
			});
			server.perform([&] (const db::Transaction &t) {
				t.setRole(db::AccessRoleId::Nobody);
				ret &= test->runAccessTest(t, db::AccessRoleId::Nobody);
				return true;
			});
			server.perform([&] (const db::Transaction &t) {
				t.setRole(db::AccessRoleId::Authorized);
				ret &= test->runAccessTest(t, db::AccessRoleId::Authorized);
				return true;
			});
			server.perform([&] (const db::Transaction &t) {
				t.setRole(db::AccessRoleId::Admin);
				ret &= test->runAccessTest(t, db::AccessRoleId::Admin);
				return true;
			});
			return ret;
		});

		runTest(stream, "CompressionTest", count, passed, [&] () -> bool {
			auto ret = true;
			server.perform([&] (const db::Transaction &t) {
				ret &= test->runCompressionTest(t);
				return true;
			});
			return ret;
		});

		runTest(stream, "DeltaTest", count, passed, [&] () -> bool {
			auto ret = true;
			ret &= test->runDeltaTest(server);
			return ret;
		});

		runTest(stream, "RelationTest", count, passed, [&] () -> bool {
			auto ret = true;
			ret &= test->runRelationTest(server);
			return ret;
		});

		runTest(stream, "VirtualTest", count, passed, [&] () -> bool {
			auto ret = true;
			server.perform([&] (const db::Transaction &t) {
				ret &= test->runVirtualTest(t);
				return true;
			});
			return ret;
		});

		test = nullptr;

		_desc = stream.str();

		return count == passed;
	}
} _PqTest;

}

#endif
