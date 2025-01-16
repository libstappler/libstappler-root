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

#include "SPWebHost.h"

namespace STAPPLER_VERSIONIZED stappler::web {

class Host;
class AsyncTask;

class SP_PUBLIC AsyncTaskGroup : public AllocBase {
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

class SP_PUBLIC AsyncTask : public AllocBase {
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

}

#endif /* EXTRA_WEBSERVER_WEBSERVER_UTILS_SPWEBASYNCTASK_H_ */
