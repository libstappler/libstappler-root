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

#include "SPWebDbd.h"
#include "SPWebRoot.h"

namespace STAPPLER_VERSIONIZED stappler::web {

struct DbConnection : public AllocBase {
	db::sql::Driver::Handle handle;
	Time ctime;
	DbConnection *next = nullptr;
};

struct DbConnList : public AllocBase {
	DbConnection *opened = nullptr;
	DbConnection *free = nullptr;
	uint32_t count = 0;

	Mutex mutex;

	pool_t *pool = nullptr;
	db::sql::Driver *driver = nullptr;
	DbdModule::Config config;
	Map<StringView, StringView> params;

	DbConnList(pool_t *p, db::sql::Driver *d, DbdModule::Config cfg, Map<StringView, StringView> &&pp);
	~DbConnList();

	db::sql::Driver::Handle connect(bool toClose);
	void disconnect(db::sql::Driver::Handle);

	db::sql::Driver::Handle open();
	void close(db::sql::Driver::Handle h);

	void cleanup(const std::unique_lock<Mutex> &lock);

	void finalize();
};

DbConnList::DbConnList(pool_t *p, db::sql::Driver *d, DbdModule::Config cfg, Map<StringView, StringView> &&pp)
: pool(p), driver(d), config(cfg), params(sp::move(pp)) { }

DbConnList::~DbConnList() { }

db::sql::Driver::Handle DbConnList::connect(bool toClose) {
	db::sql::Driver::Handle ret(nullptr);

	perform([&, this] {
		ret = driver->connect(params);
	}, pool);

	if (ret.get()) {
		auto key = toString("dbd", uintptr_t(ret.get()));
		pool::store(ret.get(), key, [this, ret, toClose] () {
			if (toClose) {
				close(ret);
			} else {
				driver->finish(ret);
			}
		});
	}
	return ret;
}

void DbConnList::disconnect(db::sql::Driver::Handle h) {
	if (h.get()) {
		auto key = toString("dbd", uintptr_t(h.get()));
		pool::store(h.get(), key, nullptr);
	}

	if (!config.persistent) {
		perform([&, this] {
			driver->finish(h);
		}, pool);
	}
}

db::sql::Driver::Handle DbConnList::open() {
	db::sql::Driver::Handle ret(nullptr);

	std::unique_lock<Mutex> lock(mutex);

	cleanup(lock);

	while (opened) {
		ret = opened->handle;

		auto tmp = opened;
		tmp->handle = db::sql::Driver::Handle(nullptr);
		opened = tmp->next;

		tmp->next = free;
		free = tmp;

		-- count;

		auto conn = driver->getConnection(ret);
		if (driver->isValid(conn)) {
			break;
		} else {
			perform([&, this] {
				driver->finish(ret);
			}, pool);
			ret = db::sql::Driver::Handle(nullptr);
		}
	}
	lock.unlock();

	if (ret.get()) {
		return ret;
	} else {
		return connect(true);
	}
}

void DbConnList::close(db::sql::Driver::Handle h) {
	disconnect(h);

	if (h.get()) {
		auto key = toString("dbd", uintptr_t(h.get()));
		pool::store(h.get(), key, nullptr);
	}

	auto conn = driver->getConnection(h);
	bool valid = driver->isValid(conn) && driver->isIdle(conn);
	auto ctime = driver->getConnectionTime(h);
	if (!valid
			|| count >= config.nmax
			|| (count >= config.nkeep && (config.exptime == TimeInterval() || (Time::now() - ctime < config.exptime)))) {
		perform([&, this] {
			driver->finish(h);
		}, pool);
	} else {
		std::unique_lock<Mutex> lock(mutex);

		cleanup(lock);

		if (free) {
			auto ptr = free;
			ptr->ctime = ctime;
			ptr->handle = h;
			free = free->next;

			ptr->next = opened;
			opened = ptr;
			++ count;
		} else {
			auto ptr = new (pool) DbConnection;
			ptr->ctime = ctime;
			ptr->handle = h;
			ptr->next = opened;
			opened = ptr;
			++ count;
		}
	}
}

void DbConnList::cleanup(const std::unique_lock<Mutex> &lock) {
	auto now = Time::now();

	auto target = &opened;
	while (target && count > config.nkeep) {
		if (config.exptime && (now - (*target)->ctime) < config.exptime) {
			perform([&, this] {
				driver->finish(opened->handle);
			}, pool);
			-- count;

			auto tmp = *target;
			*target = tmp->next;

			tmp->handle = db::sql::Driver::Handle(nullptr);
			tmp->next = free;
			free = tmp;
			continue;
		} else {
			if ((*target)->next) {
				target = &(*target)->next;
			} else {
				break;
			}
		}
	}
}

void DbConnList::finalize() {
	config.nkeep = 0;
	config.nmin = 0;
	config.nmax = 0;

	perform([&, this] {
		while (opened) {
			auto ret = opened->handle;

			auto tmp = opened;
			tmp->handle = db::sql::Driver::Handle(nullptr);
			opened = tmp->next;

			tmp->next = free;
			free = tmp;

			-- count;

			driver->finish(ret);
		}
	}, pool);
}

DbdModule *DbdModule::create(pool_t *rootPool, Root *root, Map<StringView, StringView> &&params) {
	auto pool = pool::create(rootPool);
	DbdModule *m = nullptr;

	perform([&] {
		db::sql::Driver *driver = nullptr;

		StringView driverName;
		Config cfg;
		for (auto &it : params) {
			if (it.first == "nmin") {
				if (!StringView(it.second).readInteger(10).unwrap([&] (auto v) {
					cfg.nmin = stappler::math::clamp(uint32_t(v), uint32_t(1), config::MAX_DB_CONNECTIONS);
				})) {
					log::error("DbdModule", "Invalid value for nmin: ", it.second);
				}
			} else if (it.first == "nkeep") {
				if (!StringView(it.second).readInteger(10).unwrap([&] (auto v) {
					cfg.nkeep = stappler::math::clamp(uint32_t(v), uint32_t(1), config::MAX_DB_CONNECTIONS);
				})) {
					log::error("DbdModule", "Invalid value for nkeep: ", it.second);
				}
			} else if (it.first == "nmax") {
				if (!StringView(it.second).readInteger(10).unwrap([&] (auto v) {
					cfg.nmax = stappler::math::clamp(uint32_t(v), uint32_t(1), config::MAX_DB_CONNECTIONS);
				})) {
					log::error("DbdModule", "Invalid value for nmax: ", it.second);
				}
			} else if (it.first == "exptime") {
				if (!StringView(it.second).readInteger(10).unwrap([&] (auto v) {
					cfg.exptime = TimeInterval::microseconds(v);
				})) {
					log::error("DbdModule", "Invalid value for exptime: ", it.second);
				}
			} else if (it.first == "persistent") {
				if (it.second == "1" || it.second == "yes") {
					cfg.persistent = true;
				} else if (it.second == "0" || it.second == "no") {
					cfg.persistent = false;
				} else {
					log::error("DbdModule", "Invalid invalid value for persistent: ", it.second);
				}
			} else if (it.first == "driver") {
				driverName = it.second;
				driver = root->getDbDriver(it.second);
			}
		}

		if (driver) {
			m = new (pool) DbdModule(pool, driver, cfg, sp::move(params));
		} else {
			log::error("DbdModule", "Driver not found: ", driverName);
		}
	}, pool);
	return m;
}

void DbdModule::destroy(DbdModule *module) {
	auto p = module->getPool();
	perform([&] {
		module->close();
	}, p);
	pool::destroy(p);
}

db::sql::Driver::Handle DbdModule::openConnection(pool_t *pool) {
	db::sql::Driver::Handle rec;

	if (!_reslist->config.persistent) {
		rec = _reslist->connect(true);
	} else {
		rec = _reslist->open();
	}

	if (!_reslist->driver->isValid(rec)) {
		return db::sql::Driver::Handle(nullptr);
	}

	return rec;
}

void DbdModule::closeConnection(db::sql::Driver::Handle rec) {
	if (!_reslist->config.persistent) {
		_reslist->driver->finish(rec);
	} else {
		_reslist->close(rec);
	}
}

void DbdModule::close() {
	if (!_destroyed) {
		_reslist->finalize();
		_destroyed = true;
	}
}

db::sql::Driver *DbdModule::getDriver() const {
	return _reslist->driver;
}

DbdModule::DbdModule(pool_t *pool, db::sql::Driver *driver, Config cfg, Map<StringView, StringView> &&params)
: _pool(pool) {
	_reslist = new (pool) DbConnList(pool, driver, cfg, sp::move(params));
	auto handle = driver->connect(params);
	if (handle.get()) {
		driver->init(handle, Vector<StringView>());
		driver->finish(handle);
	} else {
		log::error("DbdModule", "Fail to initialize connection with driver ", driver->getDriverName());
	}

	pool::cleanup_register(_pool, [this] {
		_destroyed = true;
	});
}

}
