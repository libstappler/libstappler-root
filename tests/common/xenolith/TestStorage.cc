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

#include "TestStorage.h"
#include "SPValid.h"
#include "SPDbFieldExtensions.h"

namespace STAPPLER_VERSIONIZED stappler::xenolith::app {

class StorageTestComponent : public storage::Component {
public:
	static constexpr auto DbPasswordSalt = "UtilsStorageTestComponent";

	virtual ~StorageTestComponent() { }

	StorageTestComponent(storage::ComponentLoader &loader)
	: Component(loader, "UtilsStorageTest") {
		using namespace db;

		loader.getServer();
		auto app = loader.getTransaction().getAdapter().getApplicationInterface();
		app->getUserScheme();
		app->getFileScheme();
		app->getDocumentRoot();
		app->pushDebugMessage(db::Value());
		app->pushErrorMessage(db::Value());

		loader.exportScheme(_users.define({
			Field::Text("name", MinLength(2), MaxLength(32), Transform::Identifier, Flags::Indexed),
			Field::Password("password", MinLength(2), MaxLength(32), PasswordSalt(DbPasswordSalt))
		}));

		loader.exportScheme(_objects.define({
			Field::Text("key", Transform::Alias),
			Field::Integer("index", Flags::Indexed, Flags::Unique),
			Field::Float("value", Flags::Indexed),
			Field::Boolean("flag", Flags::Indexed),
			Field::Integer("time", Flags::Indexed | Flags::AutoMTime),
			Field::Bytes("secret"),
			Field::Data("data"),
			Field::Custom(new FieldBigIntArray("clusters")),
			Field::Custom(new FieldIntArray("refs")),
			Field::Custom(new FieldTextArray("text")),
			Field::Custom(new FieldPoint("coords")),

			Field::Extra("tsvData", db::Vector<Field>{
				Field::Text("text", MaxLength(1_KiB)),
				Field::Text("html", MaxLength(1_KiB)),
				Field::Data("words"),
			}, AutoFieldDef{
				db::Vector<AutoFieldScheme>({
					AutoFieldScheme{ _objects, {"text", "key"} }
				}),
				DefaultFn([] (const db::Value &data) -> db::Value {
					db::StringStream html;
					db::StringStream text;
					text << data.getString("key") << " ";
					html << "<html><body><h1>" <<  data.getString("key") << "</h1>";
					for (auto &it : data.getArray("text")) {
						text << it.getString() << " ";
						html << "<p>" <<  it.getString() << "</p>";
					}
					html << "</body></html>";

					auto textdata = StringView(text.weak());
					textdata.trimChars<StringView::WhiteSpace>();

					db::Value ret;
					ret.setString(textdata, "text");
					ret.setString(html.str(), "html");
					auto &words = ret.emplace("words");

					textdata.split<StringView::WhiteSpace>([&] (StringView word) {
						words.addString(word);
					});

					return ret;
				}),
				db::Vector<db::String>({"text", "key"}),
			}),

			Field::FullTextView("tsv", db::FullTextViewFn([this] (const db::Scheme &scheme, const db::Value &obj) -> db::FullTextVector {
				size_t count = 0;
				db::FullTextVector vec;

				count = _search.makeSearchVector(vec, obj.getString("key"), db::FullTextRank::A, count);
				for (auto &it : obj.getArray("text")) {
					count = _search.makeSearchVector(vec, it.getString(), db::FullTextRank::B, count);
				}

				return vec;
			}), /* db::FullTextQueryFn([this] (const db::Value &data) -> db::FullTextQuery {
				return _search.parseQuery(data.getString());
			}), */ _search, db::Vector<db::String>({"key", "text"})),
		}));
	}

	virtual void handleChildInit(const storage::Server &serv, const db::Transaction &t) override {
		using namespace db;

		auto objs = _objects.select(t, db::Query().include("index"));

		for (auto &it : objs.asArray()) {
			_objects.remove(t, it.getInteger("__oid"));
		}

		for (size_t i = 0; i < 30; ++ i) {
			db::Value data {
				pair("key", db::Value(db::toString("key", i))),
				pair("index", db::Value(10 + i)),
				pair("value", db::Value(1.5 + i)),
				pair("flag", db::Value(i < 15)),
				pair("secret", db::Value(valid::makeRandomBytes<db::Interface>(12))),
				pair("clusters", db::Value{
					db::Value(i * 1000 + 100), db::Value(-i * 1000 - 200)
				}),
				pair("refs", db::Value{
					db::Value(i * 100000 + 1000), db::Value(-i * 100000 - 2000)
				}),
				pair("text", db::Value{
					db::Value(db::toString("key", i)),
					db::Value(db::toString("value", i)),
					db::Value(db::toString("word", i, " ", "sub", i))
				}),
				pair("coords", db::Value{
					db::Value(0.5 + i), db::Value(db::toString(-0.5 - i))
				}),
				pair("data", db::Value{
					pair("key", db::Value(db::toString("key", i))),
					pair("index", db::Value(10 + i)),
					pair("value", db::Value(1.5 + i)),
				})
			};

			_objects.create(t,  data);
		}

		Component::handleChildInit(serv, t);
	}
	virtual void handleChildRelease(const storage::Server &serv, const db::Transaction &t) override {
		std::cout << "handleChildRelease\n";
		Component::handleChildRelease(serv, t);
	}

