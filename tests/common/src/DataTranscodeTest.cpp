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

#ifdef MODULE_STAPPLER_DATA

#include "SPData.h"

namespace STAPPLER_VERSIONIZED stappler::app::test {

static constexpr StringView JsonExampleString(
R"JsonString({
	"glossary": {
		"title": "example glossary",
		"GlossDiv": {
			"title": "S",
			"GlossList": {
				"GlossEntry": {
					"ID": "SGML",
					"SortAs": "SGML",
					"GlossTerm": "Standard Generalized Markup Language",
					"Acronym": "SGML",
					"Abbrev": "ISO 8879:1986",
					"GlossDef": {
						"para": "A meta-markup language, used to create markup languages such as DocBook.",
						"GlossSeeAlso": ["GML", "XML"]
					},
					"GlossSee": "markup"
				}
			}
		}
	}
})JsonString");

template <typename Interface>
static void runTest(std::ostream &out, const Vector<Bytes> &bytes, data::EncodeFormat fmt) {
	uint64_t accum1 = 0;
	uint64_t accum2 = 0;
	auto t = Time::now();

	Vector<data::ValueTemplate<Interface>> data;

	size_t compressed = 0;
	size_t uncompressed = 0;

	for (auto &it : bytes) {
		auto mempool = memory::pool::create();
		memory::pool::push(mempool);

		do {
			t = Time::now();
			auto d = data::read<Interface>(it);
			accum1 += (Time::now() - t).toMicros();
			memory::pool::pop();
			data.emplace_back(d);
		} while (0);

		memory::pool::destroy(mempool);
	}

	size_t idx = 0;
	for (auto &it : data) {
		auto reserve = bytes[idx].size();
		auto mempool = memory::pool::create();
		memory::pool::push(mempool);

		do {
			t = Time::now();
			auto d = data::write(it, fmt, reserve);
			accum2 += (Time::now() - t).toMicros();
			compressed += d.size();
			if (fmt.compression != data::EncodeFormat::Compression::NoCompression) {
				EncodeFormat fmt2;
				fmt2.format = fmt.format;

				auto d = data::write(it, fmt2, reserve);
				uncompressed += d.size();
			} else {
				uncompressed += d.size();
			}
		} while (0);

		memory::pool::pop();
		memory::pool::destroy(mempool);
		++ idx;
	}

	out << accum1 << ", " << accum2 << ", " << float(uncompressed) / float(compressed) << "\n";
}

static void runTestForPath(StringView path) {
	auto mempool = memory::pool::create();
	memory::pool::push(mempool);

	Vector<Value> dataSource;

	filesystem::ftw(path, [&] (StringView path, bool isFile) {
		if (isFile && filepath::lastExtension(path) == "json") {
			auto val = data::readFile<Interface>(path);
			if (val) {
				if (dataSource.size() < 2) {
					dataSource.emplace_back(move(val));
				}
			}
		}
	});

	Vector<Vector<Bytes>> testSources;

	Vector<Pair<data::EncodeFormat, String>> formats {
		pair(data::EncodeFormat::Pretty, "Pretty,      "),
		pair(data::EncodeFormat::Json,   "Json,        "),
		pair(data::EncodeFormat(data::EncodeFormat::Json, data::EncodeFormat::Compression::LZ4Compression),   "Json/lz4,    "),
		pair(data::EncodeFormat(data::EncodeFormat::Json, data::EncodeFormat::Compression::LZ4HCCompression), "Json/lz4HC,  "),
		pair(data::EncodeFormat(data::EncodeFormat::Json, data::EncodeFormat::Compression::Brotli),           "Json/Brotli, "),
		pair(data::EncodeFormat::Cbor,   "Cbor,        "),
		pair(data::EncodeFormat(data::EncodeFormat::Cbor, data::EncodeFormat::Compression::LZ4Compression)  , "Cbor/lz4,    "),
		pair(data::EncodeFormat(data::EncodeFormat::Cbor, data::EncodeFormat::Compression::LZ4HCCompression), "Cbor/lz4HC,  "),
		pair(data::EncodeFormat(data::EncodeFormat::Cbor, data::EncodeFormat::Compression::Brotli),           "Cbor/Brotli, "),
	};

	testSources.resize(formats.size());

	for (auto &it : dataSource) {
		size_t i = 0;
		for (auto &fmt : formats) {
			testSources[i].emplace_back(data::write(it, fmt.first));
			++ i;
		}
	}

	auto mempool2 = memory::pool::create();
	StringStream tmp;
	size_t i = 0;
	for (auto &fmt : formats) {
		memory::pool::push(mempool2);
		runTest<memory::PoolInterface>(tmp, testSources[i], fmt.first);
		memory::pool::pop();
		memory::pool::clear(mempool2);
		++ i;
	}

	tmp << "Type, Test, Read, Write, Compress\n";

	size_t n = 1;

	while (n > 0) {
		i = 0;
		for (auto &fmt : formats) {
			tmp << "Pool, " << fmt.second;
			memory::pool::push(mempool2);
			runTest<memory::PoolInterface>(tmp, testSources[i], fmt.first);
			memory::pool::pop();
			memory::pool::clear(mempool2);
			++ i;
		}

		i = 0;
		for (auto &fmt : formats) {
			tmp << "Std,  " << fmt.second;
			runTest<Interface>(tmp, testSources[i], fmt.first);
			++ i;
		}
		-- n;
	}

	memory::pool::destroy(mempool);

	memory::pool::pop();
}

struct DataTranscodeTest : MemPoolTest {
	DataTranscodeTest() : MemPoolTest("DataTranscodeTest") { }

	virtual bool run(pool_t *pool) {
		StringStream stream;
		size_t count = 0;
		size_t passed = 0;
		stream << "\n";

		runTest(stream, "Json->Cbor", count, passed, [&] {
			auto data = data::read<Interface>(JsonExampleString);
			auto cborBytes = data::write(data, EncodeFormat::Cbor);
			auto cborData = data::read<Interface>(cborBytes);
			return data == cborData;
		});

		runTest(stream, "Json->CborCompressed", count, passed, [&] {
			auto data = data::read<Interface>(JsonExampleString);
			auto cborBytes = data::write(data, EncodeFormat::CborCompressed);
			auto cborData = data::read<Interface>(cborBytes);
			return data == cborData;
		});

#if MODULE_STAPPLER_BROTLI_LIB
		runTest(stream, "Json->CborBrotli", count, passed, [&] {
			auto data = data::read<Interface>(JsonExampleString);
			auto cborBytes = data::write(data, EncodeFormat(EncodeFormat::Cbor, EncodeFormat::Brotli));
			auto cborData = data::read<Interface>(cborBytes);
			return data == cborData;
		});
#endif

		runTest(stream, "DataTest1", count, passed, [&] {
			auto cwd = filesystem::currentDir<Interface>();
			auto path = filepath::root(filepath::root(cwd));

			auto d = filepath::merge<Interface>(path, "tests/data/json1");
			runTestForPath(d);
			return true;
		});

		runTest(stream, "DataTest2", count, passed, [&] {
			auto cwd = filesystem::currentDir<Interface>();
			auto path = filepath::root(filepath::root(cwd));

			auto d = filepath::merge<Interface>(path, "doc/json");
			runTestForPath(d);
			return true;
		});

		_desc = stream.str();

		return count == passed;
	}
} _DataTranscodeTest;

}

#endif
