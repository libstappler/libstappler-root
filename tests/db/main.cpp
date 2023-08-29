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

struct Options {
	db::AccessRoleId role = db::AccessRoleId::Admin;
	String driver = "pgsql";
	String host = "localhost";
	String dbname = "dbtest";
	String user = "admin";
	String password = "admin";
	String data;

	bool help = false;
	bool verbose = false;

	Value encode() const {
		Value ret;
		ret.setString(driver, "driver");
		ret.setString(host, "host");
		ret.setString(dbname, "dbname");
		ret.setString(user, "user");
		ret.setString(password, "password");
		ret.setString(data, "data");
		if (help) { ret.setBool(true, "help"); }
		if (verbose) { ret.setBool(true, "verbose"); }
		ret.setInteger(toInt(role), "role");
		return ret;
	}
};

static int parseOptionSwitch(Options &ret, char c, const char *str) {
	if (c == 'h') {
		ret.help = true;
	} else if (c == 'v') {
		ret.verbose = true;
	}
	return 1;
}

static int parseOptionString(Options &ret, const StringView &str, int argc, const char * argv[]) {
	if (str == "help") {
		ret.help = true;
	} else if (str == "verbose") {
		ret.verbose = true;
	} else if (str == "admin") {
		ret.role = db::AccessRoleId::Admin;
	} else if (str == "nobody") {
		ret.role = db::AccessRoleId::Nobody;
	} else if (str == "authorized") {
		ret.role = db::AccessRoleId::Authorized;
	} else if (str == "system") {
		ret.role = db::AccessRoleId::System;
	} else if (str.starts_with("data=")) {
		ret.data = str.sub("data="_len).str<Interface>();
	} else if (str.starts_with("user=")) {
		ret.user = str.sub("user="_len).str<Interface>();
	} else if (str.starts_with("password=")) {
		ret.password = str.sub("password="_len).str<Interface>();
	} else if (str.starts_with("dbname=")) {
		ret.password = str.sub("dbname="_len).str<Interface>();
	}
	return 1;
}

SP_EXTERN_C int _spMain(argc, argv) {
	auto opts = data::parseCommandLineOptions<Interface, Options>(argc, argv,
			&parseOptionSwitch, &parseOptionString);
	if (opts.first.help) {
		std::cout << HELP_STRING << "\n";
		return 0;
	}

	if (opts.first.verbose) {
		std::cout << " Current work dir: " << stappler::filesystem::currentDir<Interface>() << "\n";
		std::cout << " Documents dir: " << stappler::filesystem::documentsPathReadOnly<Interface>() << "\n";
		std::cout << " Cache dir: " << stappler::filesystem::cachesPathReadOnly<Interface>() << "\n";
		std::cout << " Writable dir: " << stappler::filesystem::writablePathReadOnly<Interface>() << "\n";
		std::cout << " Options: " << stappler::data::EncodeFormat::Pretty << opts.first.encode() << "\n";
		std::cout << " Arguments: \n";
		for (auto &it : opts.second) {
			std::cout << "\t" << it << "\n";
		}
	}

	Value params = opts.first.encode();

	filesystem::mkdir_recursive(".db", false);

	if (opts.second.size() > 1) {
		auto arg = StringView(opts.second.at(1));
		if (StringView(arg).starts_with("update-test-")) {
			arg += "update-test-"_len;
			Server server(params, [&] (memory::pool_t *pool) -> Rc<ServerScheme> {
				return Rc<SchemeUpdateTest>::alloc(pool, arg.readInteger(10).get(0));
			}, opts.first.role);
			return 0;
		} else if (arg == "virtual-test") {
			Rc<SchemeVirtualTest> test;
			Server server(params, [&] (memory::pool_t *pool) -> Rc<ServerScheme> {
				test = Rc<SchemeVirtualTest>::alloc(pool, arg.readInteger(10).get(0));
				return test;
			}, opts.first.role);

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
			}, opts.first.role);

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
			}, opts.first.role);

			test->runTestGlobal(server);
			return 0;
		} else if (arg == "compression-test") {
			Rc<SchemeCompressionTest> test;
			Server server(params, [&] (memory::pool_t *pool) -> Rc<ServerScheme> {
				test = Rc<SchemeCompressionTest>::alloc(pool);
				return test;
			}, opts.first.role);

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
			}, opts.first.role);

			server.perform([&] (const db::Transaction &t) {
				test->runTest(t);
				return true;
			});
			return 0;
		}
	}

	Server server(params, [] (memory::pool_t *pool) -> Rc<ServerScheme> {
		return Rc<ServerScheme>::alloc(pool);
	}, opts.first.role);

	return 0;
}

}
