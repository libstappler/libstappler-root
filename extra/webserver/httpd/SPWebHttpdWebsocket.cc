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

#include "SPWebHttpdWebsocket.h"
#include "SPWebHttpdRequest.h"
#include "SPWebWebsocketManager.h"

#include <openssl/opensslv.h>
#if (OPENSSL_VERSION_NUMBER >= 0x10001000)
/* must be defined before including ssl.h */
#define OPENSSL_NO_SSL_INTERN
#endif
#include <openssl/ssl.h>

#define BRIGADE_NORMALIZE(b) \
do { \
    apr_bucket *e = APR_BRIGADE_FIRST(b); \
    do {  \
        if (e->length == 0 && !APR_BUCKET_IS_METADATA(e)) { \
            apr_bucket *d; \
            d = APR_BUCKET_NEXT(e); \
            apr_bucket_delete(e); \
            e = d; \
        } \
        else { \
            e = APR_BUCKET_NEXT(e); \
        } \
    } while (!APR_BRIGADE_EMPTY(b) && (e != APR_BRIGADE_SENTINEL(b))); \
} while (0)

namespace STAPPLER_VERSIONIZED stappler::web {

static const char ssl_io_filter[] = "SSL/TLS Filter";

typedef struct SSLDirConfigRec SSLDirConfigRec;

enum ssl_shutdown_type_e {
	SSL_SHUTDOWN_TYPE_UNSET,
	SSL_SHUTDOWN_TYPE_STANDARD,
	SSL_SHUTDOWN_TYPE_UNCLEAN,
	SSL_SHUTDOWN_TYPE_ACCURATE
};

struct SSLConnRec {
	SSL *ssl;
	const char *client_dn;
	X509 *client_cert;
	ssl_shutdown_type_e shutdown_type;
	const char *verify_info;
	const char *verify_error;
	int verify_depth;
	int is_proxy;
	int disabled;
	enum {
		NON_SSL_OK = 0, /* is SSL request, or error handling completed */
		NON_SSL_SEND_REQLINE, /* Need to send the fake request line */
		NON_SSL_SEND_HDR_SEP, /* Need to send the header separator */
		NON_SSL_SET_ERROR_MSG /* Need to set the error message */
	} non_ssl_request;

	/* Track the handshake/renegotiation state for the connection so
	 * that all client-initiated renegotiations can be rejected, as a
	 * partial fix for CVE-2009-3555. */
	enum {
		RENEG_INIT = 0, /* Before initial handshake */
		RENEG_REJECT, /* After initial handshake; any client-initiated
		 * renegotiation should be rejected */
		RENEG_ALLOW, /* A server-initiated renegotiation is taking
		 * place (as dictated by configuration) */
		RENEG_ABORT /* Renegotiation initiated by client, abort the
		 * connection */
	} reneg_state;

	server_rec *server;
	SSLDirConfigRec *dc;

	const char *cipher_suite; /* cipher suite used in last reneg */
	int service_unavailable; /* thouugh we negotiate SSL, no requests will be served */
	int vhost_found; /* whether we found vhost from SNI already */
};

struct ssl_filter_ctx_t {
	SSL *pssl;
	BIO *pbioRead;
	BIO *pbioWrite;
	ap_filter_t *pInputFilter;
	ap_filter_t *pOutputFilter;
	SSLConnRec *config;
};

struct char_buffer_t {
	int length;
	char *value;
};

struct bio_filter_in_ctx_t {
	SSL *ssl;
	BIO *bio_out;
	ap_filter_t *f;
	apr_status_t rc;
	ap_input_mode_t mode;
	apr_read_type_e block;
	apr_bucket_brigade *bb;
	char_buffer_t cbuf;
	apr_pool_t *pool;
	char buffer[AP_IOBUFSIZE];
	ssl_filter_ctx_t *filter_ctx;
};

struct bio_filter_out_ctx_t {
	ssl_filter_ctx_t *filter_ctx;
	conn_rec *c;
	apr_bucket_brigade *bb;    /* Brigade used as a buffer. */
	apr_status_t rc;
};

struct HttpdWebsocketConnection::FrameWriter : WebsocketFrameWriter {
	apr_bucket_brigade *tmpbb = nullptr;

	FrameWriter(pool_t *p, apr_bucket_alloc_t *alloc) : WebsocketFrameWriter(p) {
		tmpbb = apr_brigade_create((apr_pool_t *)p, alloc);
	}
};

struct HttpdWebsocketConnection::FrameReader : WebsocketFrameReader {
	apr_bucket_alloc_t *bucket_alloc;
	apr_bucket_brigade *tmpbb;

	FrameReader(Root *r, pool_t *p, apr_bucket_alloc_t *alloc)
	: WebsocketFrameReader(r, p), bucket_alloc(alloc) {
		if (pool) {
		    tmpbb = apr_brigade_create((apr_pool_t *)pool, bucket_alloc);
		}
	}

