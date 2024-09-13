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

#ifndef EXTRA_WEBSERVER_WEBSERVER_REQUEST_SPWEBREQUESTCONFIG_H_
#define EXTRA_WEBSERVER_WEBSERVER_REQUEST_SPWEBREQUESTCONFIG_H_

#include "SPWebInfo.h"
#include "SPUrl.h"
#include "SPTime.h"

namespace STAPPLER_VERSIONIZED stappler::web {

class HostController;
class Session;
class InputFilter;
class WebsocketHandler;
class WebsocketConnection;

class SP_PUBLIC RequestController : public AllocBase {
public:
	virtual ~RequestController();

	RequestController(pool_t *, RequestInfo &&);

	virtual void bind(HostController *);

	virtual bool init();
	virtual void finalize();

	pool_t *getPool() const { return _pool; }

	HostController *getHost() const { return _host; }

	const RequestInfo &getInfo() const { return _info; }

	float isAcceptable(StringView) const;

	virtual void startResponseTransmission() = 0;

	virtual size_t getBytesSent() const = 0;
	virtual void putc(int) = 0;
	virtual size_t write(const uint8_t *, size_t) = 0;
	virtual void flush() = 0;

	virtual bool isSecureConnection() const = 0;
	virtual bool isSecureAuthAllowed() const;

	virtual void setDocumentRoot(StringView) = 0;
	virtual void setContentType(StringView) = 0;
	virtual void setHandler(StringView) = 0;
	virtual void setContentEncoding(StringView) = 0;
	virtual void setStatus(Status status, StringView) = 0;

	virtual StringView getCookie(StringView name, bool removeFromHeadersTable = true) = 0;

	virtual void setFilename(StringView, bool updateStat = true, Time mtime = Time()) = 0;

	virtual StringView getRequestHeader(StringView) const = 0;
	virtual void foreachRequestHeaders(const Callback<void(StringView, StringView)> &) const = 0;

	virtual StringView getResponseHeader(StringView) const = 0;
	virtual void foreachResponseHeaders(const Callback<void(StringView, StringView)> &) const = 0;
	virtual void setResponseHeader(StringView, StringView) = 0;
	virtual void clearResponseHeaders() = 0;

	virtual StringView getErrorHeader(StringView) const = 0;
	virtual void foreachErrorHeaders(const Callback<void(StringView, StringView)> &) const = 0;
	virtual void setErrorHeader(StringView, StringView) = 0;
	virtual void clearErrorHeaders() = 0;

	virtual db::Adapter acquireDatabase();

	virtual InputFilter *makeInputFilter(InputFilterAccept);
	virtual void setInputFilter(InputFilter *);
	virtual InputFilter *getInputFilter() const { return _filter; }

	virtual Value getDefaultResult();

	virtual WebsocketConnection *convertToWebsocket(WebsocketHandler *, allocator_t *, pool_t *) { return nullptr; }

	virtual void pushErrorMessage(Value &&);
	virtual void pushDebugMessage(Value &&);

protected:
	friend class Request;

	pool_t *_pool = nullptr;
	RequestInfo _info;
	HostController *_host = nullptr;
	db::InputConfig _inputConfig;
	db::BackendInterface *_database = nullptr;

	Vector<Value> _debug;
	Vector<Value> _errors;

	RequestHandler *_handler = nullptr;
	db::User *_user = nullptr;
	int64_t _userId = 0;
	Session *_session = nullptr;
	InputFilter *_filter = nullptr;

	Map<StringView, CookieStorageInfo> _cookies;
	db::AccessRoleId _accessRole = db::AccessRoleId::Nobody;
	Vector<Pair<StringView, float>> _acceptList;
};

}

#endif /* EXTRA_WEBSERVER_WEBSERVER_REQUEST_SPWEBREQUESTCONFIG_H_ */
