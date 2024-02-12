/**
 Copyright (c) 2024 Stappler LLC <admin@stappler.dev>

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

#ifndef EXTRA_WEBSERVER_WEBSERVER_UTILS_SPWEBASYNCTASK_H_
#define EXTRA_WEBSERVER_WEBSERVER_UTILS_SPWEBASYNCTASK_H_

#include "SPWeb.h"

namespace STAPPLER_VERSIONIZED stappler::web {

class Host;
class AsyncTask;

class AsyncTaskGroup : public AllocBase {
public:
	static AsyncTaskGroup *getCurrent();

	AsyncTaskGroup();
	AsyncTaskGroup(const Host &);
	AsyncTaskGroup(const Host &, std::function<void()> &&);

	void onAdded(AsyncTask *);
	void onPerformed(AsyncTask *);

	void update();
	void waitForAll();

	bool perform(const Callback<void(AsyncTask &)> &cb);

	Pair<size_t, size_t> getCounters() const; // <completed, added>

	Host getHost() const { return _host; }

protected:
	Time _lastUpdate = Time::now();
	std::thread::id _threadId = std::this_thread::get_id();
	std::mutex _mutex;
	std::mutex _condMutex;
	std::condition_variable _condition;

	std::vector<AsyncTask *> _queue;
	std::atomic<size_t> _added = 0;
	std::atomic<size_t> _completed = 0;
	Host _host;

	std::function<void()> _notifyFn = nullptr;
};

class AsyncTask : public AllocBase {
public:
	static constexpr uint8_t PriorityLowest = config::PriorityLowest;
	static constexpr uint8_t PriorityLow = config::PriorityLow;
	static constexpr uint8_t PriorityNormal = config::PriorityNormal;
	static constexpr uint8_t PriorityHigh = config::PriorityHigh;
	static constexpr uint8_t PriorityHighest = config::PriorityHighest;

	using ExecuteCallback = Function<bool(const AsyncTask &)>;
	using CompleteCallback = Function<void(const AsyncTask &, bool)>;

	// usage:
	// auto newTask = Task::prepare([&] (Task &task) {
	//    // do configuration in Task's memory pool context
	// });
	//
	static AsyncTask *prepare(pool_t *rootPool, const Callback<void(AsyncTask &)> &cb, AsyncTaskGroup * = nullptr);
	static AsyncTask *prepare(const Callback<void(AsyncTask &)> &cb, AsyncTaskGroup * = nullptr);

	static bool perform(const Host &, const Callback<void(AsyncTask &)> &cb, AsyncTaskGroup * = nullptr);
	static bool perform(const Callback<void(AsyncTask &)> &cb, AsyncTaskGroup * = nullptr);

	static void destroy(AsyncTask *);

	static void run(AsyncTask *);

	static AsyncTask *getCurrent();

	void addExecuteFn(const ExecuteCallback &);
	void addExecuteFn(ExecuteCallback &&);

	void addCompleteFn(const CompleteCallback &);
	void addCompleteFn(CompleteCallback &&);

	/* set default task priority */
	void setPriority(uint8_t priority) { _priority = priority; }

	/* get task priority */
	uint8_t getPriority() const { return _priority; }

	/* used by task manager to set success state */
	void setSuccessful(bool value) { _isSuccessful = value; }

	/* if task execution was successful */
	bool isSuccessful() const { return _isSuccessful; }

	void setHost(const Host &serv) { _host = serv; }
	const Host &getHost() const { return _host; }

	void setScheduled(Time t) { _scheduled = t; }
	Time getScheduled() const { return _scheduled; }

	AsyncTaskGroup *getGroup() const { return _group; }

	void performWithStorage(const Callback<void(const db::Transaction &)> &) const;

	bool execute();
	void onComplete();

	pool_t *pool() const { return _pool; }

protected:
	AsyncTask(pool_t *, AsyncTaskGroup *);

	pool_t *_pool = nullptr;
	uint8_t _priority = PriorityNormal;
	Time _scheduled;
	bool _isSuccessful = false;

	Host _host;
	Vector<ExecuteCallback> _execute;
	Vector<CompleteCallback> _complete;

	AsyncTaskGroup *_group = nullptr;
};

enum class SharedMode {
	Pool,
	Allocator,
};

template <typename T>
class Shared : public RefBase<memory::PoolInterface> {
public:
	template <typename ...Args>
	static Shared *create(Args && ...);

