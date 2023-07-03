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

#ifndef TESTS_DB_SRC_SCHEMERELATIONTEST_H_
#define TESTS_DB_SRC_SCHEMERELATIONTEST_H_

#include "Server.h"

namespace stappler::dbtest {

class SchemeRelationTest : public ServerScheme {
public:
	virtual ~SchemeRelationTest();
	SchemeRelationTest(memory::pool_t *, uint32_t version);

	virtual void fillSchemes(db::Map<StringView, const db::Scheme *> &);

	void fillTest(const db::Transaction &, int64_t id);
	bool checkTest(const db::Transaction &, int64_t id);

protected:
	db::Scheme _objects = db::Scheme("objects");
	db::Scheme _refs = db::Scheme("refs");
	db::Scheme _subobjects = db::Scheme("subobjects");
	db::Scheme _images = db::Scheme("images");
	db::Scheme _hierarchy = db::Scheme("hierarchy");
	db::Scheme _pages = db::Scheme("pages");
};

}

#endif /* TESTS_DB_SRC_SCHEMERELATIONTEST_H_ */
