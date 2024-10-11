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

#include "SPWebHostComponent.h"
#include "SPValid.h"
#include "SPDbFieldExtensions.h"
#include "SPWebRequestHandler.h"
#include "SPWebInputFilter.h"

namespace STAPPLER_VERSIONIZED stappler::web {

class TestHandlerMapVariant1 : public RequestHandlerMap::Handler {
public:
	virtual bool isPermitted() override { return true; }

	virtual Value onData() override {
		return Value({
			pair("params", Value(_params)),
			pair("query", Value(_queryFields)),
			pair("input", Value(_inputFields)),
			pair("isCompleted", Value(_filter->isCompleted())),
			pair("bodyLength", Value(_filter->getBody().size())),
			pair("contentLength", Value(_filter->getContentLength())),
			pair("bytesRead", Value(_filter->getBytesRead())),
			pair("bytesReadSinceUpdate", Value(_filter->getBytesReadSinceUpdate())),
			pair("startTime", Value(_filter->getStartTime().toMicros())),
			pair("elapsedTime", Value(_filter->getElapsedTime().toMicros())),
			pair("elapsedTimeSinceUpdate", Value(_filter->getElapsedTimeSinceUpdate().toMicros()))
		});
	}
};

class TestHandlerMapVariant2 : public RequestHandlerMap::Handler {
public:
	virtual bool isPermitted() override { return true; }

	virtual Value onData() override {
		Root::dumpCurrentState();
		return Value({
			pair("input", Value(_inputFields)),
		});
	}
};

class TestHandlerMapVariant3 : public RequestHandlerMap::Handler {
public:
	virtual bool isPermitted() override { return true; }

	virtual Value onData() override {
		return Value({
			pair("body", Value(_filter->getBody().str())),
		});
	}
};

class TestHandlerMap : public RequestHandlerMap {
public:
	TestHandlerMap() {
		addHandler("Variant1", RequestMethod::Get, "/:id/:page", Handler::Make<TestHandlerMapVariant1>())
				.addQueryFields({
			db::Field::Integer("intValue", db::Flags::Required),
			db::Field::Integer("mtime", db::Flags::AutoMTime),
		});

		addHandler("Variant1Post", RequestMethod::Post, "/:id/:page", Handler::Make<TestHandlerMapVariant1>())
				.addQueryFields({
			db::Field::Integer("intValue", db::Flags::Required),
			db::Field::Integer("mtime", db::Flags::AutoMTime),
		}).addInputFields({
			db::Field::Text("text", db::Flags::Required),
			db::Field::File("file", db::MaxFileSize(2_MiB)),
			db::Field::Data("data"),
		}).setInputConfig(db::InputConfig{
			db::InputConfig::Require::Body | db::InputConfig::Require::Data | db::InputConfig::Require::Files | db::InputConfig::Require::FilesAsData,
			2_MiB,
			2_MiB,
			2_MiB,
		});

		addHandler("Variant2Post", RequestMethod::Post, "/urlencoded", Handler::Make<TestHandlerMapVariant2>())
				.addInputFields({
			db::Field::Text("text", db::Flags::Required),
			db::Field::Data("dict"),
			db::Field::Data("arr"),
			db::Field::Data("array space"),
		}).setInputConfig(db::InputConfig{
			db::InputConfig::Require::Data,
			2_MiB,
			2_MiB,
			2_MiB,
		});
		addHandler("Variant3Post", RequestMethod::Post, "/files", Handler::Make<TestHandlerMapVariant3>())
				.setInputConfig(db::InputConfig{
			db::InputConfig::Require::Files | db::InputConfig::Require::Body,
			2_MiB,
			2_MiB,
			2_MiB,
		});
	}
};

class TestHandler : public HostComponent {
public:
	TestHandler(const Host &serv, const HostComponentInfo &);
	virtual ~TestHandler() { }

	virtual void handleChildInit(const Host &) override;

	virtual void handleStorageInit(const Host &, const db::Adapter &) override;

