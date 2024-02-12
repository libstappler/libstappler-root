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

SP_EXTERN_C int main(int argc, const char *argv[]) {
	auto opts = data::parseCommandLineOptions<Interface, Value>(argc, argv,
			&parseOptionSwitch, &parseOptionString);
	if (opts.first.getBool("help")) {
		std::cout << HELP_STRING << "\n";
		return 0;
	}

	if (opts.first.getBool("verbose")) {
		std::cout << " Current work dir: " << stappler::filesystem::currentDir<Interface>() << "\n";
		std::cout << " Documents dir: " << stappler::filesystem::documentsPathReadOnly<Interface>() << "\n";
		std::cout << " Cache dir: " << stappler::filesystem::cachesPathReadOnly<Interface>() << "\n";
		std::cout << " Writable dir: " << stappler::filesystem::writablePathReadOnly<Interface>() << "\n";
		std::cout << " Options: " << stappler::data::EncodeFormat::Pretty << opts.first << "\n";
		std::cout << " Arguments: \n";
		for (auto &it : opts.second) {
			std::cout << "\t" << it << "\n";
		}
	}

	auto mempool = memory::pool::create();
	memory::pool::push(mempool);

	auto dataDir = opts.first.getString("data");
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

	std::cout << "Type, Test, Read, Write, Compress\n";

	size_t n = 16;

	while (n > 0) {
		i = 0;
		for (auto &fmt : formats) {
			std::cout << "Pool, " << fmt.second;
			memory::pool::push(mempool2);
			runTest<memory::PoolInterface>(std::cout, testSources[i], fmt.first);
			memory::pool::pop();
			memory::pool::clear(mempool2);
			++ i;
		}

		i = 0;
		for (auto &fmt : formats) {
			std::cout << "Std,  " << fmt.second;
			runTest<Interface>(std::cout, testSources[i], fmt.first);
			++ i;
		}
		-- n;
	}

	memory::pool::destroy(mempool);

	memory::pool::pop();

	return 0;
}

}
