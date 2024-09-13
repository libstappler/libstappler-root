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

#ifndef EXTRA_WEBSERVER_HTTPD_SPWEBHTTPDROOT_H_
#define EXTRA_WEBSERVER_HTTPD_SPWEBHTTPDROOT_H_

#include "SPWebRoot.h"

#include "apr_thread_proc.h"
#include "apr_thread_pool.h"
#include "apr_optional.h"
#include "mod_ssl.h"
#include "mod_dbd.h"

namespace STAPPLER_VERSIONIZED stappler::web {

class SP_PUBLIC HttpdRoot : public Root {
public:
	virtual ~HttpdRoot();
	HttpdRoot(pool_t *);

	virtual bool isSecureConnection(const Request &) const override;

	Status handlePostConfig(pool_t *, server_rec *s);
	void handleChildInit(pool_t *, server_rec *s);

	virtual bool performTask(const Host &, AsyncTask *task, bool performFirst) override;
	virtual bool scheduleTask(const Host &, AsyncTask *task, TimeInterval) override;

	virtual void foreachHost(const Callback<void(Host &)> &) override;

	virtual db::sql::Driver::Handle dbdOpen(pool_t *, const Host &) override;
	virtual void dbdClose(const Host &, db::sql::Driver::Handle) override;
	virtual db::sql::Driver::Handle dbdAcquire(const Request &) override;

protected:
	virtual void initThreads() override;

	APR_OPTIONAL_FN_TYPE(ssl_is_https) * _sslIsHttps = nullptr;

	APR_OPTIONAL_FN_TYPE(ap_dbd_open) * _dbdOpen = nullptr;
	APR_OPTIONAL_FN_TYPE(ap_dbd_close) * _dbdClose = nullptr;
	APR_OPTIONAL_FN_TYPE(ap_dbd_acquire) * _dbdRequestAcquire = nullptr;
	APR_OPTIONAL_FN_TYPE(ap_dbd_cacquire) * _dbdConnectionAcquire = nullptr;

	apr_thread_t *_timerThread = nullptr;
	apr_time_t _timerValue = 0;

	apr_thread_pool_t *_threadPool = nullptr;

	server_rec * _rootServer = nullptr;
};

}

#endif /* EXTRA_WEBSERVER_HTTPD_SPWEBHTTPDROOT_H_ */
