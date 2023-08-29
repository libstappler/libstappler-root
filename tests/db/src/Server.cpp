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
#include "Server.h"
#include "STFieldTextArray.h"

namespace stappler::dbtest {

ServerScheme::ServerScheme(memory::pool_t *p) : _pool(p) {
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
}

void ServerScheme::fillSchemes(db::Map<StringView, const db::Scheme *> &schemes) {
	schemes.emplace(_users.getName(), &_users);
	schemes.emplace(_files.getName(), &_files);
	schemes.emplace(_errors.getName(), &_errors);
}

Server::~Server() {
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
