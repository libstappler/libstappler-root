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

#ifndef EXTRA_WEBSERVER_WEBSERVER_SPWEBINFO_H_
#define EXTRA_WEBSERVER_WEBSERVER_SPWEBINFO_H_

#include "SPWeb.h"
#include "SPUrl.h"

namespace STAPPLER_VERSIONIZED stappler::web {

class RequestHandler;
class RequestHandlerMap;

enum class EtagMode {
	AddSuffix,
	NoChange,
	Remove
};

enum class CookieFlags {
	Secure = 1 << 0,
	HttpOnly = 1 << 1,
	SetOnError = 1 << 2,
	SetOnSuccess = 1 << 3,

	SameSiteNone = 1 << 4,
	SameSiteLux = 2 << 4,
	SameSiteStrict = 3 << 4,

	Default = 1 | 2 | 8 | 16 // Secure | HttpOnly | SetOnSuccess
};

SP_DEFINE_ENUM_AS_MASK(CookieFlags)


// Apache HTTPD mapping
enum class RequestMethod : int {
	Get =				0,
	Put =				1,
	Post =				2,
	Delete =			3,
	Connect =			4,
	Options =			5,
	Trace =				6,
	Patch =				7,
	Propfind =			8,
	Proppatch =			9,
	MkCol =				10,
	Copy =				11,
	Move =				12,
	Lock =				13,
	Unlock =			14,
	VersionControl =	15,
	Checkout =			16,
	Uncheckout =		17,
	Checkin =			18,
	Update =			19,
	Label =				20,
	Report =			21,
	MkWorkspace =		22,
	MkActivity =		23,
	BaselineControl =	24,
	Merge =				25,
	Invalid =			26,
};

enum Status : int {
	OK = 0,
	DECLINED = -1,
	DONE = -2,
	SUSPENDED = -3,

	HTTP_CONTINUE = 100,
	HTTP_SWITCHING_PROTOCOLS = 101,
	HTTP_PROCESSING = 102,
	HTTP_OK = 200,
	HTTP_CREATED = 201,
	HTTP_ACCEPTED = 202,
	HTTP_NON_AUTHORITATIVE = 203,
	HTTP_NO_CONTENT = 204,
	HTTP_RESET_CONTENT = 205,
	HTTP_PARTIAL_CONTENT = 206,
	HTTP_MULTI_STATUS = 207,
	HTTP_ALREADY_REPORTED = 208,
	HTTP_IM_USED = 226,
	HTTP_MULTIPLE_CHOICES = 300,
	HTTP_MOVED_PERMANENTLY  = 301,
	HTTP_MOVED_TEMPORARILY = 302,
	HTTP_SEE_OTHER = 303,
	HTTP_NOT_MODIFIED = 304,
	HTTP_USE_PROXY = 305,
	HTTP_TEMPORARY_REDIRECT = 307,
	HTTP_PERMANENT_REDIRECT = 308,
	HTTP_BAD_REQUEST = 400,
	HTTP_UNAUTHORIZED = 401,
	HTTP_PAYMENT_REQUIRED = 402,
	HTTP_FORBIDDEN = 403,
	HTTP_NOT_FOUND = 404,
	HTTP_METHOD_NOT_ALLOWED = 405,
	HTTP_NOT_ACCEPTABLE = 406,
	HTTP_PROXY_AUTHENTICATION_REQUIRED = 407,
	HTTP_REQUEST_TIME_OUT = 408,
	HTTP_CONFLICT = 409,
	HTTP_GONE = 410,
	HTTP_LENGTH_REQUIRED = 411,
	HTTP_PRECONDITION_FAILED = 412,
	HTTP_REQUEST_ENTITY_TOO_LARGE = 413,
	HTTP_REQUEST_URI_TOO_LARGE = 414,
	HTTP_UNSUPPORTED_MEDIA_TYPE = 415,
	HTTP_RANGE_NOT_SATISFIABLE = 416,
	HTTP_EXPECTATION_FAILED = 417,
	HTTP_MISDIRECTED_REQUEST = 421,
	HTTP_UNPROCESSABLE_ENTITY = 422,
	HTTP_LOCKED = 423,
	HTTP_FAILED_DEPENDENCY = 424,
	HTTP_UPGRADE_REQUIRED = 426,
	HTTP_PRECONDITION_REQUIRED = 428,
	HTTP_TOO_MANY_REQUESTS = 429,
	HTTP_REQUEST_HEADER_FIELDS_TOO_LARGE = 431,
	HTTP_UNAVAILABLE_FOR_LEGAL_REASONS = 451,
	HTTP_INTERNAL_SERVER_ERROR = 500,
	HTTP_NOT_IMPLEMENTED = 501,
	HTTP_BAD_GATEWAY = 502,
	HTTP_SERVICE_UNAVAILABLE = 503,
	HTTP_GATEWAY_TIME_OUT = 504,
	HTTP_VERSION_NOT_SUPPORTED = 505,
	HTTP_VARIANT_ALSO_VARIES = 506,
	HTTP_INSUFFICIENT_STORAGE = 507,
	HTTP_LOOP_DETECTED = 508,
	HTTP_NOT_EXTENDED = 510,
	HTTP_NETWORK_AUTHENTICATION_REQUIRED = 511,
};

enum class InputFilterAccept {
	None = 0,
	Urlencoded = 1,
	Multipart = 2,
	Json = 3,
	Files = 4
};

enum class ResourceType {
	ResourceList,
	ReferenceSet,
	ObjectField,
	Object,
	Set,
	View,
	File,
	Array,
	Search
};

// brotli compression configuration
// based on mod_brotli defaults
struct CompressionInfo {
	bool enabled = true;
	int quality = 5;
	int lgwin = 18;
	int lgblock = 0;
	EtagMode etag_mode = EtagMode::NoChange;
	const char *note_input_name = nullptr;
	const char *note_output_name = nullptr;
	const char *note_ratio_name = nullptr;
};

struct SessionInfo {
	String name = config::DEFAULT_SESSION_NAME;
	String key = config::DEFAULT_SESSION_KEY;
	TimeInterval maxAge;
	bool secure = true;