	virtual void handleStorageTransaction(db::Transaction &t) override {
		std::cout << "handleStorageTransaction\n";
		Component::handleStorageTransaction(t);
	}
	virtual void handleHeartbeat(const storage::Server &serv) override {
		Component::handleHeartbeat(serv);
	}

	const db::Scheme &getUsers() const { return _users; }
	const db::Scheme &getObjects() const { return _objects; }

protected:
	db::Scheme _users = db::Scheme("test_users");
	db::Scheme _objects = db::Scheme("test_objects");
	search::Configuration _search = search::Configuration(search::Language::Simple);
};

bool StorageTestComponentContainer::init() {
	return ComponentContainer::init("UtilsStorageTest");
}

void StorageTestComponentContainer::handleStorageInit(storage::ComponentLoader &loader) {
	std::cout << "handleStorageInit\n";
	ComponentContainer::handleStorageInit(loader);
	_component = new StorageTestComponent(loader);
}
void StorageTestComponentContainer::handleStorageDisposed(const db::Transaction &t) {
	_component = nullptr;
	ComponentContainer::handleStorageDisposed(t);
	std::cout << "handleStorageDisposed\n";
}

void StorageTestComponentContainer::handleComponentsLoaded(const storage::Server &serv) {
	ComponentContainer::handleComponentsLoaded(serv);

	_server->select(_component->getObjects(), [this] (const Value &objects) {
		onObjects(objects);
	});
	_server->select(_component->getObjects(), [this] (const Value &objects) {
		onIds(objects);
	}, [] (db::Query &q) {
		q.include("index", "key");
	});

	_server->count(_component->getObjects(), [] (size_t count) {
		log::debug("StorageTestComponentContainer", toString("Count: ", count));
	});
	_server->count(_component->getObjects(), [] (size_t count) {
		log::debug("StorageTestComponentContainer", toString("Count: ", count));
	}, [] (db::Query &q) {

	});

	_server->set("StorageTestComponentContainer", Value({
		pair("key", Value("StorageTestComponentContainer"))
	}));
	_server->set("StorageTestComponentContainer", Value({
		pair("key", Value("StorageTestComponentContainer"))
	}), [this] (const Value &val) {
		_server->get("StorageTestComponentContainer", [] (const Value &val) { });
		_server->clear("StorageTestComponentContainer", [] (const Value &) { });
		_server->clear("StorageTestComponentContainer");
	});
}

void StorageTestComponentContainer::handleComponentsUnloaded(const storage::Server &serv) {
	std::cout << "handleComponentsUnloaded\n";
	ComponentContainer::handleComponentsUnloaded(serv);
}

bool StorageTestComponentContainer::getAll(Function<void(Value &&)> &&cb, Ref *ref) {
	return perform([this, cb = move(cb), ref] (const storage::Server &serv, const db::Transaction &t) mutable {
		db::Value val;
		auto users = _component->getUsers().select(t, db::Query());
		for (auto &it : users.asArray()) {
			val.addString(it.getString("name"));
		}

		serv.getApplication()->performOnMainThread([cb = move(cb), val = Value(val)] () mutable {
			cb(move(val));
		}, ref);

		return true;
	}, ref);
}

bool StorageTestComponentContainer::createUser(StringView name, StringView password, Function<void(Value &&)> &&cb, Ref *ref) {
	return perform([this, cb = move(cb), name = name.str<Interface>(), password = password.str<Interface>(), ref] (const storage::Server &serv, const db::Transaction &t) mutable {
		db::Value val;
		auto u = _component->getUsers().select(t, db::Query().select("name", db::Value(name))).getValue(0);
		if (u) {
			val = _component->getUsers().update(t, u, db::Value({
				pair("password", db::Value(password)),
			}));
		} else {
			val = _component->getUsers().create(t, db::Value({
				pair("name", db::Value(name)),
				pair("password", db::Value(password)),
			}));
		}

		serv.getApplication()->performOnMainThread([cb = move(cb), val = Value(val)] () mutable {
			cb(move(val));
		}, ref);

		return true;
	}, ref);
}

bool StorageTestComponentContainer::checkUser(StringView name, StringView password, Function<void(Value &&)> &&cb, Ref *ref) {
	return perform([this, cb = move(cb), name = name.str<Interface>(), password = password.str<Interface>(), ref] (const storage::Server &serv, const db::Transaction &t) mutable {
		db::Value val;
		auto u = _component->getUsers().select(t, db::Query().select("name", db::Value(name))).getValue(0);
		if (u) {
			if (!valid::validatePassord(password, u.getBytes("password"), StorageTestComponent::DbPasswordSalt)) {
				val = db::Value("invalid_password");
			} else {
				val = move(u);
			}
		}

		serv.getApplication()->performOnMainThread([cb = move(cb), val = Value(val)] () mutable {
			cb(move(val));
		}, ref);

		return true;
	}, ref);
}

void StorageTestComponentContainer::onObjects(const Value &val) {

}

void StorageTestComponentContainer::onIds(const Value &val) {
	auto makeValue = [] (size_t i) {
		return Value {
			pair("key", Value(toString("key", i))),
			pair("index", Value(10 + i)),
			pair("value", Value(1.5 + i)),
			pair("flag", Value(i < 15)),
			pair("secret", Value(valid::makeRandomBytes<Interface>(12))),
			pair("clusters", Value{
				Value(i * 1000 + 100), Value(-i * 1000 - 200)
			}),
			pair("refs", Value{
				Value(i * 100000 + 1000), Value(-i * 100000 - 2000)
			}),
			pair("text", Value{
				Value(toString("key", i)),
				Value(toString("value", i)),
				Value(toString("word", i, " ", "sub", i))
			}),
			pair("coords", Value{
				Value(0.5 + i), Value(toString(-0.5 - i))
			}),
			pair("data", Value{
				pair("key", Value(toString("key", i))),
				pair("index", Value(10 + i)),
				pair("value", Value(1.5 + i)),
			})
		};
	};

	auto &objs = _component->getObjects();

	auto obj = val.getValue(0);
	auto obj2 = val.getValue(2);
	auto id = val.getValue(0).getInteger("__oid");
	auto id2 = val.getValue(3).getInteger("__oid");
	auto id3 = val.getValue(4).getInteger("__oid");
	auto key = val.getValue(0).getString("key");

	_server->get(objs, [] (const Value &) { }, id);
	_server->get(objs, [] (const Value &) { }, key);
	_server->get(objs, [] (const Value &) { }, Value(id));
	_server->get(objs, [] (const Value &) { }, Value(toString(id)));
	_server->get(objs, [] (const Value &) { }, Value(key));
	_server->get(objs, [] (const Value &) { }, obj);

	_server->get(objs, [] (const Value &) { }, id, StringView("key"));
	_server->get(objs, [] (const Value &) { }, key, StringView("key"));
	_server->get(objs, [] (const Value &) { }, Value(id), StringView("key"));
	_server->get(objs, [] (const Value &) { }, Value(toString(id)), StringView("key"));
	_server->get(objs, [] (const Value &) { }, Value(key), StringView("key"));
	_server->get(objs, [] (const Value &) { }, obj, StringView("key"));

	_server->get(objs, [] (const Value &) { }, id, { StringView("key") });
	_server->get(objs, [] (const Value &) { }, key, { StringView("key") });
	_server->get(objs, [] (const Value &) { }, Value(id), { StringView("key") });
	_server->get(objs, [] (const Value &) { }, Value(toString(id)), { StringView("key") });
	_server->get(objs, [] (const Value &) { }, Value(key), { StringView("key") });
	_server->get(objs, [] (const Value &) { }, obj, { StringView("key") });

	_server->get(objs, [] (const Value &) { }, id, { "key" });
	_server->get(objs, [] (const Value &) { }, key, { "key" });
	_server->get(objs, [] (const Value &) { }, Value(id), { "key" });
	_server->get(objs, [] (const Value &) { }, Value(toString(id)), { "key" });
	_server->get(objs, [] (const Value &) { }, Value(key), { "key" });
	_server->get(objs, [] (const Value &) { }, obj, { "key" });

	_server->get(objs, [] (const Value &) { }, id, { objs.getField("key") });
	_server->get(objs, [] (const Value &) { }, key, { objs.getField("key") });
	_server->get(objs, [] (const Value &) { }, Value(id), { objs.getField("key") });
	_server->get(objs, [] (const Value &) { }, Value(toString(id)), { objs.getField("key") });
	_server->get(objs, [] (const Value &) { }, Value(key), { objs.getField("key") });
	_server->get(objs, [] (const Value &) { }, obj, { objs.getField("key") });

	_server->create(objs, makeValue(100));
	_server->create(objs, makeValue(101), [] (const Value &) { });
	_server->create(objs, makeValue(102), [] (const Value &) { }, db::Conflict::DoNothing);
	_server->create(objs, makeValue(103), [] (const Value &) { }, db::UpdateFlags::None, db::Conflict::DoNothing);

	_server->touch(objs, id);
	_server->touch(objs, obj);

	_server->update(objs, id, makeValue(104));
	_server->update(objs, id, makeValue(105), [] (const Value &) { });
	_server->update(objs, obj, makeValue(106));
	_server->update(objs, obj, makeValue(107), [] (const Value &) { });

	_server->remove(objs, id2);
	_server->remove(objs, id3, [] (bool) {

	});
	_server->remove(objs, obj2, [] (bool) {

	});

}

}