	void clear() {
		apr_brigade_destroy(tmpbb);
		WebsocketFrameReader::clear();
	    tmpbb = apr_brigade_create((apr_pool_t *)pool, bucket_alloc);
	}
};

struct HttpdWebsocketConnection::NetworkReader : AllocBase {
	pool_t *pool = nullptr;
	apr_bucket_alloc_t *bucket_alloc;
    apr_bucket_brigade *b = nullptr;
    apr_bucket_brigade *tmpbb = nullptr;

    NetworkReader(pool_t *p, apr_bucket_alloc_t *alloc) : bucket_alloc(alloc) {
    	pool = pool::create(p);
    }

    void drop() {
    	if (tmpbb && b) {
    		if (APR_BRIGADE_EMPTY(tmpbb) && APR_BRIGADE_FIRST(b) == APR_BRIGADE_LAST(b)) {
    			pool::clear(pool);
    			b = nullptr;
    			tmpbb = nullptr;
    		}
    	}
    }
};

static apr_socket_t *HttpdWebsocketConnection_duplicateSocket(apr_pool_t *pool, apr_socket_t *sock) {
	apr_os_sock_t osSock;
	if (apr_os_sock_get(&osSock, sock) == 0) {
		bool success = false;
		apr_socket_t *wsSock = nullptr;
		do {
			apr_os_sock_info_t info;
			apr_sockaddr_t *addrRemote = nullptr, *addrLocal = nullptr;
			if (apr_socket_addr_get(&addrRemote, APR_REMOTE, sock) != APR_SUCCESS) { break; }
			if (apr_socket_addr_get(&addrLocal, APR_LOCAL, sock) != APR_SUCCESS) { break; }
			if (apr_socket_type_get(sock, &info.type) != APR_SUCCESS) { break; }
			if (apr_socket_protocol_get(sock, &info.protocol) != APR_SUCCESS) { break; }

			info.family = addrRemote->family;

			info.local = (struct sockaddr *)::alloca(addrLocal->salen);
			info.remote = (struct sockaddr *)::alloca(addrRemote->salen);

			memcpy((void *)info.local, (const void *)&addrLocal->sa.sin, addrLocal->salen);
			memcpy((void *)info.remote, (const void *)&addrRemote->sa.sin, addrRemote->salen);

			info.os_sock = &osSock;

			if (apr_os_sock_make(&wsSock, &info, pool) == APR_SUCCESS) {
				apr_int32_t on = 0;
				apr_socket_opt_get(sock, APR_TCP_NODELAY, &on);
				if (on) {
					apr_socket_opt_set(wsSock, APR_TCP_NODELAY, 1);
				}

				success = true;
			}
		} while (0);

		if (success) {
			return wsSock;
		}
	}
	return nullptr;
}

apr_status_t HttpdWebsocketConnection::filterFunc(ap_filter_t *f, apr_bucket_brigade *bb) {
	// Block any output
	return APR_SUCCESS;
}

int HttpdWebsocketConnection::filterInit(ap_filter_t *f) {
	return perform([&] () -> apr_status_t {
		return OK;
	}, (pool_t *)f->c->pool);
}

void HttpdWebsocketConnection::filterRegister() {
	ap_register_output_filter(WEBSOCKET_FILTER, &(filterFunc),
			&(filterInit), ap_filter_type(AP_FTYPE_NETWORK - 1));
	ap_register_output_filter(WEBSOCKET_FILTER_OUT, &(HttpdWebsocketConnection::outputFilterFunc),
			&(HttpdWebsocketConnection::outputFilterInit), ap_filter_type(AP_FTYPE_NETWORK - 1));
	ap_register_input_filter(WEBSOCKET_FILTER_IN, &(HttpdWebsocketConnection::inputFilterFunc),
			&(HttpdWebsocketConnection::inputFilterInit), ap_filter_type(AP_FTYPE_NETWORK - 1));
}

HttpdWebsocketConnection *HttpdWebsocketConnection::create(allocator_t *alloc, pool_t *origPool, const Request &rctx) {
	auto pool = (apr_pool_t *)origPool;
	auto req = ((HttpdRequestController *)rctx.getController())->getRequest();
	auto conn = req->connection;

	// search for SSL params
	void *sslOutputFilter = nullptr;

	ap_filter_t *output_filter = conn->output_filters;
	while (output_filter != NULL) {
		if ((output_filter->frec != NULL) && (output_filter->frec->name != NULL)) {
			if (!strcasecmp(output_filter->frec->name, "ssl/tls filter")) {
				sslOutputFilter = output_filter->ctx;
			}
		}
		output_filter = output_filter->next;
	}

	auto wsSock = HttpdWebsocketConnection_duplicateSocket(pool, ap_get_conn_socket(conn));
	if (!wsSock) {
		return nullptr;
	}

	conn_rec *wsConn = (conn_rec *)apr_palloc(pool, sizeof(conn_rec));
	wsConn->pool = pool;
	wsConn->base_server = req->server;
	wsConn->vhost_lookup_data = nullptr;

	if (apr_socket_addr_get(&wsConn->local_addr, APR_LOCAL, wsSock) != APR_SUCCESS) { return nullptr; }
	if (apr_socket_addr_get(&wsConn->client_addr, APR_REMOTE, wsSock) != APR_SUCCESS) { return nullptr; }

	if (conn->client_ip) { wsConn->client_ip = apr_pstrdup(pool, conn->client_ip); }
	if (conn->remote_host) { wsConn->remote_host = apr_pstrdup(pool, conn->remote_host); }
	if (conn->remote_logname) { wsConn->remote_logname = apr_pstrdup(pool, conn->remote_logname); }
	if (conn->local_ip) { wsConn->local_ip = apr_pstrdup(pool, conn->local_ip); }
	if (conn->local_host) { wsConn->local_host = apr_pstrdup(pool, conn->local_host); }
	wsConn->id = -1;

	wsConn->conn_config = ap_create_conn_config(pool);
	wsConn->notes = apr_table_make(pool, 5);

	// create this later
	wsConn->input_filters = nullptr;
	wsConn->output_filters = nullptr;

	wsConn->sbh = nullptr;
	wsConn->bucket_alloc = apr_bucket_alloc_create(pool);
	wsConn->cs = nullptr;
	wsConn->data_in_input_filters = conn->data_in_input_filters;
	wsConn->data_in_output_filters = conn->data_in_output_filters;
	wsConn->clogging_input_filters = conn->clogging_input_filters;
	wsConn->double_reverse = conn->double_reverse;
	wsConn->aborted = 0;
	wsConn->keepalives = conn->keepalives;
	wsConn->log = nullptr;
	wsConn->log_id = nullptr;
	wsConn->current_thread = nullptr;
	wsConn->master = nullptr;

	ap_get_core_module_config(wsConn->conn_config) = (void *)wsSock;

	HttpdWebsocketConnection *ret = nullptr;
	perform([&] {
		ret = new (origPool) HttpdWebsocketConnection(alloc, origPool, wsConn, wsSock);
		if (sslOutputFilter) {
			if (!ret->setSslCtx(conn, sslOutputFilter)) {
				ret = nullptr;
			}
		}
	}, origPool);
	return ret;
}

void HttpdWebsocketConnection::prepare(apr_socket_t *sock) {
	apr_os_sock_t invalidSock = -1;
	apr_os_sock_put(&sock, &invalidSock, apr_socket_pool_get(sock));

	if (!_ssl) {
		return;
	}

	ssl_filter_ctx_t *origFilterCtx = (ssl_filter_ctx_t *)_ssl;

	SSLConnRec *connCtx = (SSLConnRec *)pool::palloc(_pool, sizeof(SSLConnRec));
	memcpy(connCtx, origFilterCtx->config, sizeof(SSLConnRec));

	if (connCtx->client_dn) { connCtx->client_dn = pool::pstrdup(_pool, connCtx->client_dn); }
	if (connCtx->verify_info) { connCtx->verify_info = pool::pstrdup(_pool, connCtx->verify_info); }
	if (connCtx->verify_error) { connCtx->verify_error = pool::pstrdup(_pool, connCtx->verify_error); }
	if (connCtx->cipher_suite) { connCtx->cipher_suite = pool::pstrdup(_pool, connCtx->cipher_suite); }

	ssl_filter_ctx_t *filterCtx = (ssl_filter_ctx_t *)pool::palloc(_pool, sizeof(ssl_filter_ctx_t));
	filterCtx->pssl = origFilterCtx->pssl;
	filterCtx->pbioRead = origFilterCtx->pbioRead;
	filterCtx->pbioWrite = origFilterCtx->pbioWrite;
	filterCtx->pInputFilter = nullptr; // setup later
	filterCtx->pOutputFilter = nullptr; // setup later
	filterCtx->config = connCtx;

	((void **)_connection->conn_config)[_sslIdx] = connCtx;

	bio_filter_in_ctx_t *origInCtx = (bio_filter_in_ctx_t *)BIO_get_data(filterCtx->pbioRead);
	bio_filter_in_ctx_t *inCtx = (bio_filter_in_ctx_t *)pool::palloc(_pool, sizeof(bio_filter_in_ctx_t));

    inCtx->ssl = filterCtx->pssl;
    inCtx->bio_out = filterCtx->pbioWrite;
    inCtx->f = filterCtx->pInputFilter; // setupLater
    inCtx->rc = origInCtx->rc;
    inCtx->mode = origInCtx->mode;
    inCtx->cbuf.length = origInCtx->cbuf.length;
    inCtx->cbuf.value = (inCtx->cbuf.length == 0) ? nullptr : (char *)pool::pmemdup(_pool, origInCtx->cbuf.value, origInCtx->cbuf.length);
    inCtx->bb = apr_brigade_create((apr_pool_t *)_pool, _connection->bucket_alloc);
    if (!APR_BRIGADE_EMPTY(origInCtx->bb)) {
    	std::cout << "Input BB is not empty\n";
    }
    inCtx->block = origInCtx->block;
    inCtx->pool = (apr_pool_t *)_pool;
    inCtx->filter_ctx = filterCtx;

	BIO_set_data(filterCtx->pbioRead, (void *)inCtx);

	bio_filter_out_ctx_t *origOutCtx = (bio_filter_out_ctx_t *)BIO_get_data(filterCtx->pbioWrite);
	bio_filter_out_ctx_t *outCtx = (bio_filter_out_ctx_t *)pool::palloc(_pool, sizeof(bio_filter_out_ctx_t));

	outCtx->filter_ctx = filterCtx;
	outCtx->c = _connection;
	outCtx->rc = origOutCtx->rc;
	outCtx->bb = apr_brigade_create((apr_pool_t *)_pool, _connection->bucket_alloc);
    if (!APR_BRIGADE_EMPTY(origOutCtx->bb)) {
    	std::cout << "Output BB is not empty\n";
    }
	BIO_set_data(filterCtx->pbioWrite, (void *)outCtx);

	filterCtx->pOutputFilter = ap_add_output_filter(ssl_io_filter, filterCtx, nullptr, _connection);
	inCtx->f = filterCtx->pInputFilter = ap_add_input_filter(ssl_io_filter, inCtx, nullptr, _connection);

	SSL_set_app_data(filterCtx->pssl, _connection);

	origFilterCtx->config->ssl = nullptr;
	origFilterCtx->config->client_cert = nullptr;

	origFilterCtx->pssl = nullptr;
	origFilterCtx->pbioRead = nullptr;
	origFilterCtx->pbioWrite = nullptr;

	origInCtx->ssl = nullptr;
	origInCtx->bio_out = nullptr;

	_ssl = filterCtx;
}

void HttpdWebsocketConnection::drop() {
	apr_os_sock_t invalidSock = -1;
	apr_os_sock_put(&_socket, &invalidSock, (apr_pool_t *)_pool);
}

bool HttpdWebsocketConnection::write(WebsocketFrameType t, const uint8_t *bytes, size_t count) {
	if (!_enabled) {
		return false;
	}

	size_t offset = 0;
	StackBuffer<32> buf;
	WebsocketFrameWriter::makeHeader(buf, count, t);

	offset = buf.size();

	auto bb = _writer->tmpbb;
	auto of = _connection->output_filters;

	_mutex.lock();
	auto err = ap_fwrite(of, bb, (const char *)buf.data(), offset);
	if (count > 0) {
		err = ap_fwrite(of, bb, (const char *)bytes, count);
	}

	err = ap_fflush(of, bb);
	apr_brigade_cleanup(bb);

	_mutex.unlock();
	if (err != APR_SUCCESS) {
		return false;
	}
	return true;
}

bool HttpdWebsocketConnection::write(apr_bucket_brigade *bb, const uint8_t *bytes, size_t &count) {
	_mutex.lock();
	auto of = _connection->output_filters;
	ap_fwrite(of, bb, (const char *)bytes, count);
	ap_fflush(of, bb);
	apr_brigade_cleanup(bb);
	_mutex.unlock();
 	return true;
}

bool HttpdWebsocketConnection::write(apr_bucket_brigade *bb) {
	bool ret = true;
	size_t offset = 0;
	auto e = APR_BRIGADE_FIRST(bb);
	if (_writer->empty()) {
		for (; e != APR_BRIGADE_SENTINEL(bb); e = APR_BUCKET_NEXT(e)) {
			if (!APR_BUCKET_IS_METADATA(e)) {
				const char * ptr = nullptr;
				apr_size_t len = 0;

				apr_bucket_read(e, &ptr, &len, APR_BLOCK_READ);

				auto written = send((const uint8_t *)ptr, size_t(len));
				if (_serverCloseCode == WebsocketStatusCode::UnexceptedCondition) {
					return false;
				}
				if (written != len) {
					offset = written;
					break;
				}
			}
		}
	}

	if (e != APR_BRIGADE_SENTINEL(bb)) {
		ret = cache(bb, e, offset);
	}

    apr_brigade_cleanup(bb);
	return ret;
}

static size_t HttpdWebsocketConnection_getBrigadeSize(apr_bucket_brigade *bb, apr_bucket *e, size_t off) {
	size_t ret = 0;
	for (; e != APR_BRIGADE_SENTINEL(bb); e = APR_BUCKET_NEXT(e)) {
		if (!APR_BUCKET_IS_METADATA(e) && e->length != apr_size_t(-1)) {
			ret += e->length;
		}
	}
	return ret - off;
}

size_t HttpdWebsocketConnection::send(const uint8_t *data, size_t len) {
	auto err = apr_socket_send(_socket, (const char *)data, &len);
	if (err != APR_SUCCESS) {
		_serverReason = "Fail to send data";
		_serverCloseCode = WebsocketStatusCode::UnexceptedCondition;
	}
	return len;
}

bool HttpdWebsocketConnection::cache(apr_bucket_brigade *bb, apr_bucket *e, size_t off) {
	auto size = HttpdWebsocketConnection_getBrigadeSize(bb, e, off);
	if (size == 0) {
		return true;
	}

	auto slot = _writer->nextEmplaceSlot(size);
	for (; e != APR_BRIGADE_SENTINEL(bb); e = APR_BUCKET_NEXT(e)) {
		if (!APR_BUCKET_IS_METADATA(e)) {
			const char * ptr = nullptr;
			apr_size_t len = 0;

			apr_bucket_read(e, &ptr, &len, APR_BLOCK_READ);
			if (off) {
				ptr += off;
				len -= off;
				off = 0;
			}

			slot->emplace((const uint8_t *)ptr, len);
		}
	}

	_pollfd.reqevents |= APR_POLLOUT;
	_fdchanged = true;

	return true;
}

bool HttpdWebsocketConnection::run(WebsocketHandler *h, const Callback<void()> &beginCb, const Callback<void()> &endCb) {
	apr_pollset_create(&_poll, 1, (apr_pool_t *)_pool, APR_POLLSET_NOCOPY | APR_POLLSET_WAKEABLE);

	if (!_socket || !_poll) {
		_serverReason = "Fail to initialize connection";
		_serverCloseCode = WebsocketStatusCode::UnexceptedCondition;
		cancel();
		return false;
	}

	// override default with user-defined max
	_reader->max = h->getMaxInputFrameSize();

	memset((void *)&_pollfd, 0, sizeof(apr_pollfd_t));
	_pollfd.p = (apr_pool_t *)_pool;
	_pollfd.reqevents = APR_POLLIN;
	_pollfd.desc_type = APR_POLL_SOCKET;
	_pollfd.desc.s = _socket;
	apr_pollset_add(_poll, &_pollfd);

	apr_socket_opt_set(_socket, APR_SO_NONBLOCK, 1);
	apr_socket_timeout_set(_socket, 0);

	_enabled = true;

	_shouldTerminate.test_and_set();

	web::perform([&] {
		h->handleBegin();
	}, _reader->pool, config::TAG_WEBSOCKET, this);
	_reader->clear();

	auto ttl = h->getTtl().toMicroseconds();

	beginCb();
	while (true) {
		apr_int32_t num = 0;
		const apr_pollfd_t *fds = nullptr;
		_pollfd.rtnevents = 0;
		if (_fdchanged) {
			apr_pollset_remove(_poll, &_pollfd);
			apr_pollset_add(_poll, &_pollfd);
		}
		auto now = Time::now();
		auto err = apr_pollset_poll(_poll, ttl, &num, &fds);

		if (!_shouldTerminate.test_and_set()) {
			// terminated
			break;
		} else if (num == 0) {
			// timeout
			auto t = (Time::now() - now).toMicros();
			if (t > ttl) {
				_serverCloseCode = WebsocketStatusCode::Away;
				break;
			} else {
				std::cout << "Error: " << err << "\n";
				break;
			}
		} else {
			if (err == APR_EINIT) {
				_group.update();
				if (!h->processBroadcasts()) {
					_shouldTerminate.clear();
					break;
				}
			}
			for (int i = 0; i < num; ++ i) {
				if (!processSocket(&fds[i], h)) {
					_shouldTerminate.clear();
					break;
				}
			}
		}

		if (!_shouldTerminate.test_and_set()) {
			break;
		}
	}

	_enabled = false;
	endCb();
	_group.waitForAll();
	web::perform([&] {
		h->handleEnd();
	}, _reader->pool, config::TAG_WEBSOCKET, this);
	memory::pool::clear(_reader->pool);

	cancel();

	return true;
}

void HttpdWebsocketConnection::wakeup() {
	apr_pollset_wakeup(_poll);
}

apr_status_t HttpdWebsocketConnection::outputFilterFunc(ap_filter_t *f, apr_bucket_brigade *bb) {
	if (APR_BRIGADE_EMPTY(bb)) {
		return APR_SUCCESS;
	}

	auto c = (HttpdWebsocketConnection *)f->ctx;
	if (c->isEnabled()) {
		if (c->write(bb)) {
			return APR_SUCCESS;
		} else {
			return APR_EGENERAL;
		}
	} else {
		return ap_pass_brigade(f->next, bb);
	}
}

int HttpdWebsocketConnection::outputFilterInit(ap_filter_t *f) {
	return OK;
}

apr_status_t HttpdWebsocketConnection::inputFilterFunc(ap_filter_t *f, apr_bucket_brigade *b,
		ap_input_mode_t mode, apr_read_type_e block, apr_off_t readbytes) {
	apr_status_t rv;
	auto conn = (HttpdWebsocketConnection *)f->ctx;
	NetworkReader *ctx = conn->_network;

	const char *str = nullptr;
	apr_size_t len = 0;

	if (mode == AP_MODE_INIT) {
		return APR_SUCCESS;
	}

	if (!ctx->tmpbb) {
		ctx->tmpbb = apr_brigade_create((apr_pool_t *)ctx->pool, ctx->bucket_alloc);
	}

	if (!ctx->b) {
		ctx->b = apr_brigade_create((apr_pool_t *)ctx->pool, ctx->bucket_alloc);
		rv = ap_run_insert_network_bucket(f->c, ctx->b, conn->_socket);
		if (rv != APR_SUCCESS) {
			return rv;
		}
	}

	/* ### This is bad. */
	BRIGADE_NORMALIZE(ctx->b);

	/* check for empty brigade again *AFTER* BRIGADE_NORMALIZE()
	 * If we have lost our socket bucket (see above), we are EOF.
	 *
	 * Ideally, this should be returning SUCCESS with EOS bucket, but
	 * some higher-up APIs (spec. read_request_line via ap_rgetline)
	 * want an error code. */
	if (APR_BRIGADE_EMPTY(ctx->b)) {
		return APR_EOF;
	}

	if (mode == AP_MODE_GETLINE || mode == AP_MODE_EATCRLF || mode == AP_MODE_EXHAUSTIVE) {
		return APR_EOF;
	}

	/* read up to the amount they specified. */
	if (mode == AP_MODE_READBYTES || mode == AP_MODE_SPECULATIVE) {
		apr_bucket *e = APR_BRIGADE_FIRST(ctx->b);

		if (e->type == &apr_bucket_type_socket) {
			auto a = e;
			auto socket = (apr_socket_t *)e->data;
		    apr_interval_time_t timeout;
		    if (block == APR_NONBLOCK_READ) {
		        apr_socket_timeout_get(socket, &timeout);
		        apr_socket_timeout_set(socket, 0);
		    }

		    if (len == 0) {
		    	len = 8_KiB;
		    }

		    auto buf = (char *)::alloca(len);

		    rv = apr_socket_recv(socket, buf, &len);

		    if (block == APR_NONBLOCK_READ) {
		        apr_socket_timeout_set(socket, timeout);
		    }

		    if (rv != APR_SUCCESS && rv != APR_EOF) {
				// return rv;
		    } else if (len > 0) {
				a = apr_bucket_heap_make(a, buf, len, nullptr);
				str = buf;
				APR_BUCKET_INSERT_AFTER(a, apr_bucket_socket_create(socket, a->list));
				rv = APR_SUCCESS;
		    } else {
				a = apr_bucket_immortal_make(a, "", 0);
				str = (const char *)a->data;
				rv = APR_SUCCESS;
		    }
		} else {
			rv = apr_bucket_read(e, &str, &len, block);
		}

		if (APR_STATUS_IS_EAGAIN(rv) && block == APR_NONBLOCK_READ) {
			/* getting EAGAIN for a blocking read is an error; for a
			 * non-blocking read, return an empty brigade. */
			return APR_SUCCESS;
		} else if (rv != APR_SUCCESS) {
			return rv;
		} else if (block == APR_BLOCK_READ && len == 0) {
			if (e) {
				apr_bucket_delete(e);
			}

			if (mode == AP_MODE_READBYTES) {
				e = apr_bucket_eos_create(f->c->bucket_alloc);
				APR_BRIGADE_INSERT_TAIL(b, e);
			}
			return APR_SUCCESS;
		}

		/* Have we read as much data as we wanted (be greedy)? */
		if (apr_off_t(len) < readbytes) {
			apr_size_t bucket_len;

			rv = APR_SUCCESS;
			/* We already registered the data in e in len */
			e = APR_BUCKET_NEXT(e);
			while ((apr_off_t(len) < readbytes) && (rv == APR_SUCCESS) && (e != APR_BRIGADE_SENTINEL(ctx->b))) {
				/* Check for the availability of buckets with known length */
				if (e->length != (apr_size_t)-1) {
					len += e->length;
					e = APR_BUCKET_NEXT(e);
				} else {
					/*
					 * Read from bucket, but non blocking. If there isn't any
					 * more data, well than this is fine as well, we will
					 * not wait for more since we already got some and we are
					 * only checking if there isn't more.
					 */
					rv = apr_bucket_read(e, &str, &bucket_len, APR_NONBLOCK_READ);
					if (rv == APR_SUCCESS) {
						len += bucket_len;
						e = APR_BUCKET_NEXT(e);
					}
				}
			}
		}

		/* We can only return at most what we read. */
		if (apr_off_t(len) < readbytes) {
			readbytes = len;
		}

		rv = apr_brigade_partition(ctx->b, readbytes, &e);
		if (rv != APR_SUCCESS) {
			return rv;
		}

		/* Must do move before CONCAT */
		ctx->tmpbb = apr_brigade_split_ex(ctx->b, e, ctx->tmpbb);

		if (mode == AP_MODE_READBYTES) {
			APR_BRIGADE_CONCAT(b, ctx->b);
		} else if (mode == AP_MODE_SPECULATIVE) {
			apr_bucket *copy_bucket;

			for (e = APR_BRIGADE_FIRST(ctx->b); e != APR_BRIGADE_SENTINEL(ctx->b); e = APR_BUCKET_NEXT(e)) {
				rv = apr_bucket_copy(e, &copy_bucket);
				if (rv != APR_SUCCESS) {
					return rv;
				}
				APR_BRIGADE_INSERT_TAIL(b, copy_bucket);
			}
		}

		/* Take what was originally there and place it back on ctx->b */
		APR_BRIGADE_CONCAT(ctx->b, ctx->tmpbb);
	}
	return APR_SUCCESS;
}

int HttpdWebsocketConnection::inputFilterInit(ap_filter_t *f) {
	return OK;
}

bool HttpdWebsocketConnection::processSocket(const apr_pollfd_t *fd, WebsocketHandler *h) {
	if ((fd->rtnevents & APR_POLLOUT) != 0) {
		if (!writeSocket(fd)) {
			return false;
		}
	}
	if ((fd->rtnevents & APR_POLLIN) != 0) {
		if (!readSocket(fd, h)) {
			return false;
		}
	}
	if ((fd->rtnevents & APR_POLLHUP) != 0) {
		return false;
	}
	_pollfd.rtnevents = 0;
	return true;
}

apr_status_t HttpdWebsocketConnection::read(apr_bucket_brigade *bb, char *buf, size_t *len) {
	if (_connection->input_filters) {
		auto f = _connection->input_filters;
	    apr_status_t rv = APR_SUCCESS;
	    apr_size_t readbufsiz = *len;

		if (bb != NULL) {
			rv = ap_get_brigade(f, bb, AP_MODE_READBYTES, APR_NONBLOCK_READ, readbufsiz);
			if (rv == APR_SUCCESS) {
				if ((rv = apr_brigade_flatten(bb, buf, len)) == APR_SUCCESS) {
					readbufsiz = *len;
				}
			}
			apr_brigade_cleanup(bb);
		}
		_network->drop();

	    if (readbufsiz == 0 && (rv == APR_SUCCESS || APR_STATUS_IS_EAGAIN(rv))) {
	    	return APR_EAGAIN;
	    }
	    return rv;
	} else {
		return apr_socket_recv(_socket, buf, len);
	}
}

bool HttpdWebsocketConnection::readSocket(const apr_pollfd_t *fd, WebsocketHandler *h) {
	apr_status_t err = APR_SUCCESS;
	while (err == APR_SUCCESS) {
		if (_reader) {
			size_t len = _reader->getRequiredBytes();
			uint8_t *buf = _reader->prepare(len);

			err = read(_reader->tmpbb, (char *)buf, &len);
			if (err == APR_SUCCESS && !_reader->save(buf, len)) {
				return false;
			} else if (err != APR_SUCCESS && !APR_STATUS_IS_EAGAIN(err)) {
				return false;
			}

			if (_reader->isControlReady()) {
				if (_reader->type == WebsocketFrameType::Close) {
					_clientCloseCode = WebsocketStatusCode(_reader->buffer.get<BytesViewNetwork>().readUnsigned16());
					_reader->popFrame();
					return false;
				} else if (_reader->type == WebsocketFrameType::Ping) {
					write(WebsocketFrameType::Pong, nullptr, 0);
					_reader->popFrame();
				}
			} else if (_reader->isFrameReady()) {
				auto ret = web::perform([&] {
					if (h) {
						h->sendPendingNotifications(_reader->pool);
					}
					if (h && !h->handleFrame(_reader->type, _reader->frame.buffer)) {
						return false;
					}
					return true;
				}, _reader->pool, config::TAG_WEBSOCKET, this);
				_reader->popFrame();
				if (!ret) {
					return false;
				}
			}
		} else {
			return false;
		}
	}
	_pollfd.reqevents |= APR_POLLIN;
	return true;
}

bool HttpdWebsocketConnection::writeSocket(const apr_pollfd_t *fd) {
	while (!_writer->empty()) {
		auto slot = _writer->nextReadSlot();

		while (!slot->empty()) {
			auto len = slot->getNextLength();
			auto ptr = slot->getNextBytes();

			auto written = send(ptr, len);
			if (_serverCloseCode == WebsocketStatusCode::UnexceptedCondition) {
				return false;
			}
			if (written > 0) {
				slot->pop(written);
			}
			if (written != len) {
				break;
			}
		}

		if (slot->empty()) {
			_writer->popReadSlot();
		} else {
			break;
		}
	}

	if (_writer->empty()) {
		if (_pollfd.reqevents & APR_POLLOUT) {
			_pollfd.reqevents &= ~APR_POLLOUT;
			_fdchanged = true;
		}
	} else {
		if ((_pollfd.reqevents & APR_POLLOUT) == 0) {
			_pollfd.reqevents |= APR_POLLOUT;
			_fdchanged = true;
		}
	}

	return true;
}

void HttpdWebsocketConnection::cancel(StringView reason) {
	// drain all data
	StackBuffer<(size_t)1_KiB> sbuf;

	if (_connected) {
		apr_status_t err = APR_SUCCESS;
		while (err == APR_SUCCESS) {
			size_t len = 1_KiB;
			err = read(_reader->tmpbb, (char *)sbuf.data(), &len);
			sbuf.clear();
		}

		apr_socket_opt_set(_socket, APR_SO_NONBLOCK, 0);
		apr_socket_timeout_set(_socket, -1);

		writeSocket(&_pollfd);
	}

	sbuf.clear();

	WebsocketStatusCode nstatus = resolveStatus(_serverCloseCode);
	uint16_t status = byteorder::HostToNetwork(_clientCloseCode==WebsocketStatusCode::None?(uint16_t)nstatus:(uint16_t)_clientCloseCode);
	size_t memSize = std::min((size_t)123, _serverReason.size());
	size_t frameSize = 4 + memSize;
	sbuf[0] = (0b10000000 | 0x8);
	sbuf[1] = ((uint8_t)(memSize + 2));
	memcpy(sbuf.data() + 2, &status, sizeof(uint16_t));
	if (!_serverReason.empty()) {
		memcpy(sbuf.data() + 4, _serverReason.data(), memSize);
	}

	write(_writer->tmpbb, sbuf.data(), frameSize);
	ap_shutdown_conn(_connection, 1);
	_connected = false;

	clearSslCtx();
	close();
}

void HttpdWebsocketConnection::close() {
	apr_socket_close(_socket);
	//::close(_eventFd);
}

bool HttpdWebsocketConnection::setSslCtx(conn_rec *c, void *ctx) {
	int modInd = stappler_web_module.module_index;

	ssl_filter_ctx_t *origFilterCtx = (ssl_filter_ctx_t *)ctx;
	auto vec = (void **)(c->conn_config);

	for (int i = 0; i < modInd; ++ i) {
		if (origFilterCtx->config == vec[i]) {
			_sslIdx = i;
			break;
		}
	}

	if (_sslIdx < 0) {
		return false;
	}

	_ssl = ctx;
	return true;
}

void HttpdWebsocketConnection::clearSslCtx() {
	if (!_ssl) {
		return;
	}

	ssl_filter_ctx_t *filterCtx = (ssl_filter_ctx_t *)_ssl;
	if (filterCtx->pssl) {
		SSL_free(filterCtx->pssl);
		filterCtx->config->ssl = filterCtx->pssl = nullptr;
	}
}

HttpdWebsocketConnection::HttpdWebsocketConnection(allocator_t *alloc, pool_t *pool, conn_rec *c, apr_socket_t *sock)
: WebsocketConnection(alloc, pool, HttpdHostController::get(c->base_server)), _connection(c), _socket(sock) {
	ap_add_output_filter(WEBSOCKET_FILTER_OUT, (void *)this, nullptr, _connection);
	ap_add_input_filter(WEBSOCKET_FILTER_IN, (void *)this, nullptr, _connection);

	_commonWriter = _writer = new (_pool) FrameWriter(_pool, _connection->bucket_alloc);
	_commonReader = _reader = new (_pool) FrameReader(_group.getHost().getRoot(), _pool, _connection->bucket_alloc);
	_network = new (_pool) NetworkReader(_pool, _connection->bucket_alloc);

}

}
