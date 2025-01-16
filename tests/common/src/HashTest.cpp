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
#include "SPPriorityList.h"
#include "SPRefContainer.h"
#include "SPMemPriorityQueue.h"
#include "Test.h"

namespace STAPPLER_VERSIONIZED stappler {

class TestNamedRef : public NamedRef {
public:
	virtual ~TestNamedRef() { }
	virtual StringView getName() const { return _name; }

	TestNamedRef(StringView name) : _name(name) { }

protected:
	StringView _name;
};

class TestPoolRef : public Ref {
public:
	virtual ~TestPoolRef() { }
	virtual StringView getName() const { return _name; }

	TestPoolRef(StringView name) : _name(name) { }

protected:
	StringView _name;
};

}

namespace STAPPLER_VERSIONIZED stappler::app::test {

struct TestItem : public Ref {
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


		runTest(stream, "Refs debug test (std)", count, passed, [&] () -> bool {
			auto ref = Rc<TestNamedRef>::alloc("ref");
			auto refId = memleak::retainBacktrace(ref);

			auto ref2 = ref;
			auto refId2 = memleak::retainBacktrace(ref2);

			memleak::foreachBacktrace(ref, [] (uint64_t, Time, const std::vector<std::string> &) {});

			memleak::releaseBacktrace(ref, refId);
			memleak::foreachBacktrace(ref, [] (uint64_t, Time, const std::vector<std::string> &) {});
			memleak::releaseBacktrace(ref, refId2);
			return true;
		});

		runTest(stream, "Refs debug test (pool)", count, passed, [&] () -> bool {
			auto ref = Rc<TestPoolRef>::alloc("ref");
			auto refId = memleak::retainBacktrace(ref);

			auto ref2 = ref;
			auto refId2 = memleak::retainBacktrace(ref2);

			memleak::foreachBacktrace(ref, [] (uint64_t, Time, const std::vector<std::string> &) {});

			memleak::releaseBacktrace(ref, refId);
			memleak::foreachBacktrace(ref, [] (uint64_t, Time, const std::vector<std::string> &) {});
			memleak::releaseBacktrace(ref, refId2);
			return true;
		});

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

