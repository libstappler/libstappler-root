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

#ifndef EXTRA_WEBSERVER_WEBSERVER_REQUEST_SPWEBREQUEST_H_
#define EXTRA_WEBSERVER_WEBSERVER_REQUEST_SPWEBREQUEST_H_

#include "SPWebInfo.h"
#include "SPWebHost.h"
#include "SPPugContext.h"

namespace STAPPLER_VERSIONIZED stappler::web {

class Session;
class RequestController;
class InputFilter;

class SP_PUBLIC Request final : public std::basic_ostream<char, std::char_traits<char>>, public AllocBase {
public:
	using char_type = char;
	using traits_type = std::char_traits<char>;
	using ostream_type = std::basic_ostream<char_type, traits_type>;
	using Require = db::InputConfig::Require;

	static Request getCurrent();

	Request();
	Request(RequestController *);
	Request & operator =(RequestController *);

	Request(Request &&);
	Request & operator =(Request &&);

	Request(const Request &);
	Request & operator =(const Request &);

	RequestController *getController() const { return _config; }
	explicit operator bool () const { return _config != nullptr; }

	const RequestInfo &getInfo() const;

	StringView getRequestHeader(StringView) const;
	void foreachRequestHeaders(const Callback<void(StringView, StringView)> &) const;

	StringView getResponseHeader(StringView) const;
	void foreachResponseHeaders(const Callback<void(StringView, StringView)> &) const;
	void setResponseHeader(StringView, StringView) const;
	void clearResponseHeaders() const;

	StringView getErrorHeader(StringView) const;
	void foreachErrorHeaders(const Callback<void(StringView, StringView)> &) const;
	void setErrorHeader(StringView, StringView) const;
	void clearErrorHeaders() const;

	void setRequestHandler(RequestHandler *);
	RequestHandler *getRequestHandler() const;

	void writeData(const Value &, bool allowJsonP = false);

	/* request params setters */
	void setDocumentRoot(StringView);
	void setContentType(StringView);
	void setHandler(StringView);
	void setContentEncoding(StringView);

	/* set path for file, that should be returned in response via sendfile */
	void setFilename(StringView, bool updateStat = true, Time mtime = Time());

	/* set HTTP status code and response status line ("404 NOT FOUND", etc.)
	 * if no string provided, default status line for code will be used */
	void setStatus(Status status, StringView = StringView());

	void setCookie(StringView name, StringView value, TimeInterval maxAge = TimeInterval(), CookieFlags flags = CookieFlags::Default);
	void removeCookie(StringView name, CookieFlags flags = CookieFlags::Default);

	// cookies, that will be sent in server response
	const Map<StringView, CookieStorageInfo> getResponseCookies() const;

	StringView getCookie(StringView name, bool removeFromHeadersTable = true) const;

	Status redirectTo(StringView location);
	Status sendFile(StringView path, size_t cacheTimeInSeconds = maxOf<size_t>());
	Status sendFile(StringView path, StringView contentType, size_t cacheTimeInSeconds = maxOf<size_t>());

	Status runPug(const StringView & path, const Function<bool(pug::Context &, const pug::Template &)> & = nullptr);

	String getFullHostname(int port = -1) const;

	// true if successful cache test
	bool checkCacheHeaders(Time, const StringView &etag);
	bool checkCacheHeaders(Time, uint32_t idHash);

	const db::InputConfig & getInputConfig() const;

	void setInputConfig(const db::InputConfig &);

	// check if request is sent by server/handler administrator
	// uses 'User::isAdmin' or tries to authorize admin by cross-server protocol
	bool isAdministrative();

	void setUser(db::User *);
	void setUser(int64_t);

	// try to get user data from session (by 'getSession' call)
	// if session is not existed - returns nullptr
	// if session is anonymous - returns nullptr
	db::User *getUser();
	db::User *getAuthorizedUser() const;

	int64_t getUserId() const;

	bool isSecureConnection() const;

	RequestController *config() const;
	Host host() const;
	pool_t *pool() const;

	db::AccessRoleId getAccessRole() const;
	void setAccessRole(db::AccessRoleId) const;

	db::Transaction acquireDbTransaction() const;

	const Vector<Value> & getDebugMessages() const;
	const Vector<Value> & getErrorMessages() const;

	template <typename Source, typename Text>
	void addError(Source &&source, Text &&text) const {
		addErrorMessage(Value{
			std::make_pair("source", Value(std::forward<Source>(source))),
			std::make_pair("text", Value(std::forward<Text>(text)))
		});
	}

	template <typename Source, typename Text>
	void addError(Source &&source, Text &&text, Value &&d) const {
		addErrorMessage(Value{
			std::make_pair("source", Value(std::forward<Source>(source))),
			std::make_pair("text", Value(std::forward<Text>(text))),
			std::make_pair("data", std::move(d))
		});
	}

	template <typename Source, typename Text>
	void addDebug(Source &&source, Text &&text) const {
		addDebugMessage(Value{
			std::make_pair("source", Value(std::forward<Source>(source))),
			std::make_pair("text", Value(std::forward<Text>(text)))
		});
	}

	template <typename Source, typename Text>
	void addDebug(Source &&source, Text &&text, Value &&d) const {
		addDebugMessage(Value{
			std::make_pair("source", Value(std::forward<Source>(source))),
			std::make_pair("text", Value(std::forward<Text>(text))),
			std::make_pair("data", std::move(d))
		});
	}

	void addErrorMessage(Value &&) const;
	void addDebugMessage(Value &&) const;

	void addCleanup(Function<void()> &&) const;

	void storeObject(void *ptr, const StringView &key, Function<void()> && = nullptr) const;

	template <typename T = void>
	T *getObject(const StringView &key) const {
		return pool::get<T>(pool(), key);
	}

	bool performWithStorage(const Callback<bool(const db::Transaction &)> &cb) const;

	Session *getSession();
	Session *authorizeUser(db::User *, TimeInterval maxAge);

	void setInputFilter(InputFilter *);
	InputFilter *getInputFilter() const;

protected:
	void initScriptContext(pug::Context &ctx);

	/* Buffer class used as basic_streambuf to allow stream writing to request
	 * like 'request << "String you want to send"; */
	class Buffer : public std::basic_streambuf<char, std::char_traits<char>> {
	public:
		using int_type = typename traits_type::int_type;
		using pos_type = typename traits_type::pos_type;
		using off_type = typename traits_type::off_type;

		using streamsize = std::streamsize;
		using streamoff = std::streamoff;

		using ios_base = std::ios_base;

		Buffer(RequestController *r);
		Buffer(Buffer&&);
		Buffer& operator=(Buffer&&);

		Buffer(const Buffer&);
		Buffer& operator=(const Buffer&);

	protected:
		virtual int_type overflow(int_type c = traits_type::eof()) override;

		virtual pos_type seekoff(off_type off, ios_base::seekdir way, ios_base::openmode) override;
		virtual pos_type seekpos(pos_type pos, ios_base::openmode mode) override;

		virtual int sync() override;

		virtual streamsize xsputn(const char_type* s, streamsize n) override;

		RequestController *_config = nullptr;
	};

	Buffer _buffer;
	RequestController *_config = nullptr;
};

}

#endif /* EXTRA_WEBSERVER_WEBSERVER_REQUEST_SPWEBREQUEST_H_ */
