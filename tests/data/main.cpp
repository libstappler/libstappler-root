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

namespace stappler::app {

using namespace mem_std;

static constexpr auto HELP_STRING(
R"HelpString(dataapp <options>
Options are one of:
    -v (--verbose)
    -h (--help))HelpString");

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
	} else if (str.starts_with("data=")) {
		ret.setString(str.sub("data="_len), "data");
	}
	return 1;
}

template <typename Interface>
static void runTest(const Vector<Bytes> &bytes, data::EncodeFormat fmt) {
	uint64_t accum1 = 0;
	uint64_t accum2 = 0;
	auto t = Time::now();

	Vector<data::ValueTemplate<Interface>> data;

	size_t objects = 0;

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
			if (!d.empty()) {
				++ objects;
			}
		} while (0);

		memory::pool::pop();
		memory::pool::destroy(mempool);
		++ idx;
	}

	std::cout << accum1 << " " << accum2 << " (" << objects << " object)\n";
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

	auto mempool = memory::pool::create();
	memory::pool::push(mempool);

	auto dataDir = opts.getString("data");
	if (dataDir.empty()) {
		dataDir = filesystem::currentDir<Interface>("doc/out");
	}

	Vector<Value> dataSource;

	filesystem::ftw(dataDir, [&] (StringView path, bool isFile) {
		if (isFile && filepath::lastExtension(path) == "json") {
			auto val = data::readFile<Interface>(path);
			if (val) {
				dataSource.emplace_back(move(val));
			}
		}
	});

	Vector<Vector<Bytes>> testSources;

	Vector<Pair<data::EncodeFormat, String>> formats {
		pair(data::EncodeFormat::Pretty, "Format: Pretty     "),
		pair(data::EncodeFormat::Json,   "Format: Json       "),
		pair(data::EncodeFormat(data::EncodeFormat::Json, data::EncodeFormat::Compression::LZ4Compression),   "Format: Json/lz4   "),
		pair(data::EncodeFormat(data::EncodeFormat::Json, data::EncodeFormat::Compression::LZ4HCCompression), "Format: Json/lz4HC "),
		pair(data::EncodeFormat(data::EncodeFormat::Json, data::EncodeFormat::Compression::Brotli),           "Format: Json/Brotli"),
		pair(data::EncodeFormat::Cbor,   "Format: Cbor       "),
		pair(data::EncodeFormat(data::EncodeFormat::Cbor, data::EncodeFormat::Compression::LZ4Compression)  , "Format: Cbor/lz4   "),
		pair(data::EncodeFormat(data::EncodeFormat::Cbor, data::EncodeFormat::Compression::LZ4HCCompression), "Format: Cbor/lz4HC "),
		pair(data::EncodeFormat(data::EncodeFormat::Cbor, data::EncodeFormat::Compression::Brotli),           "Format: Cbor/Brotli"),
	};

	testSources.resize(formats.size());

	for (auto &it : dataSource) {
		size_t i = 0;
		for (auto &fmt : formats) {
			testSources[i].emplace_back(data::write(it, fmt.first));
			++ i;
		}
	}

	size_t i = 0;
	for (auto &fmt : formats) {
		std::cout << "Pool: " << fmt.second << ": ";
		auto mempool = memory::pool::create();
		memory::pool::push(mempool);
		runTest<memory::PoolInterface>(testSources[i], fmt.first);
		memory::pool::pop();
		memory::pool::destroy(mempool);
		++ i;
	}

	i = 0;
	for (auto &fmt : formats) {
		std::cout << "Std: " << fmt.second << ": ";
		runTest<Interface>(testSources[i], fmt.first);
		++ i;
	}

	memory::pool::pop();

	return 0;
}

}