	virtual void initTransaction(db::Transaction &) override;
	// virtual void handleHeartbeat(const VirtualServer &) override;

protected:
	Scheme _objects = Scheme("objects", Scheme::WithDelta);
	Scheme _refs = Scheme("refs", Scheme::WithDelta);
	Scheme _subobjects = Scheme("subobjects", Scheme::WithDelta);
	Scheme _images = Scheme("images");
	Scheme _test = Scheme("test");
	Scheme _hierarchy = Scheme("hierarchy");
	Scheme _pages = Scheme("pages");
	Scheme _detached = Scheme("detached", Scheme::Detouched);
	search::Configuration _search = search::Configuration(search::Language::Simple);
};

TestHandler::TestHandler(const Host &serv, const HostComponentInfo &info)
: HostComponent(serv, info) {
	exportValues(_objects, _refs, _subobjects, _images, _test, _detached, _hierarchy, _pages);

	using namespace db;

	_hierarchy.define(Vector<Field>({
		Field::Text("name", MinLength(3)),
		Field::Integer("id", Flags::Indexed | Flags::Unique),
		Field::Object("root", _hierarchy, Linkage::Manual, ForeignLink("sections")),
		Field::Set("sections", _hierarchy, Linkage::Manual, ForeignLink("root")),

		Field::View("pages", _pages, ViewFn([] (const Scheme &, const Value &obj) -> bool {
			return obj.getBool("hidden") ? false : true;
		}), Vector<String>({ "hidden" })),

		Field::Set("all_pages", _pages)
	}));

	_pages.define(Vector<Field>({
		Field::Text("name", MinLength(3)),
		Field::Boolean("hidden"),
		Field::Object("root", _hierarchy),
	}));

	_objects.define(Vector<Field>{
		Field::Text("text", MinLength(3), Flags::Indexed),
		Field::Extra("data", Vector<Field>{
			Field::Array("strings", Field::Text("")),
		}),
		Field::Set("subobjects", _subobjects),
		Field::File("file", MaxFileSize(1_MiB)),
		Field::Text("alias", Transform::Alias),
		Field::Integer("mtime", Flags::AutoMTime | Flags::Indexed),
		Field::Integer("index", Flags::Indexed),
		Field::View("refs", _refs, ViewFn([] (const Scheme &objScheme, const Value &obj) -> bool {
			return true;
		}), FieldView::Delta),

		Field::Array("array", Field::Extra("", Vector<Field>{
			Field::Integer("one"),
			Field::Integer("two"),
		})),

		Field::Extra("textFile", Vector<Field>{
			Field::Text("type"),
			Field::Integer("mtime"),
			Field::Text("content"),
		}),
		Field::Extra("binaryFile", Vector<Field>{
			Field::Text("type"),
			Field::Integer("mtime"),
			Field::Bytes("content"),
		}),

		Field::Set("images", _images, Flags::Composed),
	},
	AccessRole::Admin(AccessRoleId::Authorized));

	_refs.define({
		Field::Text("alias", Transform::Alias),
		Field::Text("text", MinLength(3)),
		Field::Set("features", _objects, RemovePolicy::StrongReference), // objects, that will be removed when ref is removed
		Field::Set("optionals", _objects, RemovePolicy::Reference),
		Field::Integer("mtime", Flags::AutoMTime | Flags::Indexed),
		Field::Integer("index", Flags::Indexed),
		Field::File("file", MaxFileSize(100_KiB)),
		Field::Data("array", Transform::Array),
		Field::Object("objectRef", _objects, Flags::Reference),
		Field::Object("subobject", _subobjects, Flags::Reference),

		Field::Extra("extra", Vector<Field>{
			Field::Integer("one"),
			Field::Integer("two"),
		}),

		Field::Image("cover", MaxImageSize(1080, 1080, ImagePolicy::Resize), Vector<Thumbnail>{
			Thumbnail("thumb", 160, 160),
			Thumbnail("cover512", 512, 512),
			Thumbnail("cover256", 256, 256),
			Thumbnail("cover128", 128, 128),
			Thumbnail("cover64", 64, 64),
		}),

		Field::Data("data")
	});

	_subobjects.define({
		Field::Text("text", MinLength(3)),
		Field::Object("object", _objects, RemovePolicy::Cascade),
		Field::Integer("mtime", Flags::AutoMTime | Flags::Indexed),
		Field::Integer("index", Flags::Indexed),
	});

	_images.define(Vector<Field>{
		Field::Integer("ctime", Flags::ReadOnly | Flags::AutoCTime | Flags::ForceInclude),
		Field::Integer("mtime", Flags::ReadOnly | Flags::AutoMTime | Flags::ForceInclude),

		Field::Text("name", Transform::Identifier, Flags::Required | Flags::Indexed | Flags::ForceInclude),

		Field::Image("content", MaxImageSize(2048, 2048, ImagePolicy::Resize), Vector<Thumbnail>{
			Thumbnail("thumb", 380, 380)
		}),
	},
		AccessRole::Admin(AccessRoleId::Authorized)
	);

	_test.define({
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

		Field::Extra("tsvData", Vector<Field>{
			Field::Text("text", MaxLength(1_KiB)),
			Field::Text("html", MaxLength(1_KiB)),
			Field::Data("words"),
		}, AutoFieldDef{
			Vector<AutoFieldScheme>({
				AutoFieldScheme{ _test, {"text", "key"} }
			}),
			DefaultFn([] (const Value &data) -> Value {
				StringStream html;
				StringStream text;
				text << data.getString("key") << " ";
				html << "<html><body><h1>" <<  data.getString("key") << "</h1>";
				for (auto &it : data.getArray("text")) {
					text << it.getString() << " ";
					html << "<p>" <<  it.getString() << "</p>";
				}
				html << "</body></html>";

				auto textdata = StringView(text.weak());
				textdata.trimChars<StringView::WhiteSpace>();

				Value ret;
				ret.setString(textdata, "text");
				ret.setString(html.str(), "html");
				auto &words = ret.emplace("words");

				textdata.split<StringView::WhiteSpace>([&] (StringView word) {
					words.addString(word);
				});

				return ret;
			}),
			Vector<String>({"text", "key"}),
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
		}), */ _search, Vector<String>({"key", "text"})),
	});

