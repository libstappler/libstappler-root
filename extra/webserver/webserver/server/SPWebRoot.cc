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

#include "SPWebRoot.h"
#include "SPWebInputFilter.h"
#include "SPWebRequest.h"
#include "SPWebRequestHandler.h"
#include "SPWebWebsocketConnection.h"

#include "SPPlatformUnistd.h"
#include "SPLog.h"

#if LINUX
#include <signal.h>
#endif

namespace STAPPLER_VERSIONIZED stappler::web {

static Root *getRootFromContext(pool_t *p, uint32_t tag, const void *ptr) {
	switch (tag) {
	case uint32_t(config::TAG_HOST): return ((HostController *)ptr)->getRoot(); break;
	case uint32_t(config::TAG_REQUEST): return ((RequestController *)ptr)->getHost()->getRoot(); break;
	case uint32_t(config::TAG_WEBSOCKET): return ((WebsocketConnection *)ptr)->getHost().getRoot(); break;
	}
	return nullptr;
}

Root *Root::getCurrent() {
	Root *ret = nullptr;
	pool::foreach_info(&ret, [] (void *ud, pool_t *p, uint32_t tag, const void *data) -> bool {
		auto ptr = getRootFromContext(p, tag, data);
		if (ptr) {
			*((Root **)ud) = ptr;
			return false;
		}
		return true;
	});

	return ret;
}

void Root::parseParameterList(Map<StringView, StringView> &target, StringView str) {
	StringView r(str);
	r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
	while (!r.empty()) {
		StringView params, n, v;
		if (r.is('"')) {
			++ r;
			params = r.readUntil<StringView::Chars<'"'>>();
			if (r.is('"')) {
				++ r;
			}
		} else {
			params = r.readUntil<StringView::CharGroup<CharGroupId::WhiteSpace>>();
		}

		if (!params.empty()) {
			n = params.readUntil<StringView::Chars<'='>>();
			++ params;
			v = params;

			if (!n.empty() && ! v.empty()) {
				target.emplace(n.pdup(target.get_allocator()), v.pdup(target.get_allocator()));
			}
		}

		r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
	}
}

void Root::setErrorNotification(pool_t *p, Function<void(Value &&)> errorCb, Function<void(Value &&)> debugCb) {
	perform([&] {
		ErrorNotificator *err = nullptr;
		pool::userdata_get((void **)&err, ErrorNotificatorKey, p);
		if (err) {
			err->error = move(errorCb);
			err->debug = move(debugCb);
		} else {
			err = new (p) ErrorNotificator;
			err->error = move(errorCb);
			err->debug = move(debugCb);
			pool::userdata_set(err, ErrorNotificatorKey, nullptr, p);
		}
	}, p);
}

void Root::dumpCurrentState(StringView filepath) {
	if (auto f = ::fopen(filepath.data(), "w+")) {
		if (auto host = Host::getCurrent()) {
			auto &hostInfo = host.getHostInfo();
			auto root = hostInfo.documentRoot;
			::fputs("Server:\n\tDocumentRoot: ", f);
			::fputs(root.data(), f);
			::fputs("\n\tName: ", f);
			::fputs(hostInfo.hostname.data(), f);
			::fputs("\n\tDate: ", f);
			::fputs(Time::now().toHttp<Interface>().data(), f);

			if (auto req = Request::getCurrent()) {
				auto &reqInfo = req.getInfo();
				::fputs("\nRequest:\n", f);
				::fprintf(f, "\tUrl: %s%s\n", reqInfo.url.host.data(), reqInfo.unparserUri.data());
				::fprintf(f, "\tRequest: %s\n", reqInfo.requestLine.data());
				::fprintf(f, "\tIp: %s\n", reqInfo.useragentIp.data());

				::fputs("\tHeaders:\n", f);
				req.foreachRequestHeaders([&] (StringView key, StringView value) {
					::fprintf(f, "\t\t%s: %s\n", key.data(), value.data());
				});
			}

			::fputs("\nBacktrace:\n", f);

			getBacktrace(2, [&] (StringView str) {
				::fprintf(f, "\t%s\n", str.data());
			});

			::fclose(f);
		}
	}
}

Root::~Root() {
	pool::destroy(_workerPool);
}

Root::Root(pool_t *p) : _rootPool(p) {
	_workerPool = pool::create(p);

	_serverNameLine = StringView(
			toString("Stappler/", getStapplerVersionString(), " ", "Webserver/", config::getWebserverVersionString())).pdup(_rootPool);

	initExtensions();
}

Root::Stat Root::getStat() const {
	return Stat{
		_requestsReceived.load(),
		_heartbeatCounter.load(),
		_dbQueriesReleased.load(),
		_dbQueriesPerformed.load()
	};
}

void Root::setDebugEnabled(bool val) {
	_debug = val;
}

bool Root::isSecureConnection(const Request &) const {
	return false;
}

db::sql::Driver * Root::getDbDriver(StringView driver) {
	auto it = _dbDrivers.find(driver);
	if (it != _dbDrivers.end()) {
		return it->second;
	} else {
		auto d = createDbDriver(driver);
		_dbDrivers.emplace(driver, d).first;
		return d;
	}
}

db::sql::Driver::Handle Root::dbdOpen(pool_t *, const Host &serv) {
	log::error("web::Root", "Root::dbdOpen is not implemented");
	return db::sql::Driver::Handle(nullptr);
}

void Root::dbdClose(const Host &serv, db::sql::Driver::Handle) {
	log::error("web::Root", "Root::dbdClose is not implemented");
}

db::sql::Driver::Handle Root::dbdAcquire(const Request &req) {
	log::error("web::Root", "Root::dbdAcquire is not implemented");
	return db::sql::Driver::Handle(nullptr);
}

Status Root::runPostReadRequest(Request &r) {
	return perform([&, this] {
		_requestsReceived += 1;
		//OutputFilter::insert(r);

		Request request(r);
		Host host = request.host();

		auto ret = host.handleRequest(request);
		if (ret > 0 || ret == DONE) {
			return ret;
		}

		RequestHandler *rhdl = request.getRequestHandler();
		if (rhdl) {
			return rhdl->onPostReadRequest(request);
		}

		return DECLINED;
	}, r.pool(), config::TAG_REQUEST, r.config());
}

Status Root::runTranslateName(Request &r) {
	return perform([&] () {
		Request request(r);

		RequestHandler *rhdl = request.getRequestHandler();
		if (rhdl) {
			if (!rhdl->isRequestPermitted(request)) {
				auto status = request.getInfo().status;
				if (status == 0 || status == 200) {
					return HTTP_FORBIDDEN;
				}
				return status;
			}
			auto res = rhdl->onTranslateName(request);
			if (res == DECLINED
					&& request.getInfo().method != RequestMethod::Post
					&& request.getInfo().method != RequestMethod::Put
					&& request.getInfo().method != RequestMethod::Patch
					&& request.getInfo().method != RequestMethod::Options) {
				request.setRequestHandler(nullptr);
			}
			return res;
		}

		return DECLINED;
	}, r.pool(), config::TAG_REQUEST, r.config());
}

Status Root::runCheckAccess(Request &r) {
	return perform([&] {
		Request request(r);
		RequestHandler *rhdl = request.getRequestHandler();
		if (rhdl) {
			return OK; // already checked by serenity
		}
		if (!request.getInfo().filename.empty()) {
			if (StringView(request.getInfo().filename).starts_with(StringView(request.getInfo().documentRoot))) {
				return OK;
			}
		}
		return DECLINED;
	}, r.pool(), config::TAG_REQUEST, r.config());
}

Status Root::runQuickHandler(Request &r, int v) {
	return perform([&] () -> Status {
		Request request(r);
		RequestHandler *rhdl = request.getRequestHandler();
		if (rhdl) {
			return rhdl->onQuickHandler(request, v);
		}
		return DECLINED;
	}, r.pool(), config::TAG_REQUEST, r.config());
}

void Root::runInsertFilter(Request &r) {
	perform([&] {
		Request request(r);

		RequestHandler *rhdl = request.getRequestHandler();
		if (rhdl) {
			rhdl->onInsertFilter(request);
		}
	}, r.pool(), config::TAG_REQUEST, r.config());
}

Status Root::runHandler(Request &r) {
	return perform([&] () -> Status {
		Request request(r);

		RequestHandler *rhdl = request.getRequestHandler();
		if (rhdl) {
			return rhdl->onHandler(request);
		}

		return DECLINED;
	}, r.pool(), config::TAG_REQUEST, r.config());
}

void Root::handleFilterInit(InputFilter *f) {
	RequestHandler *rhdl = f->getRequest().getRequestHandler();
	if (rhdl) {
		rhdl->onFilterInit(f);
	}
}
void Root::handleFilterUpdate(InputFilter *f) {
	RequestHandler *rhdl = f->getRequest().getRequestHandler();
	if (rhdl) {
		rhdl->onFilterUpdate(f);
	}
}
void Root::handleFilterComplete(InputFilter *f) {
	RequestHandler *rhdl = f->getRequest().getRequestHandler();
	if (rhdl) {
		rhdl->onFilterComplete(f);
	}
}

void Root::initExtensions() {
	StringView r(config::MIME_TYPES);
	while (!r.empty()) {
		auto str = r.readUntil<StringView::Chars<'\n', '\r'>>();
		r.skipChars<StringView::Chars<'\n', '\r'>>();
		if (!str.empty()) {
			auto type = str.readUntil<StringView::CharGroup<CharGroupId::WhiteSpace>>();
			str.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
			if (!str.empty()) {
				while (!str.empty()) {
					auto value = str.readUntil<StringView::CharGroup<CharGroupId::WhiteSpace>>();
					if (!value.empty()) {
						_extensionTypes.emplace(value, type);
					}
					str.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
				}
			}
		}
	}
}

void Root::addDb(StringView str) {
	perform([&, this] {
		emplace_ordered(_dbs, str.pdup(_rootPool));
	}, _rootPool);
}

void Root::setDbParams(StringView str) {
	perform([&, this] {
		parseParameterList(_dbParams, str);
	}, _rootPool);
}

void Root::setThreadsCount(StringView init, StringView max) {
	_initThreads = std::max(size_t(init.readInteger(10).get(1)), size_t(1));
	_maxThreads = std::max(size_t(max.readInteger(10).get(1)), _initThreads);
}

void Root::handleHeartbeat(pool_t *pool) {
	foreachHost([&] (Host &serv) {
		perform([&] {
			serv.handleHeartBeat(pool);
		}, pool, config::TAG_HOST, serv.getController());
	});
}

void Root::handleBroadcast(const Value &res) {
	if (res.getBool("system")) {
		auto &option = res.getString("option");
		if (!option.empty()) {
			if (option == "debug") {
				_debug = res.getBool("debug");
			}
		}
		return;
	} else if (res.getBool("local")) {
		foreachHost([&] (Host &serv) {
			perform([&] {
				serv.handleBroadcast(res);
			}, serv.getThreadPool(), config::TAG_HOST, serv.getController());
		});
	}
}

void Root::handleChildInit(pool_t *p) {
	perform([&, this] {
		initDatabases();

		initSignals();

		_pending = new Vector<PendingTask>();

		foreachHost([&, this] (Host &host) {
			host.handleChildInit(_configPool);
		});

		// start threads only after all initialization is done
		initThreads();
	}, p);
}

Status Root::runTypeChecker(Request &r) {
	auto &info = r.getInfo();
	if (info.stat.isDir) {
		r.setContentType(config::DIR_MIME_TYPE);
		return OK;
	}

	if (info.filename.empty()) {
		return DECLINED;
	}

	StringView contentType;
	StringView charset;
	Vector<StringView> contentEncoding;

	auto onTypeCheckerExtension = [&, this] (StringView ext) {
		auto ct = findTypeCheckerContentType(r, ext);
		if (!ct.empty()) {
			contentType = ct;
		}

		auto cs = findTypeCheckerCharset(r, ext);
		if (!cs.empty()) {
			charset = cs;
		}

		auto ce = findTypeCheckerContentEncoding(r, ext);
		if (!ce.empty()) {
			contentEncoding.emplace_back(ce);
		}
	};

	StringView fileName = filepath::lastComponent(info.filename);
	fileName.skipChars<StringView::Chars<'.'>>();

	auto tmp = fileName;
	tmp.skipUntil<StringView::Chars<'.'>>();
	if (tmp.size() < 2 || !tmp.is('.') ) {
		return DECLINED;
	}

	while (!tmp.empty()) {
		if (tmp.is('.')) {
			tmp.skipChars<StringView::Chars<'.'>>();
			auto ext = tmp.readUntil<StringView::Chars<'.'>>();
			if (!ext.empty()) {
				auto tmpStr = ext.str<Interface>();
				string::apply_tolower_c(tmpStr);
				onTypeCheckerExtension(tmpStr);
			}
		} else {
			tmp.skipUntil<StringView::Chars<'.'>>();
		}
	}

	if (!contentEncoding.empty()) {
	    if (info.contentEncoding.empty() && contentEncoding.size() == 1) {
	    	r.setContentEncoding(contentEncoding.front());
	    } else {
			StringStream stream;

			bool start = true;
			if (!info.contentEncoding.empty()) {
				stream << info.contentEncoding;
				start = false;
			}

			for (auto &it : contentEncoding) {
				if (start) {
					start = false;
				} else {
					stream << ", ";
				}
				stream << it;
			}

			r.setContentEncoding(stream.weak());
	    }
	}

    if (!contentType.empty()) {
    	auto v = extractCharset(contentType);
		if (!charset.empty()) {
			if (v.empty()) {
				StringStream stream;
				stream << contentType << "; charset=" << charset;
				r.setContentType(stream.weak());
			} else {
				StringView start = StringView(contentType, v.data() - contentType.data());
				StringView end = StringView(v.data() + v.size(), contentType.size() - (v.data() - contentType.data() + v.size()));

				StringStream stream;
				stream << start << charset << end;
				r.setContentType(stream.weak());
			}
		} else {
			r.setContentType(contentType);
		}
    }

    if (info.contentType.empty()) {
        return DECLINED;
    }

    return OK;
}

StringView Root::findTypeCheckerContentType(Request &r, StringView ext) const {
	auto it = _extensionTypes.find(ext);
	if (it != _extensionTypes.end()) {
		return it->second;
	}
	return StringView();
}

StringView Root::findTypeCheckerCharset(Request &r, StringView ext) const {
	return StringView();
}

StringView Root::findTypeCheckerContentLanguage(Request &r, StringView ext) const {
	return StringView();
}

StringView Root::findTypeCheckerContentEncoding(Request &r, StringView ext) const {
	return StringView();
}

void Root::initDatabases() {
	perform([&, this] {
		Map<db::sql::Driver *, Vector<StringView>> databases;

		if (!_dbParams.empty()) {
			StringView driver;
			StringView dbname;

			for (auto &it : _dbParams) {
				if (it.first == "driver") {
					driver = it.second;
				} else if (it.first == "dbname") {
					dbname = it.second;
				}
			}

			if (driver.empty()) {
				driver = StringView("pgsql");
			}

			if (auto d = createDbDriver(driver)) {
				_dbDrivers.emplace(driver, d);
				auto dit = databases.emplace(d, Vector<StringView>()).first;
				if (!dbname.empty()) {
					emplace_ordered(dit->second, dbname);
				}
				if (!_dbs.empty()) {
					for (auto &it : _dbs) {
						emplace_ordered(dit->second, it);
					}
				}
				_primaryDriver = d;
			}
		}

		foreachHost([&, this] (Host &host) {
			auto config = host.getController();
			if (!config->getDbParams().empty()) {
				StringView driver;
				StringView dbname;
				for (auto &it : config->getDbParams()) {
					if (it.first == "driver") {
						driver = it.second;
					} else if (it.first == "dbname") {
						dbname = it.second;
					}
				}

				if (!driver.empty()) {
					auto iit = _dbDrivers.find(driver);
					if (iit == _dbDrivers.end()) {
						if (auto d = createDbDriver(driver)) {
							iit = _dbDrivers.emplace(driver, d).first;
						}
					}

					if (!dbname.empty()) {
						auto dit = databases.find(iit->second);
						if (dit != databases.end()) {
							emplace_ordered(dit->second, dbname);
						}
					}
				}
			}
		});

		if (_primaryDriver && !_dbParams.empty()) {
			bool init = false;
			auto dIt = databases.find(_primaryDriver);
			if (dIt != databases.end()) {
				if (!dIt->second.empty()) {
					auto handle = _primaryDriver->connect(_dbParams);
					if (handle.get()) {
						_primaryDriver->init(handle, dIt->second);
						_primaryDriver->finish(handle);
						init = true;
					}
				}
			}
			if (!init) {
				auto handle = _primaryDriver->connect(_dbParams);
				if (handle.get()) {
					_primaryDriver->init(handle, Vector<StringView>());
					_primaryDriver->finish(handle);
					init = true;
				}
			}
		}
	}, _workerPool);
	pool::clear(_workerPool);
}

#if LINUX
static struct sigaction s_sharedSigAction;
static struct sigaction s_sharedSigOldAction;

static void s_sigAction(int sig, siginfo_t *info, void *ucontext) {
	if (auto host = Host::getCurrent()) {
		auto &hostInfo = host.getHostInfo();
		auto root = hostInfo.documentRoot;
		Root::dumpCurrentState(toString(root, "/.reports/crash.", Time::now().toMicros(), ".txt"));
	}

	if ((s_sharedSigOldAction.sa_flags & SA_SIGINFO) != 0) {
		if (s_sharedSigOldAction.sa_sigaction) {
			s_sharedSigOldAction.sa_sigaction(sig, info, ucontext);
		}
	} else {
		if (s_sharedSigOldAction.sa_handler == SIG_DFL) {
			if (SIGURG == sig || SIGWINCH == sig || SIGCONT == sig) return;

			static struct sigaction tmpSig;
			tmpSig.sa_handler = SIG_DFL;
			::sigemptyset(&tmpSig.sa_mask);
		    ::sigaction(sig, &tmpSig, nullptr);
		    ::kill(getpid(), sig);
			::sigaction(sig, &s_sharedSigAction, nullptr);
		} else if (s_sharedSigOldAction.sa_handler == SIG_IGN) {
			return;
		} else if (s_sharedSigOldAction.sa_handler) {
			s_sharedSigOldAction.sa_handler(sig);
		}
	}
}
#endif

void Root::initSignals() {
#if LINUX
	::setenv("GNUTLS_NO_IMPLICIT_INIT", "1", 0);

	memset(&s_sharedSigAction, 0, sizeof(s_sharedSigAction));
	s_sharedSigAction.sa_sigaction = &s_sigAction;
	s_sharedSigAction.sa_flags = SA_SIGINFO;
	sigemptyset(&s_sharedSigAction.sa_mask);
	//sigaddset(&s_sharedSigAction.sa_mask, SIGSEGV);

    ::sigaction(SIGSEGV, &s_sharedSigAction, &s_sharedSigOldAction);
#endif
}

void Root::initThreads() {

}

db::sql::Driver *Root::createDbDriver(StringView driverName) {
	if (auto d = db::sql::Driver::open(_rootPool, this, driverName)) {
		d->setDbCtrl([this] (bool complete) {
			if (complete) {
				_dbQueriesReleased += 1;
			} else {
				_dbQueriesPerformed += 1;
			}
		});
		return d;
	}

	log::error("web::Root", "Fail to initialize driver: ", driverName);
	return nullptr;
}

void Root::pushErrorMessage(Value &&val) const {
	if (auto serv = Host::getCurrent()) {
		serv.reportError(val);
	}

	if (isDebugEnabled()) {
		Value bcast{
			std::make_pair("message", Value(true)),
			std::make_pair("level", Value("error")),
			std::make_pair("data", Value(val)),
		};
		if (auto a = db::Adapter::FromContext(this)) {
			a.broadcast(bcast);
		}
	}

#if DEBUG
	log::error("web::Root", data::EncodeFormat::Pretty, val);
#endif

	if (auto req = Request::getCurrent()) {
		req.addErrorMessage(move(val));
		return;
	}

	auto pool = pool::acquire();
	ErrorNotificator *err = nullptr;
	pool::userdata_get((void **)&err, ErrorNotificatorKey, pool);
	if (err && err->error) {
		err->error(std::move(val));
	}

	log::error("web::Root", "Unhandled error: ", data::EncodeFormat::Pretty, val);
}

void Root::pushDebugMessage(Value &&val) const {
	if (auto serv = Host::getCurrent()) {
		serv.reportError(val);
	}

	if (isDebugEnabled()) {
		Value bcast{
			std::make_pair("message", Value(true)),
			std::make_pair("level", Value("debug")),
			std::make_pair("data", Value(val)),
		};
		if (auto a = db::Adapter::FromContext(this)) {
			a.broadcast(bcast);
		}
	}

#if DEBUG
	log::error("web::Root", data::EncodeFormat::Pretty, val);
#endif

	if (auto req = Request::getCurrent()) {
		req.addDebugMessage(move(val));
		return;
	}

	auto pool = pool::acquire();
	ErrorNotificator *err = nullptr;
	pool::userdata_get((void **)&err, ErrorNotificatorKey, pool);
	if (err && err->debug) {
		err->debug(std::move(val));
		return;
	}

	log::debug("web::Root", "Unhandled debug message: ", data::EncodeFormat::Pretty, val);
}

db::Adapter Root::getAdapterFromContext() const {
	if (auto p = pool::acquire()) {
		db::BackendInterface *h = nullptr;
		stappler::memory::pool::userdata_get((void **)&h, db::config::STORAGE_INTERFACE_KEY.data(), p);
		if (h) {
			return db::Adapter(h, this);
		}
	}

	if (auto req = Request::getCurrent()) {
		return req.getController()->acquireDatabase();
	}
	return db::Adapter(nullptr, nullptr);
}

void Root::scheduleAyncDbTask(const Callback<Function<void(const db::Transaction &)>(pool_t *)> &setupCb) const {
	if (auto serv = Host::getCurrent()) {
		AsyncTask::perform(serv, [&] (AsyncTask &task) {
			auto cb = setupCb(task.pool());
			task.addExecuteFn([cb = std::move(cb)] (const AsyncTask &task) -> bool {
				task.performWithStorage([&] (const db::Transaction &t) {
					t.performAsSystem([&] () -> bool {
						cb(t);
						return true;
					});
				});
				return true;
			});
		});
	}
}

StringView Root::getDocuemntRoot() const {
	if (auto req = Request::getCurrent()) {
		return req.getInfo().documentRoot;
	}

	if (auto serv = Host::getCurrent()) {
		return serv.getHostInfo().documentRoot;
	}

	return StringView();
}

const db::Scheme *Root::getFileScheme() const {
	if (auto serv = Host::getCurrent()) {
		return serv.getFileScheme();
	}
	return nullptr;
}

const db::Scheme *Root::getUserScheme() const {
	if (auto serv = Host::getCurrent()) {
		return serv.getUserScheme();
	}
	return nullptr;
}

db::RequestData Root::getRequestData() const {
	db::RequestData ret;

	if (auto req = Request::getCurrent()) {
		ret.exists = true;
		ret.address = req.getInfo().useragentIp;
		ret.hostname = req.getInfo().url.host;
		ret.uri = req.getInfo().unparserUri;
	}

	return ret;
}

void Root::initTransaction(db::Transaction &t) const {
	if (auto req = Request::getCurrent()) {
		t.setRole(req.getAccessRole());
	} else {
		t.setRole(db::AccessRoleId::Nobody);
	}

	if (auto serv = Host::getCurrent()) {
		serv.initTransaction(t);
	}
}

db::InputFile *Root::getFileFromContext(int64_t id) const {
	return InputFilter::getFileFromContext(id);
}

int64_t Root::getUserIdFromContext() const {
	if (auto req = Request::getCurrent()) {
		if (auto user = req.getAuthorizedUser()) {
			return user->getObjectId();
		} else {
			return req.getUserId();
		}
	}
	return 0;
}

}
