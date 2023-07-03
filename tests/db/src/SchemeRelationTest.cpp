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

#include "SchemeRelationTest.h"

namespace stappler::dbtest {

SchemeRelationTest::~SchemeRelationTest() { }

SchemeRelationTest::SchemeRelationTest(memory::pool_t *p, uint32_t version) : ServerScheme(p) {
	using namespace db;

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
}

void SchemeRelationTest::fillSchemes(db::Map<StringView, const db::Scheme *> &schemes) {
	ServerScheme::fillSchemes(schemes);

	schemes.emplace(_objects.getName(), &_objects);
	schemes.emplace(_refs.getName(), &_refs);
	schemes.emplace(_subobjects.getName(), &_subobjects);
	schemes.emplace(_images.getName(), &_images);
	schemes.emplace(_hierarchy.getName(), &_hierarchy);
	schemes.emplace(_pages.getName(), &_pages);
}

void SchemeRelationTest::fillTest(const db::Transaction &t, int64_t id) {
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
}

bool SchemeRelationTest::checkTest(const db::Transaction &t, int64_t id) {
	using namespace db;

	return t.performAsSystem([&] () {
		auto cat = _hierarchy.select(t, Query().select("id", Value(id))).getValue(0);
		if (!cat) {
			return false;
		}

		auto all = _hierarchy.getProperty(t, cat, "all_pages");

		auto pages = _hierarchy.getProperty(t, cat, "pages");

		return pages.size() == 2 && all.size() == 4;
	});
}

}
