/**
Copyright (c) 2021 Roman Katuntsev <sbkarr@stappler.org>
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
#include "SPString.h"
#include "SPHashTable.h"
#include "SPRef.h"
#include "SPTime.h"
#include "Test.h"

namespace stappler {

class TestNamedRef : public NamedRef {
public:
	virtual ~TestNamedRef() { }
	virtual StringView getName() const { return _name; }

	TestNamedRef(StringView name) : _name(name) { }

protected:
	StringView _name;
};

}

namespace stappler::app::test {

struct HashMapTest : MemPoolTest {
	HashMapTest() : MemPoolTest("HashMapTest") { }

	virtual bool run(pool_t *pool) {
		StringStream stream;
		size_t count = 0;
		size_t passed = 0;
		stream << "\n";

		auto emplace = [&] (HashTable<Rc<TestNamedRef>> &t, std::set<StringView> &control, StringView str) {
			control.emplace(str);
			t.emplace(Rc<TestNamedRef>::alloc(str));
		};

		auto fill = [&] (HashTable<Rc<TestNamedRef>> &t, std::set<StringView> &control) {
			emplace(t, control, "One");
			emplace(t, control, "Two");
			emplace(t, control, "3");
			emplace(t, control, "Four");
			emplace(t, control, "Five");
			emplace(t, control, "Six");
			emplace(t, control, "Seven");
			emplace(t, control, "8");
			emplace(t, control, "Nine");
			emplace(t, control, "10");
			emplace(t, control, "11");
			emplace(t, control, "12");
			emplace(t, control, "13");
			emplace(t, control, "14");
			emplace(t, control, "15");
		};

		runTest(stream, "emplace", count, passed, [&] {
			HashTable<Rc<TestNamedRef>> t;
			std::set<StringView> control;
			t.reserve(30);
			fill(t, control);

			for (auto &it : control) {
				auto iit = t.find(it);
				if (iit == t.end() || (*iit)->getName() != it) {
					std::cout << "Not found: " << it << "\n";
					return false;
				}
			}

			stream << t.get_cell_count() << " / " << t.size();

			auto it = t.begin();
			while (it != t.end()) {
				control.erase((*it)->getName());
				++ it;
			}

			return control.empty();
		});

		runTest(stream, "erase", count, passed, [&] {
			HashTable<Rc<TestNamedRef>> t;
			std::set<StringView> control;
			fill(t, control);

			auto it = control.begin();
			while (it != control.end()) {
				auto iit = t.find(*it);
				if (iit == t.end() || (*iit)->getName() != *it) {
					std::cout << "Not found: " << *it << "\n";
					return false;
				}

				iit = t.erase(*it);
				it = control.erase(it);
			}

			stream << t.get_free_count();
			return control.empty() && t.empty();
		});

		runTest(stream, "erase-iterator", count, passed, [&] {
			HashTable<Rc<TestNamedRef>> t;
			std::set<StringView> control;
			fill(t, control);

			auto it = t.begin();
			while (it != t.end()) {
				it = t.erase(it);
			}

			stream << t.get_free_count();
			return t.empty();
		});

		runTest(stream, "copy", count, passed, [&] {
			HashTable<Rc<TestNamedRef>> tmp;
			std::set<StringView> control;
			fill(tmp, control);

			HashTable<Rc<TestNamedRef>> copy(tmp);

			for (auto &it : control) {
				auto iit = copy.find(it);
				if (iit == copy.end() || (*iit)->getName() != it) {
					std::cout << "Not found: " << it << "\n";
					return false;
				}
			}

			stream << copy.get_cell_count() << " / " << copy.size();

			auto it = copy.begin();
			while (it != copy.end()) {
				control.erase((*it)->getName());
				++ it;
			}

			return control.empty();
		});

		_desc = stream.str();

		return count == passed;
	}
} _HashMapTest;

}
