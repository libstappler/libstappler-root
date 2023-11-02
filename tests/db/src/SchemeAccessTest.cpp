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

#include "SchemeAccessTest.h"
#include "SPFilesystem.h"
#include "STPqHandle.h"

namespace stappler::dbtest {

SchemeAccessTest::~SchemeAccessTest() { }

SchemeAccessTest::SchemeAccessTest(memory::pool_t *p) : ServerScheme(p) {
	using namespace db;

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
}

void SchemeAccessTest::fillSchemes(db::Map<StringView, const db::Scheme *> &schemes) {
	ServerScheme::fillSchemes(schemes);

	schemes.emplace(_accessTest.getName(), &_accessTest);
}

bool SchemeAccessTest::runTest(const db::Transaction &t) {
	using namespace db;
	auto v = _accessTest.select(t, Query());

	_accessTest.create(t, Value({
		pair("name", Value(toString("Access.", Time::now().toMicros()))),
		pair("role", Value(256))
	}));
	return true;
}

}