	template <typename ...Args>
	static Shared *create(pool_t *, Args && ...);

	template <typename ...Args>
	static Shared *create(SharedMode, Args && ...);

	virtual ~Shared();

	template <typename Callback>
	void perform(Callback &&cb) {
		web::perform([&, this] {
			cb(_shared);
		}, _pool);
	}

	inline T *get() const { return _shared; }

	inline operator T * () const { return get(); }
	inline T * operator->() const { return get(); }

	inline operator bool () const { return _shared != nullptr; }

	pool_t *getPool() const { return _pool; }
	allocator_t *getAllocator() const { return _allocator; }

protected:
	Shared(SharedMode m, allocator_t *, pool_t *, T *);

	allocator_t *_allocator = nullptr;
	pool_t *_pool = nullptr;
	T *_shared = nullptr;
	SharedMode _mode = SharedMode::Pool;
};


template <typename _Base>
class SharedRc {
public:
	using Base = typename std::remove_cv<_Base>::type;
	using Type = Shared<Base>;
	using Self = SharedRc<Base>;
	using Pointer = Type *;

	template <class... Args>
	static inline Self create(Args && ... args) {
		auto pRet = Type::create();
		Self ret(nullptr);
		pRet->perform([&] (Base *base) {
		    if (base->init(std::forward<Args>(args)...)) {
		    	ret = Self(pRet, true); // unsafe assignment
			}
		});
		if (!ret) {
			delete pRet;
		}
		return ret;
	}

	template <class... Args>
	static inline Self create(pool_t *pool, Args && ... args) {
		auto pRet = Type::create(pool);
		Self ret(nullptr);
		pRet->perform([&] (Base *base) {
		    if (base->init(std::forward<Args>(args)...)) {
		    	ret = Self(pRet, true); // unsafe assignment
			}
		});
		if (!ret) {
			delete pRet;
		}
		return ret;
	}

	template <class... Args>
	static inline Self create(SharedMode mode, Args && ... args) {
		auto pRet = Type::create(mode);
		Self ret(nullptr);
		pRet->perform([&] (Base *base) {
		    if (base->init(std::forward<Args>(args)...)) {
		    	ret = Self(pRet, true); // unsafe assignment
			}
		});
		if (!ret) {
			delete pRet;
		}
		return ret;
	}

	static inline Self alloc() {
		return Self(new Type(), true);
	}

	template <class... Args>
	static inline Self alloc(Args && ... args) {
		return Self(new Type(std::forward<Args>(args)...), true);
	}

	inline SharedRc() : _ptr(nullptr) { }
	inline SharedRc(const nullptr_t &) : _ptr(nullptr) { }
	inline SharedRc(const Pointer &value) : _ptr(value) { doRetain(); }
	inline SharedRc(const Self &v) { _ptr = v._ptr; doRetain(); }
	inline SharedRc(Self &&v) {
		_ptr = v._ptr; v._ptr = nullptr;
	}

	inline SharedRc & operator = (const nullptr_t &) {
		doRelease();
		_ptr = nullptr;
		return *this;
	}

	inline SharedRc & operator = (const Pointer &value) { set(value); return *this; }
	inline SharedRc & operator = (const Self &v) { set(v._ptr); return *this; }
	inline SharedRc & operator = (Self &&v) {
		doRelease();
		_ptr = v._ptr; v._ptr = nullptr;
		return *this;
	}

	inline ~SharedRc() { doRelease(); _ptr = nullptr; }

	inline void set(const Pointer &value) {
		_ptr = doSwap(value);
	}

	inline Base *get() const {
		return _ptr ? _ptr->get() : nullptr;
	}

	inline operator Base * () const { return get(); }
	inline operator bool () const { return _ptr && _ptr->get() != nullptr; }

	inline void swap(Self & v) { auto ptr = _ptr; _ptr = v._ptr; v._ptr = ptr; }

	inline Base * operator->() const { return _ptr ? _ptr->get() : nullptr; }

	template <typename Target>
	inline RcBase<Target> cast() const {
		if (auto v = dynamic_cast<Target *>(_ptr)) {
			return RcBase<Target>(v);
		}
		return RcBase<Target>(nullptr);
	}

	inline bool operator == (const Self & other) const { return _ptr == other._ptr; }
	inline bool operator == (const Base * & other) const { return _ptr->get() == other; }
	inline bool operator == (const std::nullptr_t other) const { return _ptr == other; }