	void init(const Value &);
	void setParam(StringView, StringView);
};

struct WebhookInfo {
	String url;
	String name;
	String format;
	Value extra;

	void init(const Value &);
	void setParam(StringView, StringView);
};

struct ResourceSchemeInfo {
	StringView path;
	Value data;
};

struct RequestSchemeInfo {
	using HandlerCallback = Function<RequestHandler *()>;

	StringView component;
	HandlerCallback callback;
	Value data;
	const db::Scheme *scheme = nullptr;
	const RequestHandlerMap *map = nullptr;
};

struct HostInfo {
	StringView hostname;
	StringView documentRoot;
	StringView scheme;
	StringView admin;

	TimeInterval timeout;
	TimeInterval keepAlive;
	uint32_t maxKeepAlives = 0;
	uint16_t port = 0;
	bool useKeepAlive;
	bool isVirtual = true;
};

struct CookieStorageInfo {
	String data;
	CookieFlags flags;
	TimeInterval maxAge;
};

struct HostComponentInfo {
	StringView name;
	StringView version;
	StringView file;
	StringView symbol;

	Value data;
};

struct RequestInfo {
	RequestMethod method = RequestMethod::Invalid;
	Time requestTime;
	uint32_t protocolVersion = 0;
	Status status = OK;
	off_t contentLength = 0;
	bool headerRequest = false;

	filesystem::Stat stat;

	StringView requestLine;
	StringView protocol;
	StringView methodName;
	StringView statusLine;
	StringView rangeLine;
	StringView documentRoot;
	StringView contentType;
	StringView contentEncoding;
	StringView authType;
	StringView unparserUri;
	StringView filename;
	StringView useragentIp;
	StringView handler;

	uint16_t useragentPort = 0;

	UrlView url;

	SpanView<StringView> queryPath;
	Value queryData;

	RequestInfo clone(pool_t *);
};

RequestMethod getRequestMethod(StringView);
uint32_t getProtocolVersionNumber(StringView);
StringView extractCharset(StringView s);

StringView getStatusLine(Status);

}

#endif /* EXTRA_WEBSERVER_WEBSERVER_SPWEBINFO_H_ */
