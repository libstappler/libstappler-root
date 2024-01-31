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

#ifndef EXTRA_WEBSERVER_UNIX_SPWEBUNIXREQUEST_H_
#define EXTRA_WEBSERVER_UNIX_SPWEBUNIXREQUEST_H_

#include "SPWebRequestController.h"
#include "SPWebUnixConnectionWorker.h"

namespace STAPPLER_VERSIONIZED stappler::web {

class UnixRequestController : public RequestController {
public:
	virtual ~UnixRequestController() = default;

	UnixRequestController(pool_t *, RequestInfo &&, ConnectionWorker::Client *);

	virtual void startResponseTransmission() override;

	virtual size_t getBytesSent() const override;
	virtual void putc(int) override;
	virtual size_t write(const uint8_t *, size_t) override;
	virtual void flush() override;

	virtual bool isSecureConnection() const override;

	virtual void setDocumentRoot(StringView) override;
	virtual void setContentType(StringView) override;
	virtual void setHandler(StringView) override;
	virtual void setContentEncoding(StringView) override;
	virtual void setStatus(Status status, StringView) override;

	virtual StringView getCookie(StringView name, bool removeFromHeadersTable = true) override;

	virtual void setFilename(StringView, bool updateStat = true, Time mtime = Time()) override;

	virtual StringView getRequestHeader(StringView) const override;
	virtual void foreachRequestHeaders(const Callback<void(StringView, StringView)> &) const override;
	virtual void setRequestHeader(StringView, StringView);

	virtual StringView getResponseHeader(StringView) const override;
	virtual void foreachResponseHeaders(const Callback<void(StringView, StringView)> &) const override;
	virtual void setResponseHeader(StringView, StringView) override;
	virtual void clearResponseHeaders() override;

	virtual StringView getErrorHeader(StringView) const override;
	virtual void foreachErrorHeaders(const Callback<void(StringView, StringView)> &) const override;
	virtual void setErrorHeader(StringView, StringView) override;
	virtual void clearErrorHeaders() override;

	virtual Status processInput(ConnectionWorker::BufferChain &);

	virtual void submitResponse(Status);

protected:
	Map<StringView, StringView> _requestHeaders;
	Map<StringView, StringView> _responseHeaders;
	Map<StringView, StringView> _errorHeaders;

	ConnectionWorker::Client *_client;
};

}

#endif /* EXTRA_WEBSERVER_UNIX_SPWEBUNIXREQUEST_H_ */
