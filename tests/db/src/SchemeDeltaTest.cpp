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

#include "SchemeDeltaTest.h"

namespace stappler::dbtest {

SchemeDeltaTest::~SchemeDeltaTest() { }

SchemeDeltaTest::SchemeDeltaTest(memory::pool_t *p) : ServerScheme(p) {
	using namespace db;

	_deltaTest.define({
		Field::Text("name", MinLength(3)),
	});
}

void SchemeDeltaTest::fillSchemes(db::Map<StringView, const db::Scheme *> &schemes) {
	ServerScheme::fillSchemes(schemes);

	schemes.emplace(_deltaTest.getName(), &_deltaTest);
}

void SchemeDeltaTest::runTestGlobal(Server &server) {
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
}

int64_t SchemeDeltaTest::runTest(const db::Transaction &t, int64_t id) {
	using namespace db;
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
}

void SchemeDeltaTest::checkTest(const db::Transaction &t, Time time) {
	using namespace db;
	t.perform([&] {
		QueryList list(&_deltaTest);
		list.setDelta(time);
		list.setAll();
		list.resolve(Vector<String>{"name"});

		auto objs = t.performQueryList(list);
		std::cout << data::EncodeFormat::Pretty << objs << "\n";
		return objs.size() == 4 && objs.getValue(3).getValue("__delta").getString("action") == "delete";
	});
}

}
