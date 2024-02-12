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
#include "SPWebRequestHandler.h"
#include "SPValid.h"

namespace stappler::web::test {

class TestSelectHandler : public RequestHandler {
public:
	virtual bool isRequestPermitted(Request & rctx) override {
		return true;
	}

	virtual Status onTranslateName(Request &rctx) override {
		auto scheme = rctx.host().getScheme("refs");
		if (scheme) {
			Value d;
			rctx.performWithStorage([&, this] (const db::Transaction &t) {
				d = scheme->select(_transaction, db::Query::all());
				return true;
			});
			rctx.writeData(d);
			return DONE;
		}
		return HTTP_NOT_FOUND;
	}
};

class TestHandler : public HostComponent {
public:
	TestHandler(const Host &serv, const HostComponentInfo &info);
	virtual ~TestHandler() { }

	virtual void handleChildInit(const Host &) override;
	virtual void initTransaction(db::Transaction &) override;

protected:
	Scheme _objects = Scheme("objects");
	Scheme _refs = Scheme("refs");
	Scheme _subobjects = Scheme("subobjects");
	Scheme _images = Scheme("images");
	// Scheme _test = Scheme("test");
	Scheme _detached = Scheme("detached", Scheme::Detouched);
};

TestHandler::TestHandler(const Host &serv, const HostComponentInfo &info)
: HostComponent(serv, info) {
	exportValues(_objects, _refs, _subobjects, _images, _detached);

	using namespace db;

	_objects.define(Vector<Field>{
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
		Field::Array("array", Field::Text("", MaxLength(10))),
		Field::Object("objectRef", _objects, Flags::Reference),

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

	/*_test.define({
		Field::Text("key"),
		Field::Integer("time", Flags::Indexed),
		Field::Data("data"),
		Field::Custom(new FieldBigIntArray("clusters")),
		Field::Custom(new FieldIntArray("refs")), // PkkId
		Field::Custom(new FieldPoint("coords")),
	});*/

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
	serv.addResourceHandler("/objects/", _objects);
	serv.addResourceHandler("/refs/", _refs);

	serv.addMultiResourceHandler("/multi", {
		pair("objects", &_objects),
		pair("refs", &_refs),
		pair("subobjects", &_subobjects),
	});

	serv.addHandler("/handler", RequestHandler::Handler<TestSelectHandler>());

	addOutputCommand("test", [&] (StringView str, const Callback<void(const Value &)> &cb) -> bool {
		if (auto t = db::Transaction::acquireIfExists()) {
			cb(Value("test"));
		}
		return true;
	}, " - test");
}

void TestHandler::initTransaction(db::Transaction &t) {
	t.setRole(db::AccessRoleId::Authorized);
}

extern "C" HostComponent * CreateTestComponent(const Host &serv, const HostComponentInfo &info) {
	return new TestHandler(serv, info);
}

}
