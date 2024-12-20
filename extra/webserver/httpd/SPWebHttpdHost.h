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

#ifndef EXTRA_WEBSERVER_HTTPD_SPWEBHTTPDHOST_H_
#define EXTRA_WEBSERVER_HTTPD_SPWEBHTTPDHOST_H_

#include "SPWebHostController.h"

#include "httpd.h"

namespace STAPPLER_VERSIONIZED stappler::web {

class HttpdRoot;

class SP_PUBLIC HttpdHostController : public HostController {
public:
	static HttpdHostController *create(HttpdRoot *, server_rec *);
	static HttpdHostController *get(server_rec *);

	static HttpdHostController *merge(HttpdHostController *base, HttpdHostController *add);

	HttpdHostController(Root *, pool_t *, server_rec *);

	virtual void handleChildInit(const Host &serv, pool_t *p) override;

	server_rec *getServer() const { return _server; }

protected:
	virtual db::sql::Driver * openInternalDriver(db::sql::Driver::Handle) override;

	server_rec *_server = nullptr;
};

}

#endif /* EXTRA_WEBSERVER_HTTPD_SPWEBHTTPDHOST_H_ */
