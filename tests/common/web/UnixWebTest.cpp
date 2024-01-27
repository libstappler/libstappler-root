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
#include "SPNetworkHandle.h"
#include "Test.h"

#include "SPWebUnixRoot.h"

#include "UnixWebTestComponent.cc"

#include <unistd.h>

namespace stappler::app::test {

struct UnixWebTest : Test {
	UnixWebTest() : Test("UnixWebTest") { }

	bool perfromIndexTest(StringView rootPath) {
		auto d = base16::encode<Interface>(valid::makeRandomBytes<Interface>(8));

		StringStream out;

		NetworkHandle h;
		h.init(NetworkHandle::Method::Get, toString("http://localhost:23001/index.html?data=", d));
		h.setReceiveCallback([&] (char *data, size_t size) {
			out << StringView(data, size);
			return size;
		});
		h.perform();

		auto fileData = filesystem::readIntoMemory<Interface>(filepath::merge<Interface>(rootPath, "index.html"));

		if (BytesView(fileData) == BytesView(out.str())) {
			return true;
		}

		return false;
	}

	bool perfromPostTest(StringView rootPath) {
		auto d = base16::encode<Interface>(valid::makeRandomBytes<Interface>(8));

		 auto sendData = Value({
			pair("host", Value("localhost")),
			pair("dbname", Value("postgres")),
			pair("user", Value("stappler")),
			pair("password", Value("stappler")),
			pair("databases", Value({
				Value("stappler")
			})),
		});

		StringStream out;

		NetworkHandle h;
		h.init(NetworkHandle::Method::Post, toString("http://localhost:23001/api/post?data=", d));
		h.setReceiveCallback([&] (char *data, size_t size) {
			out << StringView(data, size);
			return size;
		});
		h.setSendData(sendData, data::EncodeFormat::Json);
		h.perform();



		return false;
	}

	virtual bool run() override {
		auto rootPath = filesystem::currentDir<Interface>("web/root");

		web::UnixRoot::Config cfg;

		cfg.listen = StringView("127.0.01:23001");

		cfg.db = mem_pool::Value({
			pair("host", mem_pool::Value("localhost")),
			pair("dbname", mem_pool::Value("postgres")),
			pair("user", mem_pool::Value("stappler")),
			pair("password", mem_pool::Value("stappler")),
			pair("databases", mem_pool::Value({
				mem_pool::Value("stappler")
			})),
		});

		cfg.hosts.emplace_back(web::UnixHostConfig{
			.hastname = "localhost",
			.admin = "admin@stappler.org",
			.root = StringView(rootPath),
			.components = web::Vector<web::HostComponentInfo>{
				web::HostComponentInfo{
					.name = "TestComonent",
					.version = "0.1",
					.file = StringView(),
					.symbol = "CreateTestComponent",
					.data = mem_pool::Value({
						pair("test", mem_pool::Value("test")),
					})
				}
			},
			.db = mem_pool::Value({
				pair("host", mem_pool::Value("localhost")),
				pair("dbname", mem_pool::Value("stappler")),
				pair("user", mem_pool::Value("stappler")),
				pair("password", mem_pool::Value("stappler")),
				pair("driver", mem_pool::Value("pgsql")),
			})
		});

		auto root = web::UnixRoot::create(move(cfg));

		std::cout << perfromIndexTest(rootPath) << "\n";

		::sleep(1);

		root->cancel();
		root = nullptr;

		return true;
	}
} _UnixWebTest;

}