	inline bool operator != (const Self & other) const { return _ptr != other._ptr; }
	inline bool operator != (const Base * & other) const { return _ptr->get() != other; }
	inline bool operator != (const std::nullptr_t other) const { return _ptr != other; }

	inline bool operator > (const Self & other) const { return _ptr > other._ptr; }
	inline bool operator > (const Base * other) const { return _ptr->get() > other; }
	inline bool operator > (const std::nullptr_t other) const { return _ptr > other; }

	inline bool operator < (const Self & other) const { return _ptr < other._ptr; }
	inline bool operator < (const Base * other) const { return _ptr->get() < other; }
	inline bool operator < (const std::nullptr_t other) const { return _ptr < other; }

	inline bool operator >= (const Self & other) const { return _ptr >= other._ptr; }
	inline bool operator >= (const Base * other) const { return _ptr->get() >= other; }
	inline bool operator >= (const std::nullptr_t other) const { return _ptr >= other; }

	inline bool operator <= (const Self & other) const { return _ptr <= other._ptr; }
	inline bool operator <= (const Base * other) const { return _ptr->get() <= other; }
	inline bool operator <= (const std::nullptr_t other) const { return _ptr <= other; }

#if SP_REF_DEBUG
	uint64_t getId() const { return _id; }
#endif
private:
	inline void doRetain() {
#if SP_REF_DEBUG
		if (_ptr) { _id = _ptr->retain(); }
#else
		if (_ptr) { _ptr->retain(); }
#endif
	}

	inline void doRelease() {
#if SP_REF_DEBUG
		if (_ptr) { _ptr->release(_id); }
#else
		if (_ptr) { _ptr->release(0); }
#endif
	}

	inline Pointer doSwap(Pointer value) {
#if SP_REF_DEBUG
		uint64_t id = 0;
		if (value) { id = value->retain(); }
		if (_ptr) { _ptr->release(_id); }
		_id = id;
		return value;
#else
		if (value) { value->retain(); }
		if (_ptr) { _ptr->release(0); }
		return value;
#endif
	}

	// unsafe
	inline SharedRc(Pointer value, bool v) : _ptr(value) { }

	Pointer _ptr = nullptr;
#if SP_REF_DEBUG
	uint64_t _id = 0;
#endif
};

template <typename T>
template <typename ...Args>
auto Shared<T>::create(Args && ... args) -> Shared * {
	auto pool = pool::create((pool_t *)nullptr);

	Shared *shared = nullptr;
	web::perform([&] {
		shared = new (pool) Shared(SharedMode::Pool, nullptr, pool,
			new (pool) T(pool, std::forward<Args>(args)...));
	}, pool);
	return shared;
}

template <typename T>
template <typename ...Args>
auto Shared<T>::create(pool_t *p, Args && ... args) -> Shared * {
	auto pool = pool::create(p);

	Shared *shared = nullptr;
	web::perform([&] {
		shared = new (pool) Shared(SharedMode::Pool, nullptr, pool,
			new (pool) T(pool, std::forward<Args>(args)...));
	}, pool);
	return shared;
}

template <typename T>
template <typename ...Args>
auto Shared<T>::create(SharedMode mode, Args && ... args) -> Shared * {
	allocator_t *alloc = nullptr;
	pool_t *pool = nullptr;

	switch (mode) {
	case SharedMode::Pool:
		pool = pool::create((pool_t *)nullptr);
		break;
	case SharedMode::Allocator:
		alloc = allocator::create();
		pool = pool::create(alloc);
		break;
	}

	Shared *shared = nullptr;
	web::perform([&] {
		shared = new (pool) Shared(mode, alloc, pool,
			new (pool) T(pool, std::forward<Args>(args)...));
	}, pool);
	return shared;
}

template <typename T>
Shared<T>::~Shared() {
	if (_shared) {
		web::perform([&, this] {
			delete _shared;
		}, _pool);
		_shared = nullptr;
	}
	if (_pool) {
		pool::destroy(_pool);
		_pool = nullptr;
	}
	if (_allocator) {
		allocator::destroy(_allocator);
		_allocator = nullptr;
	}
}

template <typename T>
Shared<T>::Shared(SharedMode m, allocator_t *alloc, pool_t *pool, T *obj)
: _allocator(alloc), _pool(pool), _shared(obj), _mode(m) { }

}

#endif /* EXTRA_WEBSERVER_WEBSERVER_UTILS_SPWEBASYNCTASK_H_ */
