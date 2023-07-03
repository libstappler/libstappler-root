/**
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

#ifndef TESTS_DB_SRC_SERVER_H_
#define TESTS_DB_SRC_SERVER_H_

#include "SPRef.h"
#include "STStorageScheme.h"
#include "STSqlDriver.h"

namespace stappler::dbtest {

class ServerScheme : public RefBase<memory::StandartInterface> {
public:
	ServerScheme(memory::pool_t *p);

	virtual void fillSchemes(db::Map<StringView, const db::Scheme *> &);

	const db::Scheme &getFileScheme() const { return _users; }
	const db::Scheme &getUserScheme() const { return _files; }
	const db::Scheme &getErrorScheme() const { return _errors; }

protected:
	memory::pool_t *_pool = nullptr;
	db::Scheme _users = db::Scheme("__users");
	db::Scheme _files = db::Scheme("__files");
	db::Scheme _errors = db::Scheme("__error");
};

class Server : public db::StorageRoot {
public:
	virtual ~Server();
	Server(const mem_std::Value &, const Callback<Rc<ServerScheme>(memory::pool_t *)> &, db::AccessRoleId);

	virtual void scheduleAyncDbTask(const db::Callback<db::Function<void(const db::Transaction &)>(db::pool_t *)> &setupCb) override;
	virtual db::String getDocuemntRoot() const override;
	virtual const db::Scheme *getFileScheme() const override;
	virtual const db::Scheme *getUserScheme() const override;

	virtual void onLocalBroadcast(const db::Value &) override;
	virtual void onStorageTransaction(db::Transaction &) override;

	void update();
	void perform(const Callback<bool(const db::Transaction &)> &cb);

protected:
	memory::pool_t *_staticPool = nullptr;
	memory::pool_t *_contextPool = nullptr;
	Rc<ServerScheme> _scheme;

	db::AccessRoleId _defaultRole;
	db::sql::Driver *_driver = nullptr;
	db::sql::Driver::Handle _handle;
	db::BackendInterface::Config interfaceConfig;
	db::Vector<db::Function<void(const db::Transaction &)>> *asyncTasks = nullptr;
};

}

#endif /* TESTS_DB_SRC_SERVER_H_ */
