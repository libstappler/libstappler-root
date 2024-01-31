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

#ifndef EXTRA_WEBSERVER_HTTPD_SPWEBSERVERHTTPDREQUEST_H_
#define EXTRA_WEBSERVER_HTTPD_SPWEBSERVERHTTPDREQUEST_H_

#include "SPWebHttpdTable.h"
#include "SPWebRequest.h"
#include "SPWebRequestController.h"

namespace STAPPLER_VERSIONIZED stappler::web {

class HttpdHostController;

class HttpdRequestController : public RequestController {
public:
	static HttpdRequestController *get(request_rec *);

	static void readRequestInfo(request_rec *rec, RequestInfo &);

	HttpdRequestController(HostController *, request_rec *, RequestInfo &&);

	request_rec *getRequest() const { return _request; }

	virtual bool isSecureConnection() const override;

	virtual void startResponseTransmission() override;

	virtual size_t getBytesSent() const override;
	virtual void putc(int) override;
	virtual size_t write(const uint8_t *, size_t) override;
	virtual void flush() override;

	virtual void setDocumentRoot(StringView) override;
	virtual void setContentType(StringView) override;
	virtual void setHandler(StringView) override;
	virtual void setContentEncoding(StringView) override;
	virtual void setStatus(Status status, StringView) override;

	virtual void setFilename(StringView, bool updateStat, Time mtime) override;

	virtual StringView getCookie(StringView name, bool removeFromHeadersTable) override;

	virtual StringView getRequestHeader(StringView) const override;
	virtual void foreachRequestHeaders(const Callback<void(StringView, StringView)> &) const override;

	virtual StringView getResponseHeader(StringView) const override;
	virtual void foreachResponseHeaders(const Callback<void(StringView, StringView)> &) const override;
	virtual void setResponseHeader(StringView, StringView) override;
	virtual void clearResponseHeaders() override;

	virtual StringView getErrorHeader(StringView) const override;
	virtual void foreachErrorHeaders(const Callback<void(StringView, StringView)> &) const override;
	virtual void setErrorHeader(StringView, StringView) override;
	virtual void clearErrorHeaders() override;

	virtual InputFilter *makeInputFilter(InputFilterAccept) override;
	virtual void setInputFilter(InputFilter *) override;

	virtual WebsocketConnection *convertToWebsocket(WebsocketHandler *, allocator_t *, pool_t *) override;

protected:
	void updateRequestInfo();

	request_rec *_request = nullptr;
	httpd::table _requestHeaders;
	httpd::table _responseHeaders;
	httpd::table _errorHeaders;
};

}

#endif /* EXTRA_WEBSERVER_HTTPD_SPWEBSERVERHTTPDREQUEST_H_ */
