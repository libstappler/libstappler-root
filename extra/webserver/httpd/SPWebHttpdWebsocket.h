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

#ifndef EXTRA_WEBSERVER_HTTPD_SPWEBHTTPDWEBSOCKET_H_
#define EXTRA_WEBSERVER_HTTPD_SPWEBHTTPDWEBSOCKET_H_

#include "SPWebWebsocketConnection.h"

#include "util_filter.h"

namespace STAPPLER_VERSIONIZED stappler::web {

class SP_PUBLIC HttpdWebsocketConnection : public WebsocketConnection {
public:
	static constexpr auto WEBSOCKET_FILTER = "stappler::web::HttpdWebsocketFilter";
	static constexpr auto WEBSOCKET_FILTER_IN = "stappler::web::HttpdWebsocketConnectionInput";
	static constexpr auto WEBSOCKET_FILTER_OUT = "stappler::web::HttpdWebsocketConnectionOutput";

	static apr_status_t filterFunc(ap_filter_t *f, apr_bucket_brigade *bb);
	static int filterInit(ap_filter_t *f);
	static void filterRegister();

	static HttpdWebsocketConnection *create(allocator_t *alloc, pool_t *pool, const Request &);

	void prepare(apr_socket_t *);
	void drop();

	// write ws protocol frame data into the filter chain
	virtual bool write(WebsocketFrameType t, const uint8_t *bytes = nullptr, size_t count = 0) override;

	// write data down into the filter chain
	bool write(apr_bucket_brigade *, const uint8_t *bytes, size_t &count);

	// write filtered bb into non-block socket and cache the rest
	bool write(apr_bucket_brigade *);

	// direct write to socket
	size_t send(const uint8_t *data, size_t len);

	bool cache(apr_bucket_brigade *bb, apr_bucket *e, size_t off);

	virtual bool run(WebsocketHandler *h, const Callback<void()> &beginCb, const Callback<void()> &endCb);
	virtual void wakeup() override;

protected:
	struct FrameWriter;
	struct FrameReader;
	struct NetworkReader;

	static apr_status_t outputFilterFunc(ap_filter_t *f, apr_bucket_brigade *bb);
	static int outputFilterInit(ap_filter_t *f);

	static apr_status_t inputFilterFunc(ap_filter_t *f, apr_bucket_brigade *b,
            ap_input_mode_t mode, apr_read_type_e block, apr_off_t readbytes);
	static int inputFilterInit(ap_filter_t *f);

	bool processSocket(const apr_pollfd_t *fd, WebsocketHandler *h);
	apr_status_t read(apr_bucket_brigade *bb, char *buf, size_t *len);

	bool readSocket(const apr_pollfd_t *fd, WebsocketHandler *h);
	bool writeSocket(const apr_pollfd_t *fd);

	void cancel(StringView reason = StringView());
	void close();

	bool setSslCtx(conn_rec *, void *);
	void clearSslCtx();

	HttpdWebsocketConnection(allocator_t *, pool_t *, conn_rec *, apr_socket_t *);

	conn_rec *_connection = nullptr;
	apr_socket_t *_socket = nullptr;
	int _sslIdx = -1;
	void *_ssl = nullptr;

	apr_pollset_t *_poll = nullptr;
	apr_pollfd_t _pollfd;
	bool _fdchanged = false;

	FrameWriter *_writer = nullptr;
	FrameReader *_reader = nullptr;
	NetworkReader *_network = nullptr;
};

}

#endif /* EXTRA_WEBSERVER_HTTPD_SPWEBHTTPDWEBSOCKET_H_ */
