/**
Copyright (c) 2017 Roman Katuntsev <sbkarr@stappler.org>
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

#ifdef MODULE_STAPPLER_DATA

#include "SPTime.h"
#include "SPString.h"
#include "SPData.h"
#include "Test.h"

namespace STAPPLER_VERSIONIZED stappler::app::test {

#if MODULE_STAPPLER_FILESYSTEM

struct PoolCborTest : MemPoolTest {
	PoolCborTest() : MemPoolTest("PoolCborTest") { }

	virtual bool run(pool_t *pool) {
		StringStream stream;
		size_t count = 0;
		size_t passed = 0;
		stream << "\n";
		size_t ntests = 32;

		runTest(stream, "StreamCborTest", count, passed, [&] {
			auto t = Time::now();
			auto d = data::readFile<memory::PoolInterface>(filesystem::currentDir<memory::PoolInterface>("app.cbor"));

			stream << (Time::now() - t).toMicroseconds();
			return true;
		});

		runTest(stream, "StdCborTest", count, passed, [&] {
			auto data = filesystem::readIntoMemory<Interface>(filesystem::currentDir<Interface>("app.cbor"));

			uint64_t v = 0;
			for (size_t i = 0; i < ntests; ++i) {
				auto t = Time::now();
				auto d1 = data::read<Interface>(data);
				v += (Time::now() - t).toMicroseconds();
			}
			stream << v / ntests;
			return true;
		});

		runTest(stream, "PoolCborTest", count, passed, [&] {
			auto data = filesystem::readIntoMemory<memory::PoolInterface>(filesystem::currentDir<memory::PoolInterface>("app.cbor"));

			uint64_t v = 0;
			for (size_t i = 0; i < ntests; ++i) {
				memory::pool::clear(pool);
				auto t = Time::now();
				auto d = data::read<memory::PoolInterface>(data);
				v += (Time::now() - t).toMicroseconds();
			}
			stream << v / ntests << " "
					 << memory::pool::get_allocated_bytes(pool) << " "
					 << memory::pool::get_return_bytes(pool);
			return true;
		});

		runTest(stream, "CompareCborTest", count, passed, [&] {
			memory::pool::clear(pool);
			auto t = Time::now();
			auto data = filesystem::readIntoMemory<memory::PoolInterface>(filesystem::currentDir<memory::PoolInterface>("app.cbor"));
			stream << (Time::now() - t).toMicroseconds() << " ";

			t = Time::now();
			auto d2 = data::read<memory::PoolInterface>(data);
			stream << (Time::now() - t).toMicroseconds() << " ";

			t = Time::now();
			auto d1 = data::read<Interface>(data);
			stream << (Time::now() - t).toMicroseconds() << " ";

			bool ret = (Value(d2) == d1);
			stream << ret;

			return ret;
		});

		_desc = stream.str();

		return count == passed;
	}
} _PoolCborTest;


struct CborDataTest : Test {
	CborDataTest() : Test("CborDataTest") { }

	virtual bool run() override {
		StringStream stream;
		size_t count = 0;
		size_t passed = 0;
		stream << "\n";

		auto cborPath = filesystem::currentDir<Interface>("data");

		Map<String, Value> cborData;
		Map<String, Value> jsonData;
		Map<String, String> diagData;

		filesystem::ftw(cborPath, [&] (const StringView &path, bool isFile) {
			if (isFile) {
				auto ext = filepath::lastExtension(path);

				auto fileData = filesystem::readIntoMemory<Interface>(path);

				if (ext == "cbor") {
					cborData.emplace(filepath::name(path).str<Interface>(), data::read<Interface>(fileData));
				} else if (ext == "json") {
					jsonData.emplace(filepath::name(path).str<Interface>(), data::read<Interface>(fileData));
				} else if (ext == "diag") {
					diagData.emplace(filepath::name(path).str<Interface>(), filesystem::readTextFile<Interface>(path));
				}
			}
		});

		for (auto &it : cborData) {
			++ count;
			auto jIt = jsonData.find(it.first);
			if (jIt != jsonData.end()) {
				if (it.second == jIt->second) {
					++ passed;
				} else {
					Value cValue = it.second;
					Value dValue = jIt->second;
					if (cValue.isArray()) {
						if (cValue.size() != dValue.size()) {
							stream << it.first << " failed: " << cValue << " vs " << dValue << "\n";
						} else {
							auto &cArr = cValue.asArray();
							auto &dArr = dValue.asArray();

							for (size_t i = 0; i < cArr.size(); ++ i) {
								auto &v1 = cArr[i];
								auto &v2 = dArr[i];
								if (v1 != v2) {
									stream << it.first << " failed: " << v1 << " vs " << v2 << "\n";
									std::cout << it.first << " failed: " << v1 << " vs " << v2 << "\n";
								}
							}
						}
					} else {
						stream << it.first << " failed: " << cValue << " vs " << dValue << "\n";
					}
				}
			} else {
				auto dIt = diagData.find(it.first);
				if (dIt != diagData.end()) {
					stream << it.first << ": " << it.second << " diag: " << dIt->second << "\n";
				}
				++ passed;
			}
		}

		_desc = stream.str();

		return passed == count;
	}

} CborDataTest;

struct CborDataFileTest : Test {
	CborDataFileTest() : Test("CborDataFileTest") { }

	virtual bool run() override {
		StringStream stream;
		size_t count = 0;
		size_t passed = 0;
		stream << "\n";

		auto cborPath = filesystem::currentDir<Interface>("data");

		Map<String, Value> cborData;
		Map<String, Value> jsonData;
		Map<String, String> diagData;

		filesystem::ftw(cborPath, [&] (const StringView &path, bool isFile) {
			if (isFile) {
				auto ext = filepath::lastExtension(path);

				if (ext == "cbor") {
					cborData.emplace(filepath::name(path).str<Interface>(), data::readFile<Interface>(path));
				} else if (ext == "json") {
					jsonData.emplace(filepath::name(path).str<Interface>(), data::readFile<Interface>(path));
				} else if (ext == "diag") {
					diagData.emplace(filepath::name(path).str<Interface>(), filesystem::readTextFile<Interface>(path));
				}
			}
		});

		for (auto &it : cborData) {
			++ count;
			auto jIt = jsonData.find(it.first);
			if (jIt != jsonData.end()) {
				if (it.second == jIt->second) {
					++ passed;
				} else {
					Value cValue = it.second;
					Value dValue = jIt->second;
					stream << it.first << " failed: " << cValue << " vs " << dValue << "\n";
				}
			} else {
				auto dIt = diagData.find(it.first);
				if (dIt != diagData.end()) {
					stream << it.first << ": " << it.second << " diag: " << dIt->second << "\n";
				}
				++ passed;
			}
		}

		_desc = stream.str();

		return passed == count;
	}

} CborDataFileTest;

#endif

}

#endif