		runTest(stream, "assign", count, passed, [&] {
			HashTable<Rc<TestNamedRef>> t;
			t.assign(Rc<TestNamedRef>::alloc("One"));
			t.assign(Rc<TestNamedRef>::alloc("Two"));
			t.assign(Rc<TestNamedRef>::alloc("3"));
			t.assign(Rc<TestNamedRef>::alloc("Four"));
			t.assign(Rc<TestNamedRef>::alloc("Five"));
			t.assign(Rc<TestNamedRef>::alloc("Six"));
			t.assign(Rc<TestNamedRef>::alloc("Seven"));
			t.assign(Rc<TestNamedRef>::alloc("8"));

			t.contains("Seven");

			HashTable<Rc<TestNamedRef>> t2 = t;
			HashTable<Rc<TestNamedRef>> t3 = move(t);
			return true;
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

		runTest(stream, "Priority list", count, passed, [&] {
			auto val1 = Rc<TestNamedRef>::alloc("One");
			auto val2 = Rc<TestNamedRef>::alloc("Two");
			auto val3 = Rc<TestNamedRef>::alloc("3");
			auto val4 = Rc<TestNamedRef>::alloc("Four");
			auto val5 = Rc<TestNamedRef>::alloc("Five");
			auto val6 = Rc<TestNamedRef>::alloc("Six");
			auto val7 = Rc<TestNamedRef>::alloc("Seven");
			auto val8 = Rc<TestNamedRef>::alloc("8");

			PriorityList<Rc<TestNamedRef>> list;
			list.emplace(val1.get(), 0, val1);
			list.emplace(val8.get(), 0, val8);
			list.emplace(val2.get(), -2, val2);
			list.emplace(val3.get(), 2, val3);
			list.emplace(val4.get(), 4, val5);
			list.emplace(val5.get(), -4, val5);
			list.emplace(val6.get(), 3, val5);
			list.emplace(val7.get(), -3, val5);

			list.foreach([] (void *, int32_t p, Rc<TestNamedRef>&) {
				return p == 4;
			});

			list.find(val8.get());
			list.find(val2.get());
			list.find(val6.get());

			list.erase(val5.get());
			list.erase(val3.get());

			list.empty();
			list.clear();

			return true;
		});

		runTest(stream, "Ordering test", count, passed, [&] {
			memory::PriorityQueue<size_t> queue;

			for (size_t i = 0; i <  memory::PriorityQueue<int>::PreallocatedNodes + memory::PriorityQueue<int>::StorageNodes; ++ i) {
				auto p = memory::PriorityQueue<int>::PriorityType(rand() % 33 - 16);
				queue.push(p, false, i);
			}

			bool success = true;
			auto p = minOf<memory::PriorityQueue<int>::PriorityType>();
			queue.foreach([&] (memory::PriorityQueue<int>::PriorityType priority, int value) {
				if (priority < p) {
					success = false;
					stream << priority << ": " << value;
				}
			});

			return success;
		});

		runTest(stream, "Ordering first test", count, passed, [&] {
			memory::PriorityQueue<size_t> queue;

			for (size_t i = 0; i <  memory::PriorityQueue<int>::PreallocatedNodes + memory::PriorityQueue<int>::StorageNodes; ++ i) {
				auto p = memory::PriorityQueue<int>::PriorityType(rand() % 33 - 16);
				queue.push(p, true, i);
			}

			bool success = true;
			auto p = minOf<memory::PriorityQueue<int>::PriorityType>();
			queue.foreach([&] (memory::PriorityQueue<int>::PriorityType priority, int value) {
				if (priority < p) {
					success = false;
					stream << priority << ": " << value;
				}
			});

			return success;
		});

		runTest(stream, "Free/Capacity test", count, passed, [&] {
			memory::PriorityQueue<size_t> queue;

			auto nIter = memory::PriorityQueue<int>::StorageNodes * 2 + memory::PriorityQueue<int>::PreallocatedNodes;

			stream << "init: " << queue.free_capacity();
			if (queue.free_capacity() != memory::PriorityQueue<int>::PreallocatedNodes) {
				return false;
			}

			for (size_t i = 0; i < nIter; ++ i) {
				auto p = memory::PriorityQueue<int>::PriorityType(rand() % 33 - 16);
				queue.push(p, false, i);
			}

			stream << "; fill: " << queue.free_capacity();
			if (queue.free_capacity() != queue.capacity() - nIter) {
				return false;
			}

			auto half = nIter / 2;
			for (size_t i = 0; i < half; ++ i) {
				if (!queue.pop_direct([&] (memory::PriorityQueue<int>::PriorityType, int) { })) {
					stream << "; no object to pop;";
					return false;
				}
			}

			stream << "; half: " << queue.free_capacity();
			if (queue.free_capacity() != queue.capacity() - (nIter - half)) {
				return false;
			}

			for (size_t i = 0; i < nIter - nIter / 2; ++ i) {
				if (!queue.pop_direct([&] (memory::PriorityQueue<int>::PriorityType, int) { })) {
					stream << "; no object to pop;";
					return false;
				}
			}

			if (queue.pop_direct([&] (memory::PriorityQueue<int>::PriorityType, int) { })) {
				stream << "; extra object to pop;";
				return false;
			}

			stream << "; end: " << queue.free_capacity();
			if (queue.free_capacity() != memory::PriorityQueue<int>::PreallocatedNodes || queue.capacity() != memory::PriorityQueue<int>::PreallocatedNodes) {
				return false;
			}

			return true;
		});

		runTest(stream, "Deallocation test", count, passed, [&] {
			memory::PriorityQueue<size_t> queue;

			auto nIter = memory::PriorityQueue<int>::PreallocatedNodes + memory::PriorityQueue<int>::StorageNodes * 2;

			for (size_t i = 0; i < nIter; ++ i) {
				auto p = memory::PriorityQueue<int>::PriorityType(rand() % 33 - 16);
				queue.push(p, false, i);
			}

			stream << "max: " << queue.capacity();

			bool success = true;
			for (size_t i = 0; i < nIter; ++ i) {
				if (!queue.pop_direct([&] (memory::PriorityQueue<int>::PriorityType, int) { })) {
					success = false;
				}
			}

			stream << "; min: " << queue.capacity();

			// check freelist validity
			for (size_t i = 0; i < memory::PriorityQueue<int>::PreallocatedNodes; ++ i) {
				auto p = memory::PriorityQueue<int>::PriorityType(rand() % 33 - 16);
				queue.push(p, false, i);
			}

			auto p = minOf<memory::PriorityQueue<int>::PriorityType>();
			queue.foreach([&] (memory::PriorityQueue<int>::PriorityType priority, int value) {
				if (priority < p) {
					success = false;
					stream << priority << ": " << value;
				}
			});

			return success && queue.capacity() == memory::PriorityQueue<int>::PreallocatedNodes;
		});

		runTest(stream, "Clear test", count, passed, [&] {
			memory::PriorityQueue<size_t> queue;

			auto nIter = memory::PriorityQueue<int>::PreallocatedNodes + memory::PriorityQueue<int>::StorageNodes * 2;

			for (size_t i = 0; i < nIter; ++ i) {
				auto p = memory::PriorityQueue<int>::PriorityType(rand() % 33 - 16);
				queue.push(p, false, i);
			}

			queue.clear();

			return queue.capacity() == memory::PriorityQueue<int>::PreallocatedNodes;
		});

		runTest(stream, "RefContainer small-remove", count, passed, [&] () -> bool {
			RefContainer<TestItem, Interface> container;

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

		runTest(stream, "RefContainer large-remove", count, passed, [&] () -> bool {
			RefContainer<TestItem, Interface> container;

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

		runTest(stream, "RefContainer small-tag", count, passed, [&] () -> bool {
			RefContainer<TestItem, Interface> container;
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

		runTest(stream, "RefContainer small-tag-all", count, passed, [&] () -> bool {
			RefContainer<TestItem, Interface> container;
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

		runTest(stream, "RefContainer large-tag", count, passed, [&] () -> bool {
			RefContainer<TestItem, Interface> container;
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

		runTest(stream, "RefContainer large-tag-all", count, passed, [&] () -> bool {
			RefContainer<TestItem, Interface> container;
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
} _HashMapTest;

}
