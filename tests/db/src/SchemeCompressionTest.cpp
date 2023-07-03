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

#include "SchemeCompressionTest.h"
#include "SPFilesystem.h"
#include "STPqHandle.h"

namespace stappler::dbtest {

SchemeCompressionTest::~SchemeCompressionTest() { }

SchemeCompressionTest::SchemeCompressionTest(memory::pool_t *p) : ServerScheme(p) {
	using namespace db;

	_compressionTest.define({
		Field::Text("name", MinLength(3)),
		Field::Data("message", Flags::Compressed),
	});
}

void SchemeCompressionTest::fillSchemes(db::Map<StringView, const db::Scheme *> &schemes) {
	ServerScheme::fillSchemes(schemes);

	schemes.emplace(_compressionTest.getName(), &_compressionTest);
}

bool SchemeCompressionTest::runTest(const db::Transaction &t) {
	using namespace db;
	return t.performAsSystem([&] {
		auto file = filesystem::currentDir<Interface>("lipsum.txt");
		if (!filesystem::exists(file)) {
			return false;
		}

		auto data = filesystem::readIntoMemory<Interface>(file);

		auto val = _compressionTest.create(t, Value({
			pair("name", Value(toString("Text", Time::now().toMicros()))),
			pair("message", Value({
				pair("text", Value(data))
			}))
		}));

		size_t messageSize = data.size();

		auto handle = (pq::Handle *)t.getAdapter().interface();
		auto query = toString("SELECT LENGTH(message) FROM ", _compressionTest.getName(), " WHERE __oid=", val.getInteger("__oid"));
		handle->performSimpleSelect(query, [&] (db::sql::Result &res) {
			messageSize = size_t(res.readId());
		});

		return messageSize < data.size();
	});
}

}
