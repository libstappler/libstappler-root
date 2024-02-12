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

#ifndef TEST_COMMON_SRC_TEST_H_
#define TEST_COMMON_SRC_TEST_H_

#include "SPCommon.h"
#include "SPMemory.h"

namespace STAPPLER_VERSIONIZED stappler::app::test {

// import std memory interface
using namespace stappler::mem_std;

struct Test {
	static bool RunAll();
	static bool Run(StringView);
	static void List();

	Test(StringView);
	virtual ~Test();

	virtual bool run() = 0;

	template <typename Callback>
	void runTest(StringStream &stream, StringView name, size_t &count, size_t &passed, const Callback &cb) {
		stream << "\t" << name << ": \"";
		if (cb()) {
			++ passed;
			stream << "\" passed\n";
		} else {
			stream << "\" failed\n";
		}
		++ count;
	}

	StringView name() const { return _name; }
	StringView desc() const { return _desc; }

	int64_t rand_int64_t() const;
	uint64_t rand_uint64_t() const;

	int64_t rand_int32_t() const;
	uint64_t rand_uint32_t() const;

	float rand_float() const;
	double rand_double() const;

	String _name;
	String _desc;
};

struct MemPoolTest : Test {
	using String = memory::string;
	using WideString = memory::u16string;

	template <typename T>
	using Vector = memory::vector<T>;

	using pool_t = memory::pool_t;

	MemPoolTest(StringView str) : Test(str) { }

	virtual bool run(pool_t *) = 0;

	virtual bool run();
};

}

#endif /* TEST_COMMON_SRC_TEST_H_ */
