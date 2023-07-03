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
#include "SPData.h"
#include "SPMemory.h"
#include "SPTime.h"
#include "SPFilesystem.h"

#include "Server.h"
#include "SchemeUpdateTest.h"
#include "SchemeVirtualTest.h"
#include "SchemeRelationTest.h"
#include "SchemeDeltaTest.h"
#include "SchemeCompressionTest.h"
#include "SchemeAccessTest.h"

namespace stappler::dbtest {

using namespace mem_std;

static constexpr auto HELP_STRING(
R"HelpString(dataapp <options>
Options are one of:
    -v (--verbose)
    -h (--help)
	--user=<username>
	--password=<username>
	--dbname=<dbname>
	--admin - установить роль по умолчанию
	--nobody - установить роль по умолчанию
	--authorized - установить роль по умолчанию
	--system - установить роль по умолчанию)HelpString");

static int parseOptionSwitch(Value &ret, char c, const char *str) {
	if (c == 'h') {
		ret.setBool(true, "help");
	} else if (c == 'v') {
		ret.setBool(true, "verbose");
	}
	return 1;
}

static int parseOptionString(Value &ret, const StringView &str, int argc, const char * argv[]) {
	if (str == "help") {
		ret.setBool(true, "help");
	} else if (str == "verbose") {
		ret.setBool(true, "verbose");
	} else if (str == "admin") {
		ret.setString("admin", "role");
	} else if (str == "nobody") {
		ret.setString("nobody", "role");
	} else if (str == "authorized") {
		ret.setString("authorized", "role");
	} else if (str == "system") {
		ret.setString("system", "role");
	} else if (str.starts_with("data=")) {
		ret.setString(str.sub("data="_len), "data");
	} else if (str.starts_with("user=")) {
		ret.setString(str.sub("user="_len), "user");
	} else if (str.starts_with("password=")) {
		ret.setString(str.sub("password="_len), "password");
	} else if (str.starts_with("dbname=")) {
		ret.setString(str.sub("dbname="_len), "dbname");
	}
	return 1;
}

SP_EXTERN_C int _spMain(argc, argv) {
	Value opts = data::parseCommandLineOptions<Interface>(argc, argv,
			&parseOptionSwitch, &parseOptionString);
	if (opts.getBool("help")) {
		std::cout << HELP_STRING << "\n";
		return 0;
	}

	if (opts.getBool("verbose")) {
		std::cout << " Current work dir: " << stappler::filesystem::currentDir<Interface>() << "\n";
		std::cout << " Documents dir: " << stappler::filesystem::documentsPathReadOnly<Interface>() << "\n";
		std::cout << " Cache dir: " << stappler::filesystem::cachesPathReadOnly<Interface>() << "\n";
		std::cout << " Writable dir: " << stappler::filesystem::writablePathReadOnly<Interface>() << "\n";
		std::cout << " Options: " << stappler::data::EncodeFormat::Pretty << opts << "\n";
	}

	Value params;
	params.setString("pgsql", "driver");
	params.setString("localhost", "host");
	params.setString("dbtest", "dbname");
	params.setString("admin", "user");
	params.setString("admin", "password");

	if (opts.isString("user")) {
		params.setString(opts.getString("user"), "user");
	}
	if (opts.isString("password")) {
		params.setString(opts.getString("password"), "password");
	}
	if (opts.isString("dbname")) {
		params.setString(opts.getString("dbname"), "dbname");
	}

	filesystem::mkdir_recursive(".db", false);

	db::AccessRoleId roleId = db::AccessRoleId::Admin;
	if (opts.isString("role")) {
		auto role = opts.getString("role");
		if (role == "admin") {
			roleId = db::AccessRoleId::Admin;
		} else if (role == "nobody") {
			roleId = db::AccessRoleId::Nobody;
		} else if (role == "authorized") {
			roleId = db::AccessRoleId::Authorized;
		} else if (role == "system") {
			roleId = db::AccessRoleId::System;
		}
	}

	if (opts.getValue("args").size() > 1) {
		auto arg = StringView(opts.getValue("args").getString(1));
		if (StringView(arg).starts_with("update-test-")) {
			arg += "update-test-"_len;
			Server server(params, [&] (memory::pool_t *pool) -> Rc<ServerScheme> {
				return Rc<SchemeUpdateTest>::alloc(pool, arg.readInteger(10).get(0));
			}, roleId);
			return 0;
		} else if (arg == "virtual-test") {
			Rc<SchemeVirtualTest> test;
			Server server(params, [&] (memory::pool_t *pool) -> Rc<ServerScheme> {
				test = Rc<SchemeVirtualTest>::alloc(pool, arg.readInteger(10).get(0));
				return test;
			}, roleId);

			server.perform([&] (const db::Transaction &t) {
				test->runTest(t);
				return true;
			});
			return 0;
		} else if (arg == "relation-test-1") {
			Rc<SchemeRelationTest> test;
			Server server(params, [&] (memory::pool_t *pool) -> Rc<ServerScheme> {
				test = Rc<SchemeRelationTest>::alloc(pool, arg.readInteger(10).get(0));
				return test;
			}, roleId);

			int64_t id = int64_t(Time::now().toMicros());

			server.perform([&] (const db::Transaction &t) {
				test->fillTest(t, id);
				return true;
			});
			server.update();
			server.perform([&] (const db::Transaction &t) {
				test->checkTest(t, id);
				return true;
			});
			return 0;
		} else if (arg == "delta-test") {
			Rc<SchemeDeltaTest> test;
			Server server(params, [&] (memory::pool_t *pool) -> Rc<ServerScheme> {
				test = Rc<SchemeDeltaTest>::alloc(pool);
				return test;
			}, roleId);

			test->runTestGlobal(server);
			return 0;
		} else if (arg == "compression-test") {
			Rc<SchemeCompressionTest> test;
			Server server(params, [&] (memory::pool_t *pool) -> Rc<ServerScheme> {
				test = Rc<SchemeCompressionTest>::alloc(pool);
				return test;
			}, roleId);

			server.perform([&] (const db::Transaction &t) {
				test->runTest(t);
				return true;
			});
			return 0;
		} else if (arg == "access-test") {
			Rc<SchemeAccessTest> test;
			Server server(params, [&] (memory::pool_t *pool) -> Rc<ServerScheme> {
				test = Rc<SchemeAccessTest>::alloc(pool);
				return test;
			}, roleId);

			server.perform([&] (const db::Transaction &t) {
				test->runTest(t);
				return true;
			});
			return 0;
		}
	}

	Server server(params, [] (memory::pool_t *pool) -> Rc<ServerScheme> {
		return Rc<ServerScheme>::alloc(pool);
	}, roleId);

	return 0;
}

}
