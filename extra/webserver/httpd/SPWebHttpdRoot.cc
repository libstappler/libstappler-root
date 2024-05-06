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

#include "SPWebHttpdRoot.h"
#include "SPWebHttpdHost.h"
#include "SPWebHttpdRequest.h"
#include "SPWebHttpdFilters.h"
#include "SPWebHttpdWebsocket.h"
#include "SPWebAsyncTask.h"
#include "SPLog.h"

#include <apr_strings.h>
#include <http_log.h>

APLOG_USE_MODULE(stappler_web);

namespace STAPPLER_VERSIONIZED stappler::web {

static void __log2(log::LogType lt, const StringView &tag, const StringView &str) {
	int logLevel = 0;
	switch (lt) {
	case log::LogType::Verbose: logLevel = APLOG_NOTICE; break;
	case log::LogType::Debug: logLevel = APLOG_DEBUG; break;
	case log::LogType::Info: logLevel = APLOG_INFO; break;
	case log::LogType::Warn: logLevel = APLOG_WARNING; break;
	case log::LogType::Error: logLevel = APLOG_ERR; break;
	case log::LogType::Fatal: logLevel = APLOG_CRIT; break;
	}

	if (auto r = Request::getCurrent()) {
		auto req = ((HttpdRequestController *)r.getController())->getRequest();

		ap_log_rerror(APLOG_MARK, logLevel, 0, req, "%s: %.*s", tag.data(), int(str.size()), str.data());

		return;
	}

	if (auto s = Host::getCurrent()) {
		auto serv = ((HttpdHostController *)s.getController())->getServer();

		ap_log_error(APLOG_MARK, logLevel, 0, serv, "%s: %.*s", tag.data(), int(str.size()), str.data());

		return;
	}

	auto p = pool::acquire();
	ap_log_perror(APLOG_MARK, logLevel, 0, (apr_pool_t *)p, "%s: %.*s", tag.data(), int(str.size()), str.data());
}

static bool __log(log::LogType lt, const StringView &tag, log::CustomLog::Type t, log::CustomLog::VA &va) {
	if (t == log::CustomLog::Text) {
		if (!va.text.empty()) {
			__log2(lt, tag, va.text);
		}
	} else {
		auto pool = pool::acquire();
		auto str = ::apr_pvsprintf((apr_pool_t *)pool, va.format.format, va.format.args);
		__log2(lt, tag, str);
	}
	return true;
}

static log::CustomLog AprLog(&__log);

HttpdRoot::~HttpdRoot() { }

HttpdRoot::HttpdRoot(pool_t *pool)
: Root(pool) {
	registerCleanupDestructor(this, pool);
}

bool HttpdRoot::isSecureConnection(const Request &req) const {
	if (_sslIsHttps) {
		return _sslIsHttps(((HttpdRequestController *)req.getController())->getRequest()->connection);
	} else {
		return false;
	}
}

Status HttpdRoot::handlePostConfig(pool_t *pool, server_rec *s) {
	_configPool = pool;

	if (s->is_virtual == 0) {
		_rootServer = s;
	}

	return Status(OK);
}

void HttpdRoot::handleChildInit(pool_t *p, server_rec *s) {
	_rootServer = s;

	perform([&, this] {
		_sslIsHttps = APR_RETRIEVE_OPTIONAL_FN(ssl_is_https);

		_dbdOpen = APR_RETRIEVE_OPTIONAL_FN(ap_dbd_open);
		_dbdClose = APR_RETRIEVE_OPTIONAL_FN(ap_dbd_close);
		_dbdRequestAcquire = APR_RETRIEVE_OPTIONAL_FN(ap_dbd_acquire);
		_dbdConnectionAcquire = APR_RETRIEVE_OPTIONAL_FN(ap_dbd_cacquire);
	}, p);

	HttpdInputFilter::filterRegister();
	HttpdOutputFilter::filterRegister();
	HttpdWebsocketConnection::filterRegister();

	Root::handleChildInit(p);
}

struct TaskContext : AllocBase {
	AsyncTask *task = nullptr;
	Host host;

