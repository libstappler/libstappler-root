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

#ifndef EXTRA_WEBSERVER_HTTPD_SPWEBSERVERHTTPD_H_
#define EXTRA_WEBSERVER_HTTPD_SPWEBSERVERHTTPD_H_

#include "SPWebInfo.h"

#include "apr_hash.h"
#include "apr_strings.h"
#include "apr_tables.h"
#include "apr_pools.h"
#include "apr_reslist.h"
#include "apr_atomic.h"
#include "apr_random.h"
#include "apr_dbd.h"
#include "apr_uuid.h"
#include "apr_md5.h"
#include "apr_base64.h"
#include "apr_errno.h"
#include "apr_time.h"
#include "apr_thread_rwlock.h"
#include "apr_thread_cond.h"
#include "apr_thread_pool.h"
#include "apr_sha1.h"

#include "httpd.h"
#include "http_core.h"
#include "http_config.h"
#include "http_connection.h"
#include "http_protocol.h"
#include "http_request.h"
#include "http_log.h"
#include "mod_dbd.h"
#include "mod_ssl.h"
#include "mod_log_config.h"
#include "util_cookies.h"

#endif /* EXTRA_WEBSERVER_HTTPD_SPWEBSERVERHTTPD_H_ */
