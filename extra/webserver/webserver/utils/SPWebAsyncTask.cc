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

#include "SPWebAsyncTask.h"

namespace STAPPLER_VERSIONIZED stappler::web {

thread_local AsyncTaskGroup *tl_currentGroup = nullptr;
thread_local AsyncTask *tl_currentTask = nullptr;

AsyncTaskGroup *AsyncTaskGroup::getCurrent() {
	return tl_currentGroup;
}

AsyncTaskGroup::AsyncTaskGroup() : _host(Host::getCurrent()) { }

AsyncTaskGroup::AsyncTaskGroup(const Host &serv) : _host(serv) { }

AsyncTaskGroup::AsyncTaskGroup(const Host &serv, std::function<void()> &&fn)
: _host(serv), _notifyFn(move(fn)) { }

void AsyncTaskGroup::onAdded(AsyncTask *task) {
	++ _added;

	if (std::this_thread::get_id() == _threadId) {
		if (Time::now() - _lastUpdate > TimeInterval::microseconds(1000 * 50)) {
			update();
		}
	}
}

void AsyncTaskGroup::onPerformed(AsyncTask *task) {
	_mutex.lock();
	_queue.push_back(task);
	_mutex.unlock();

	std::function<void()> tmp;
	if (_notifyFn) {
		tmp = _notifyFn;
	}

	_condition.notify_one();

	if (tmp) {
		tmp();
	}
}

void AsyncTaskGroup::update() {
	_mutex.lock();

	std::vector<AsyncTask *> stack;
	stack.swap(_queue);

	_mutex.unlock();

	if (stack.empty()) {
		return;
	}

	for (auto task : stack) {
		task->onComplete();
		++ _completed;

		AsyncTask::destroy(task);
	}

	_lastUpdate = Time::now();
}

void AsyncTaskGroup::waitForAll() {
	update();
	while (_added != _completed) {
		std::unique_lock<std::mutex> lock(_condMutex);
		_condition.wait_for(lock, std::chrono::microseconds(1000 * 50));
		update();
	}
}

bool AsyncTaskGroup::perform(const Callback<void(AsyncTask &)> &cb) {
	return AsyncTask::perform(_host, cb, this);
}

Pair<size_t, size_t> AsyncTaskGroup::getCounters() const {
	return stappler::pair(_completed.load(), _added.load());
}

AsyncTask *AsyncTask::prepare(pool_t *rootPool, const Callback<void(AsyncTask &)> &cb, AsyncTaskGroup *g) {
	if (rootPool) {
		if (auto p = pool::create(rootPool)) {
			AsyncTask * task = nullptr;
			web::perform([&] {
				task = new (p) AsyncTask(p, g);
				cb(*task);
			}, p);
			return task;
		}
	}
	return nullptr;
}

AsyncTask *AsyncTask::prepare(const Callback<void(AsyncTask &)> &cb, AsyncTaskGroup *g) {
	if (auto serv = Host::getCurrent()) {
		return prepare(serv.getThreadPool(), cb, g);
	}
	return nullptr;
}

bool AsyncTask::perform(const Host &serv, const Callback<void(AsyncTask &)> &cb, AsyncTaskGroup *g) {
	if (serv) {
		if (auto t = prepare(serv.getThreadPool(), cb, g)) {
			return serv.performTask(t);
		}
	}
	return false;
}

bool AsyncTask::perform(const Callback<void(AsyncTask &)> &cb, AsyncTaskGroup *g) {
	if (auto serv = Host::getCurrent()) {
		return perform(Host(serv), cb, g);
	}
	return false;
}

void AsyncTask::destroy(AsyncTask *t) {
	auto p = t->pool();
	delete t;
	pool::destroy(p);
}

void AsyncTask::run(AsyncTask *t) {
	tl_currentTask = t;
	if (auto g = t->getGroup()) {
		tl_currentGroup = g;
	}
	web::perform([&] {
		t->setSuccessful(t->execute());
		if (!t->getGroup()) {
			t->onComplete();
		}
	}, t->pool(), config::TAG_HOST, t->getHost().getController());
	tl_currentTask = nullptr;
}

AsyncTask *AsyncTask::getCurrent() {
	return tl_currentTask;
}

void AsyncTask::addExecuteFn(const ExecuteCallback &cb) {
	web::perform([&, this] {
		_execute.push_back(cb);
	}, _pool);
}

void AsyncTask::addExecuteFn(ExecuteCallback &&cb) {
	web::perform([&, this] {
		_execute.push_back(std::move(cb));
	}, _pool);
}

void AsyncTask::addCompleteFn(const CompleteCallback &cb) {
	web::perform([&, this] {
		_complete.push_back(cb);
	}, _pool);
}
void AsyncTask::addCompleteFn(CompleteCallback &&cb) {
	web::perform([&, this] {
		_complete.push_back(std::move(cb));
	}, _pool);
}

void AsyncTask::performWithStorage(const Callback<void(const db::Transaction &)> &cb) const {
	_host.performWithStorage(cb);
}

bool AsyncTask::execute() {
	bool success = true;
	for (auto &it : _execute) {
		if (!it(*this)) {
			success = false;
		}
	}
	return success;
}
void AsyncTask::onComplete() {
	for (auto &it : _complete) {
		it(*this, _isSuccessful);
	}
}

AsyncTask::AsyncTask(pool_t *p, AsyncTaskGroup *g) : _pool(p), _group(g) {
	if (!p) {
		abort();
	}
}

}
