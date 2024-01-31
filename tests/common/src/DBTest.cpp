/**
Copyright (c) 2022 Roman Katuntsev <sbkarr@stappler.org>
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
#include "Test.h"

#if MODULE_STAPPLER_DB

#include "SPSqlDriver.h"
#include "SPDbScheme.h"

namespace STAPPLER_VERSIONIZED stappler::app::test {

struct DbTest : MemPoolTest {
	DbTest() : MemPoolTest("DbTest") { }

	virtual bool run(pool_t *pool) {
		StringStream stream;
		bool success = true;
		stream << "\n";

		// open db driver interface
		// driver sqlite3 build with static library
		// for pqsql, you can manually specify path to libpq with pqsql:<path>
		auto driver = db::sql::Driver::open(pool, nullptr, "sqlite3");
		if (!driver) {
			return false;
		}

		// connection parameters is backend-dependent
		// create in-memory temporary database with sqlite
		auto driverHandle = driver->connect(mem_pool::Map<StringView, StringView>{
			pair("dbname", ":memory:"),
			pair("mode", "memory"),
		});

		if (!driverHandle.get()) {
			return false;
		}

		db::Scheme dataScheme("data_scheme");

		dataScheme.define({
			db::Field::Integer("number"),
			db::Field::Integer("ctime", db::Flags::AutoCTime),
			db::Field::Integer("mtime", db::Flags::AutoCTime),
			db::Field::Text("name"),
			db::Field::Password("password"),
			db::Field::Float("value"),
		});

		// we need BackendInterface to make queries, or an Adapter wrapper
		driver->performWithStorage(driverHandle, [&] (const db::Adapter &adapter) {
			db::BackendInterface::Config cfg;
			cfg.name = StringView("temporary_db");

			mem_pool::Map<StringView, const db::Scheme *> schemes({
				pair(dataScheme.getName(), &dataScheme)
			});

			if (!adapter.init(cfg, schemes)) {
				success = false;
				return;
			}

			adapter.performWithTransaction([&] (const db::Transaction &t) {
				auto v = dataScheme.create(t, mem_pool::Value({
					pair("number", mem_pool::Value(42)),
					pair("name", mem_pool::Value("testName")),
					pair("password", mem_pool::Value("Pas$w0rd")),
					pair("value", mem_pool::Value(37.72)),
				}));

				if (!v) {
					success = false;
					return false;
				}

				auto v1 = dataScheme.get(t, v.getInteger("__oid"));

				if (v != v1) {
					success = false;
					return false;
				}
				return true;
			});
		});

		driver->finish(driverHandle);

		_desc = stream.str();

		return success;
	}
} _DbTest;

}

#endif
