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

#ifndef EXTRA_WEBSERVER_WEBSERVER_SERVER_SPWEBROOT_H_
#define EXTRA_WEBSERVER_WEBSERVER_SERVER_SPWEBROOT_H_

#include "SPWeb.h"
#include "SPWebHost.h"
#include "SPSqlDriver.h"

namespace stappler::web {

class AsyncTask;
class Host;
class InputFilter;
class RequestController;

class Root : public db::ApplicationInterface, public AllocBase {
public:
	struct ErrorNotificator : public AllocBase {
		Function<void(Value &&)> error;
		Function<void(Value &&)> debug;
	};

	static constexpr const char *ErrorNotificatorKey = "web::Root.ErrorNotificator";

	static Root *getCurrent();

	static void parseParameterList(Map<StringView, StringView> &target, StringView params);

	static void setErrorNotification(pool_t *, Function<void(Value &&)> errorCb, Function<void(Value &&)> debugCb = nullptr);

	virtual ~Root();

	Root(pool_t *);

	StringView getServerNameLine() const { return _serverNameLine; }

	void setProcPool(pool_t *);
	pool_t *getProcPool() const;

	bool isDebugEnabled() const { return _debug.load(); }
	void setDebugEnabled(bool);

	virtual bool performTask(const Host &, AsyncTask *task, bool performFirst) = 0;
	virtual bool scheduleTask(const Host &, AsyncTask *task, TimeInterval) = 0;

	virtual void foreachHost(const Callback<void(Host &)> &) = 0;

	Host getRootHost() const;

	virtual db::sql::Driver * getDbDriver(StringView);

	virtual db::sql::Driver::Handle dbdOpen(pool_t *, const Host &);
	virtual void dbdClose(const Host &, db::sql::Driver::Handle);

	void setThreadsCount(StringView, StringView);

	void handleBroadcast(const Value &);

	Status runTypeChecker(Request &r);
	Status runPostReadRequest(Request &r);
	Status runTranslateName(Request &r);
	Status runCheckAccess(Request &r);
	Status runQuickHandler(Request &r, int v);
	void runInsertFilter(Request &r);
	Status runHandler(Request &r);

	void handleFilterInit(InputFilter *f);
	void handleFilterUpdate(InputFilter *f);
	void handleFilterComplete(InputFilter *f);

	virtual db::Adapter getAdapterFromContext() const override;
	virtual void scheduleAyncDbTask(const Callback<Function<void(const db::Transaction &)>(pool_t *)> &setupCb) const override;

	virtual StringView getDocuemntRoot() const override;

	virtual const db::Scheme *getFileScheme() const override;
	virtual const db::Scheme *getUserScheme() const override;

	virtual void pushErrorMessage(Value &&) const override;
	virtual void pushDebugMessage(Value &&) const override;

	virtual db::InputFile *getFileFromContext(int64_t id) const override;
	virtual int64_t getUserIdFromContext() const override;

	virtual db::RequestData getRequestData() const override;

	virtual void initTransaction(db::Transaction &) const override;

protected:
	struct PendingTask {
		Host host;
		AsyncTask *task;
		TimeInterval interval = nullptr;
		bool performFirst;
	};

	void initExtensions();

	StringView findTypeCheckerContentType(Request &r, StringView ext) const;
	StringView findTypeCheckerCharset(Request &r, StringView ext) const;
	StringView findTypeCheckerContentLanguage(Request &r, StringView ext) const;
	StringView findTypeCheckerContentEncoding(Request &r, StringView ext) const;

	pool_t *_workerPool = nullptr;
	pool_t *_rootPool = nullptr;

	StringView _serverNameLine = config::DEFAULT_SERVER_LINE;

	Host _rootHost;

	std::atomic<uint64_t> _requestsRecieved;
	std::atomic<uint64_t> _filtersInit;
	std::atomic<uint64_t> _tasksRunned;
	std::atomic<uint64_t> _heartbeatCounter;

	std::atomic<bool> _debug = false;

	size_t _initThreads = std::thread::hardware_concurrency() / 2;
	size_t _maxThreads = std::thread::hardware_concurrency();

	Vector<PendingTask> *_pending = nullptr;

	Map<StringView, StringView> _extensionTypes;
	Map<StringView, StringView> _dbParams;
	Map<StringView, db::sql::Driver *> _dbDrivers;
	Vector<StringView> *_dbs = nullptr;
	db::sql::Driver *_primaryDriver = nullptr;

	Mutex _mutex;
	memory::allocator_t *_allocator = nullptr;
};

}

#endif /* EXTRA_WEBSERVER_WEBSERVER_SERVER_SPWEBROOT_H_ */
