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

#ifndef EXTRA_WEBSERVER_WEBSERVER_DBD_SPWEBDBD_H_
#define EXTRA_WEBSERVER_WEBSERVER_DBD_SPWEBDBD_H_

#include "SPWebInfo.h"
#include "SPSqlDriver.h"

namespace STAPPLER_VERSIONIZED stappler::web {

class Root;

struct DbConnList;

class SP_PUBLIC DbdModule : public AllocBase {
public:
	struct Config {
		uint32_t nmin = 1;
		uint32_t nkeep = 2;
		uint32_t nmax = 10;
		TimeInterval exptime = TimeInterval::seconds(10);
		bool persistent = true;
	};

	static DbdModule *create(pool_t *rootPool, Root *root, Map<StringView, StringView> &&params);
	static void destroy(DbdModule *);

	db::sql::Driver::Handle openConnection(pool_t *);
	void closeConnection(db::sql::Driver::Handle);

	void close();

	pool_t *getPool() const { return _pool; }
	bool isDestroyed() const { return _destroyed; }

	db::sql::Driver *getDriver() const;

protected:
	DbdModule(pool_t *, db::sql::Driver *, Config cfg, Map<StringView, StringView> &&);

	pool_t *_pool = nullptr;
	DbConnList *_reslist = nullptr;
	bool _destroyed = true;
};

}

#endif /* EXTRA_WEBSERVER_WEBSERVER_DBD_SPWEBDBD_H_ */
