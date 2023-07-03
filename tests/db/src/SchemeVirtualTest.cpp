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

#include "SchemeVirtualTest.h"
#include "SPFilesystem.h"

namespace stappler::dbtest {

SchemeVirtualTest::~SchemeVirtualTest() { }

SchemeVirtualTest::SchemeVirtualTest(memory::pool_t *p, uint32_t version) : ServerScheme(p) {
	using namespace db;

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

void SchemeVirtualTest::fillSchemes(db::Map<StringView, const db::Scheme *> &schemes) {
	ServerScheme::fillSchemes(schemes);

	schemes.emplace(_virtualTest.getName(), &_virtualTest);
}

void SchemeVirtualTest::runTest(const db::Transaction &t) {
	using namespace db;

	t.performAsSystem([&] {
		auto data = _virtualTest.create(t, Value({
			pair("name", Value(toString("ValueName.", Time::now().toMicros())))
		}));

		std::cout << data::EncodeFormat::Pretty << data << "\n";

		auto data1 = _virtualTest.get(t, data.getInteger("__oid"));

		auto tmp = data;
		tmp.erase("__oid");

		auto tmp1 = data1.getValue("computed");
		tmp1.erase("time");
		tmp1.erase("path");

		_virtualTest.update(t, data.getInteger("__oid"), Value({
			pair("virtual",  data1.getValue("computed"))
		}));

		if (tmp1 == tmp) {
			std::cout << data::EncodeFormat::Pretty << data1 << "\n";
		}

		auto virtualTest = _virtualTest.get(t, data.getInteger("__oid"), {"virtual"});

		std::cout << data::EncodeFormat::Pretty << virtualTest << "\n";

		_virtualTest.remove(t, data.getInteger("__oid"));

		return true;
	});
}

}