	TaskContext(AsyncTask *t, Host s) : task(t), host(s) { }
};

static void *HttpdRoot_performTask(apr_thread_t *, void *ptr) {
#if LINUX
	pthread_setname_np(pthread_self(), "RootWorker");
#endif

	auto ctx = (TaskContext *)ptr;
	AsyncTask::run(ctx->task);
	if (!ctx->task->getGroup()) {
		AsyncTask::destroy(ctx->task);
	} else {
		ctx->task->getGroup()->onPerformed(ctx->task);
	}
	return nullptr;
}

bool HttpdRoot::performTask(const Host &host, AsyncTask *task, bool performFirst) {
	if (task) {
		if (_threadPool) {
			task->setHost(host);
			if (auto g = task->getGroup()) {
				g->onAdded(task);
			}
			auto ctx = new (task->pool()) TaskContext( task, host );
			if (performFirst) {
				return apr_thread_pool_top(_threadPool, &HttpdRoot_performTask, ctx, apr_byte_t(task->getPriority()), nullptr) == APR_SUCCESS;
			} else {
				return apr_thread_pool_push(_threadPool, &HttpdRoot_performTask, ctx, apr_byte_t(task->getPriority()), nullptr) == APR_SUCCESS;
			}
		} else if (_pending) {
			perform([&, this] {
				_pending->emplace_back(PendingTask{host, task, TimeInterval(), performFirst});
			}, _pending->get_allocator());
		}
	}
	return false;
}

bool HttpdRoot::scheduleTask(const Host &host, AsyncTask *task, TimeInterval interval) {
	if (_threadPool && task) {
		if (_threadPool) {
			task->setHost(host);
			if (auto g = task->getGroup()) {
				g->onAdded(task);
			}
			auto ctx = new (task->pool()) TaskContext( task, host );
			return apr_thread_pool_schedule(_threadPool, &HttpdRoot_performTask, ctx, interval.toMicroseconds(), nullptr) == APR_SUCCESS;
		} else if (_pending) {
			perform([&, this] {
				_pending->emplace_back(PendingTask{host, task, interval, false});
			}, _pending->get_allocator());
		}
	}
	return false;
}

void HttpdRoot::foreachHost(const Callback<void(Host &)> &cb) {
	auto serv = _rootServer;
	while (serv) {
		auto h = Host(HttpdHostController::get(serv));
		cb(h);
		serv = serv->next;
	}
}

db::sql::Driver::Handle HttpdRoot::dbdOpen(pool_t *pool, const Host &host) {
	if (_dbdOpen) {
		auto s = static_cast<HttpdHostController *>(host.getController())->getServer();
		auto ret = _dbdOpen((apr_pool_t *)pool, s);
		if (!ret) {
			debug("Root", "Failed to open DBD", Value(host.getHostInfo().documentRoot));
		}
		return db::sql::Driver::Handle(ret);
	}
	return db::sql::Driver::Handle(nullptr);
}

void HttpdRoot::dbdClose(const Host &host, db::sql::Driver::Handle d) {
	if (_dbdClose) {
		auto s = static_cast<HttpdHostController *>(host.getController())->getServer();
		return _dbdClose(s, (ap_dbd_t *)d.get());
	}
}

db::sql::Driver::Handle HttpdRoot::dbdAcquire(const Request &req) {
	if (_dbdRequestAcquire) {
		auto r = ((HttpdRequestController *)req.getController())->getRequest();
		return db::sql::Driver::Handle(_dbdRequestAcquire(r));
	}
	return db::sql::Driver::Handle(nullptr);
}

static std::atomic_flag s_timerExitFlag;

void *HttpdRoot_timerThread(apr_thread_t *self, void *data) {
#if LINUX
	pthread_setname_np(pthread_self(), "RootHeartbeat");
#endif

	auto pool = pool::create((pool_t *)nullptr);

	Root *serv = (Root *)data;
	apr_sleep(config::HEARTBEAT_PAUSE.toMicroseconds());

	while (s_timerExitFlag.test_and_set()) {
		apr_sleep(config::HEARTBEAT_TIME.toMicroseconds());
		serv->handleHeartbeat(pool);
		pool::clear(pool);
	}

	pool::destroy(pool);

	return NULL;
}

void HttpdRoot::initThreads() {
	if (apr_thread_pool_create(&_threadPool, _initThreads, _maxThreads, (apr_pool_t *)_rootPool) == APR_SUCCESS) {
		apr_thread_pool_idle_wait_set(_threadPool, (5_sec).toMicroseconds());
		apr_thread_pool_threshold_set(_threadPool, 2);

		if (!_pending->empty()) {
			for (auto &it : *_pending) {
				if (it.interval) {
					scheduleTask(it.host, it.task, it.interval);
				} else {
					performTask(it.host, it.task, it.performFirst);
				}
			}
			_pending->clear();
		}
	} else {
		_threadPool = nullptr;
	}

	s_timerExitFlag.test_and_set();
	apr_threadattr_t *attr;
	apr_status_t error = apr_threadattr_create(&attr, (apr_pool_t *)_rootPool);
	if (error == APR_SUCCESS) {
		apr_threadattr_detach_set(attr, 1);
		apr_thread_create(&_timerThread, attr,
				HttpdRoot_timerThread, this, (apr_pool_t *)_rootPool);
	}
}

}
