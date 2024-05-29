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

#include "SPCommon.h"
#include "SPDbSimpleServer.h"

#include "ExampleDb.h"

namespace STAPPLER_VERSIONIZED stappler::app {

bool ExampleDb::run(const Value &val, StringView command) {
	db::Value data;
	if (val.isString("driver")) {
		data.setString(val.getString("driver"), "driver");
	} else {
		if (val.hasValue("host")) {
			data.setString("pgsql", "driver");
		} else {
			data.setString("sqlite", "driver");
		}
	}

	for (auto &it : val.asDict()) {
		if (it.first == "host") {
			data.setString(it.second.getString(), "host");
		} else if (it.first == "dbname") {
			data.setString(it.second.getString(), "dbname");
		} else if (it.first == "user") {
			data.setString(it.second.getString(), "user");
		} else if (it.first == "password") {
			data.setString(it.second.getString(), "password");
		}
	}

	auto path = filesystem::writablePath<Interface>();

	auto serv = Rc<db::SimpleServer>::create(data, path, db::AccessRoleId::Admin, SpanView<const db::Scheme *>());
	if (serv) {
		std::cout << "Connected!\n";
		return true;
	}

	return false;
}

}
