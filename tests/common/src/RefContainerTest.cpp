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
#include "SPRefContainer.h"
#include "Test.h"

namespace stappler::app::test {

struct TestItem : public RefBase<memory::StandartInterface> {
	virtual ~TestItem() {
		// (*_out) << "Destroy: " << _tag << "\n";
	}

	TestItem(StringStream *out, uint32_t tag) : _tag(tag), _out(out) { }

	bool isDone() const { return _done; }

	uint32_t getTag() const { return _tag; }
	void setTag(uint32_t tag) { _tag = tag; }

	void invalidate() { _done = true; }

	bool _done = false;
	uint32_t _tag = 0;
	StringStream *_out = nullptr;
};

struct RefContainerTest : Test {
	RefContainerTest() : Test("RefContainerTest") { }

	virtual bool run() override {
		StringStream stream;
		size_t count = 0;
		size_t passed = 0;
		stream << "\n";

		runTest(stream, "small-remove", count, passed, [&] () -> bool {
			RefContainer<TestItem> container;

			Vector<TestItem *> ptrs;

			for (uint32_t i = 0; i < 4; ++ i) {
				auto item = Rc<TestItem>::alloc(&stream, i);
				ptrs.emplace_back(container.addItem(item));
			}

			container.removeItem(ptrs[2]);
			ptrs.erase(ptrs.begin() + 2);

			bool success = true;
			size_t i = 0;
			container.foreach([&] (TestItem *item) {
				if (item != ptrs[i]) {
					success = false;
				}
				++ i;
				return true;
			});
			return success;
		});

		runTest(stream, "large-remove", count, passed, [&] () -> bool {
			RefContainer<TestItem> container;

			Vector<TestItem *> ptrs;

			for (uint32_t i = 0; i < 7; ++ i) {
				auto item = Rc<TestItem>::alloc(&stream, i);
				ptrs.emplace_back(container.addItem(item));
			}

			container.removeItem(ptrs[2]);
			container.removeItem(ptrs[5]);
			ptrs.erase(ptrs.begin() + 5);
			ptrs.erase(ptrs.begin() + 2);

			bool success = true;
			size_t i = 0;
			container.foreach([&] (TestItem *item) {
				if (item != ptrs[i]) {
					success = false;
				}
				++ i;
				return true;
			});
			return success;
		});

		runTest(stream, "small-tag", count, passed, [&] () -> bool {
			RefContainer<TestItem> container;
			Vector<uint32_t> tags;

			for (uint32_t i = 0; i < 4; ++ i) {
				auto item = Rc<TestItem>::alloc(&stream, rand() % 10);
				tags.emplace_back(item->_tag);
				container.addItem(item);
			}

			for (auto &it : tags) {
				if (auto v = container.getItemByTag(it)) {
					if (v->_tag != it) {
						return false;
					}
				} else {
					return false;
				}
			}

			container.foreach([&] (TestItem *item) {
				stream << "Item: " << item->_tag << "\n";
				return true;
			});

			stream << "Remove " << tags.at(1) << "; Invalidate: " <<  tags.at(3) << "\n";

			container.invalidateItemByTag(tags.at(3));
			container.removeItemByTag(tags.at(1));

			container.foreach([&] (TestItem *item) {
				stream << "Item: " << item->_tag << "\n";
				return true;
			});

			stream << "Cleanup\n";

			container.cleanup();

			container.foreach([&] (TestItem *item) {
				stream << "Item: " << item->_tag << "\n";
				return true;
			});
			return true;
		});

		runTest(stream, "small-tag-all", count, passed, [&] () -> bool {
			RefContainer<TestItem> container;
			Vector<uint32_t> tags;

			for (uint32_t i = 0; i < 4; ++ i) {
				auto item = Rc<TestItem>::alloc(&stream, rand() % 3);
				tags.emplace_back(item->_tag);
				container.addItem(item);
			}

			for (auto &it : tags) {
				if (auto v = container.getItemByTag(it)) {
					if (v->_tag != it) {
						return false;
					}
				} else {
					return false;
				}
			}

			stream << "Remove " << tags.at(1) << "; Invalidate: " <<  tags.at(3) << "\n";

			container.invalidateAllItemsByTag(tags.at(3));
			container.removeAllItemsByTag(tags.at(1));

			container.foreach([&] (TestItem *item) {
				stream << "Item: " << item->_tag << "\n";
				return true;
			});

			stream << "Cleanup\n";

			container.cleanup();

			container.foreach([&] (TestItem *item) {
				stream << "Item: " << item->_tag << "\n";
				return true;
			});
			return true;
		});

		runTest(stream, "large-tag", count, passed, [&] () -> bool {
			RefContainer<TestItem> container;
			Vector<uint32_t> tags;

			for (uint32_t i = 0; i < 6; ++ i) {
				auto item = Rc<TestItem>::alloc(&stream, rand() % 10);
				tags.emplace_back(item->_tag);
				container.addItem(item);
			}

			for (auto &it : tags) {
				if (auto v = container.getItemByTag(it)) {
					if (v->_tag != it) {
						return false;
					}
				} else {
					return false;
				}
			}

			container.foreach([&] (TestItem *item) {
				stream << "Item: " << item->_tag << "\n";
				return true;
			});

			stream << "Remove " << tags.at(1) << "; Invalidate: " <<  tags.at(3) << "\n";

			container.invalidateItemByTag(tags.at(3));
			container.removeItemByTag(tags.at(1));

			container.foreach([&] (TestItem *item) {
				stream << "Item: " << item->_tag << "\n";
				return true;
			});

			stream << "Cleanup\n";

			container.cleanup();

			container.foreach([&] (TestItem *item) {
				stream << "Item: " << item->_tag << "\n";
				return true;
			});
			return true;
		});

		runTest(stream, "large-tag-all", count, passed, [&] () -> bool {
			RefContainer<TestItem> container;
			Vector<uint32_t> tags;

			for (uint32_t i = 0; i < 6; ++ i) {
				auto item = Rc<TestItem>::alloc(&stream, rand() % 3);
				tags.emplace_back(item->_tag);
				container.addItem(item);
			}

			for (auto &it : tags) {
				if (auto v = container.getItemByTag(it)) {
					if (v->_tag != it) {
						return false;
					}
				} else {
					return false;
				}
			}

			stream << "Remove " << tags.at(1) << "; Invalidate: " <<  tags.at(3) << "\n";

			container.invalidateAllItemsByTag(tags.at(3));
			container.removeAllItemsByTag(tags.at(1));

			container.foreach([&] (TestItem *item) {
				stream << "Item: " << item->_tag << "\n";
				return true;
			});

			stream << "Cleanup\n";

			container.cleanup();

			container.foreach([&] (TestItem *item) {
				stream << "Item: " << item->_tag << "\n";
				return true;
			});
			return true;
		});

		_desc = stream.str();

		return count == passed;
	}
} _RefContainerTest;

}
