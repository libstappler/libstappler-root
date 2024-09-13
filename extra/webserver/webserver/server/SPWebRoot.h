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

namespace STAPPLER_VERSIONIZED stappler::web {

class AsyncTask;
class Host;
class InputFilter;
class RequestController;

class SP_PUBLIC Root : public db::ApplicationInterface, public AllocBase {
public:
	struct Stat {
		uint64_t requestsReceived = 0;
		uint64_t heartbeatCounter = 0;
		uint64_t dbQueriesReleased = 0;
		uint64_t dbQueriesPerformed = 0;
	};

	struct ErrorNotificator : public AllocBase {
		Function<void(Value &&)> error;
		Function<void(Value &&)> debug;
	};

	static constexpr const char *ErrorNotificatorKey = "web::Root.ErrorNotificator";

	static Root *getCurrent();

	static void parseParameterList(Map<StringView, StringView> &target, StringView params);

	static void setErrorNotification(pool_t *, Function<void(Value &&)> errorCb, Function<void(Value &&)> debugCb = nullptr);

	static void dumpCurrentState(StringView filepath = StringView());

	virtual ~Root();

	Root(pool_t *);

	Stat getStat() const;

	StringView getServerNameLine() const { return _serverNameLine; }

	bool isDebugEnabled() const { return _debug.load(); }
	void setDebugEnabled(bool);

	virtual bool performTask(const Host &, AsyncTask *task, bool performFirst) = 0;
	virtual bool scheduleTask(const Host &, AsyncTask *task, TimeInterval) = 0;

	virtual void foreachHost(const Callback<void(Host &)> &) = 0;

	virtual bool isSecureConnection(const Request &) const;

	virtual db::sql::Driver * getDbDriver(StringView);

	virtual db::sql::Driver::Handle dbdOpen(pool_t *, const Host &);
	virtual void dbdClose(const Host &, db::sql::Driver::Handle);
	virtual db::sql::Driver::Handle dbdAcquire(const Request &);

	void addDb(StringView);
	void setDbParams(StringView);
	void setThreadsCount(StringView, StringView);

	virtual void handleHeartbeat(pool_t *pool);
	virtual void handleBroadcast(const Value &);

	virtual void handleChildInit(pool_t *);

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

	virtual StringView getDocumentRoot() const override;

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

	StringView findTypeCheckerContentType(Request &r, StringView ext) const;
	StringView findTypeCheckerCharset(Request &r, StringView ext) const;
	StringView findTypeCheckerContentLanguage(Request &r, StringView ext) const;
	StringView findTypeCheckerContentEncoding(Request &r, StringView ext) const;

	virtual void initDatabases();
	virtual void initSignals();
	virtual void initThreads();

	db::sql::Driver *createDbDriver(StringView driverName);

	pool_t *_workerPool = nullptr;
	pool_t *_configPool = nullptr;
	pool_t *_rootPool = nullptr;

	size_t _initThreads = std::thread::hardware_concurrency() / 2;
	size_t _maxThreads = std::thread::hardware_concurrency();

	StringView _serverNameLine;

	std::atomic<uint64_t> _requestsReceived = 0;
	std::atomic<uint64_t> _heartbeatCounter = 0;
	std::atomic<uint64_t> _dbQueriesReleased = 0;
	std::atomic<uint64_t> _dbQueriesPerformed = 0;

	std::atomic<bool> _debug = false;

	Vector<PendingTask> *_pending = nullptr;

	Map<StringView, StringView> _dbParams;
	Map<StringView, db::sql::Driver *> _dbDrivers;
	Vector<StringView> _dbs;
	db::sql::Driver *_primaryDriver = nullptr;

	Mutex _mutex;
	memory::allocator_t *_allocator = nullptr;
};

}

#endif /* EXTRA_WEBSERVER_WEBSERVER_SERVER_SPWEBROOT_H_ */
