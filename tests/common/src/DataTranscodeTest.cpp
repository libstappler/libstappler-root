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

		_desc = stream.str();

		return count == passed;
	}
} _DataTranscodeTest;

}

#endif