	_detached.define({
		Field::Integer("time", Flags::Indexed),
		Field::Text("text", MinLength(3)),
		Field::Array("textArray", Type::Text),
		Field::Array("integerArray", Type::Integer, Flags::Unique),
		Field::Object("object", _objects, Flags::Reference),
		Field::Object("strong", _objects, RemovePolicy::StrongReference),
	});
}

void TestHandler::handleChildInit(const Host &serv) {
	serv.addResourceHandler("/objects_root/", _objects, Value("root"));
	serv.addResourceHandler("/objects_indexed/", _objects, Value(1));
	serv.addResourceHandler("/objects_first/", _objects, Value({
		pair("index", Value(10))
	}));
	serv.addResourceHandler("/objects_named/", _objects, Value({
		pair("text", Value("text1"))
	}));
	serv.addResourceHandler("/objects/", _objects);
	serv.addResourceHandler("/refs/", _refs);
	serv.addResourceHandler("/test/", _test);

	serv.addResourceHandler("/categories/", _hierarchy);
	serv.addResourceHandler("/pages/", _pages);
	serv.addResourceHandler("/users/", *serv.getUserScheme());

	serv.addMultiResourceHandler("/multi", {
		pair("objects", &_objects),
		pair("refs", &_refs),
		pair("subobjects", &_subobjects),
	});

	/*serv.addHandler("/handler", SA_HANDLER(TestSelectHandler));
	serv.addHandler("/pug/", SA_HANDLER(TestPugHandler));
	serv.addHandler("/upload/", SA_HANDLER(TestUploadHandler));*/

	serv.addHandler("/map/", new TestHandlerMap);

	addOutputCommand("test", [&, this] (StringView str, const Callback<void(const Value &)> &cb) -> bool {
		if (auto t = db::Transaction::acquireIfExists()) {
			cb(_test.create(t, Value({
				Value({
					pair("time", Value(Time::now().toMicros())),
					pair("key", Value(valid::generatePassword<Interface>(6)))
				}),
				Value({
					pair("time", Value(Time::now().toMicros())),
					pair("key", Value(valid::generatePassword<Interface>(6)))
				})
			})));
		}
		return true;
	}, " - test");
}

void TestHandler::handleStorageInit(const Host &, const db::Adapter &a) {
	auto t = db::Transaction::acquire(a);

	auto obj = Value({
		pair("text", Value("text1")),
		pair("alias", Value("root")),
		pair("index", Value(10)),
		pair("strings", Value({ Value("String1"), Value("String2") })),
		pair("array", Value({
			Value({
				pair("one", Value(1)),
				pair("two", Value(2)),
			}),
			Value({
				pair("one", Value(3)),
				pair("two", Value(4)),
			})
		})),
	});

	_objects.create(t, obj);

	for (size_t i = 0; i < 30; ++ i) {
		Value data {
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

		_test.create(t,  data);
	}

	auto cat = _hierarchy.create(t, Value({
		pair("name", Value("TestCategory")),
		pair("id", Value(10)),
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

	t.release();
}

void TestHandler::initTransaction(db::Transaction &t) {
	t.setRole(db::AccessRoleId::Authorized);
}

extern "C" HostComponent * CreateTestComponent(const Host &serv, const HostComponentInfo &info) {
	return new TestHandler(serv, info);
}

}
