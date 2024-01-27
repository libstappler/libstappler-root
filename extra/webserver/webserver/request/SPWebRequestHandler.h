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

#ifndef EXTRA_WEBSERVER_WEBSERVER_REQUEST_SPWEBREQUESTHANDLER_H_
#define EXTRA_WEBSERVER_WEBSERVER_REQUEST_SPWEBREQUESTHANDLER_H_

#include "SPWebRequest.h"

namespace stappler::web {

class InputFilter;

class RequestHandler : public AllocBase {
public:
	using HandlerCallback = Function<RequestHandler *()>;

	template <typename T, typename ... Args>
	static HandlerCallback Handler(Args && ... args) {
		return HandlerCallback([=] {
			return new T(std::forward<Args>(args)...);
		});
	}

	virtual ~RequestHandler() { }

	virtual bool isRequestPermitted(Request &) { return false; }

	/**
	 * https://developer.mozilla.org/en-US/docs/HTTP/Access_control_CORS
	 *
     * @param rctx - new request, avoid to modify it
     * @param origin - actual value of Origin header, it's sender's domain name
     * @param isPreflight - we recieve preflight request (so, method and headers should be filled)
     * @param method - method for preflighted request
     * @param headers - extra (X-*) headers for preflighted request
     * @return for forbidden CORS requests server will return "405 Method Not Allowed"
     */
	virtual bool isCorsPermitted(Request &, const StringView &origin, bool isPreflight = false,
		const StringView &method = "", const StringView &headers = "") { return true; }

	/**
	 * Available method for CORS preflighted requests
     */
	virtual StringView getCorsAllowMethods(Request &) {
		return "GET, HEAD, POST, PUT, DELETE, OPTIONS";
	}

	/**
	 * Available extra headers for CORS preflighted requests
     */
	virtual StringView getCorsAllowHeaders(Request &) {
		return StringView();
	}

	/**
	 * Caching time for preflight response
     */
	virtual StringView getCorsMaxAge(Request &) {
		return "1728000"; // 20 days
	}

	/** Be sure to call supermethod when overload this method! */
	virtual Status onRequestRecieved(Request &, StringView origin, StringView path, const Value &);

	virtual Status onPostReadRequest(Request &) { return DECLINED; }
	virtual Status onTranslateName(Request &) { return DECLINED; }
	virtual Status onQuickHandler(Request &, int v) { return DECLINED; }
	virtual void onInsertFilter(Request &) { }
	virtual Status onHandler(Request &) { return DECLINED; }

	virtual void onFilterInit(InputFilter *f) { }
	virtual void onFilterUpdate(InputFilter *f) { }
	virtual void onFilterComplete(InputFilter *f) { }

	virtual const Value &getOptions() const { return _options; }

	void setAccessRole(db::AccessRoleId role) { _accessRole = role; }
	db::AccessRoleId getAccessRole() const { return _accessRole; }

protected:
	Request _request;
	StringView _originPath;
	StringView _subPath;
	Vector<StringView> _subPathVec;
	Value _options;
	db::AccessRoleId _accessRole = db::AccessRoleId::Nobody;
};

class DefaultHandler : public RequestHandler {
public:
	virtual bool isRequestPermitted(Request &) override { return true; }
};

class DataHandler : public RequestHandler {
public:
	enum class AllowMethod : uint8_t {
		None = 0,
		Get = 1 << 0,
		Post = 1 << 1,
		Put = 1 << 2,
		Delete = 1 << 3,
		All = 0xFF,
	};

	virtual ~DataHandler() { }

	// overload point
	virtual bool processDataHandler(Request &req, Value &result, Value &input) { return false; }

	virtual Status onTranslateName(Request &) override;
	virtual void onInsertFilter(Request &) override;
	virtual Status onHandler(Request &) override;

	virtual void onFilterComplete(InputFilter *f) override;

protected:
	virtual bool allowJsonP() { return true; }

	Status writeResult(Value &);

	AllowMethod _allow = AllowMethod::All;
	db::InputConfig _config = db::InputConfig({
		db::InputConfig::Require::Data | db::InputConfig::Require::FilesAsData,
		0,
		256,
		0
	});
	InputFilter *_filter;
};

SP_DEFINE_ENUM_AS_MASK(DataHandler::AllowMethod)

class FilesystemHandler : public RequestHandler {
public:
	FilesystemHandler(const String &path, size_t cacheTimeInSeconds = stappler::maxOf<size_t>());
	FilesystemHandler(const String &path, const String &ct, size_t cacheTimeInSeconds = stappler::maxOf<size_t>());

	virtual bool isRequestPermitted(Request &) override;
	virtual Status onTranslateName(Request &) override;

protected:
	String _path;
	String _contentType;
	size_t _cacheTime;
};

class RequestHandlerMap : public AllocBase {
public:
	struct HandlerInfo;

	class Handler : public RequestHandler {
	public: // simplified interface
		virtual bool isPermitted() { return false; }
		virtual Status onRequest() { return DECLINED; }
		virtual Value onData() { return Value(); }

	public:
		Handler() { }
		virtual ~Handler() { }

		virtual void onParams(const HandlerInfo *, Value &&);
		virtual bool isRequestPermitted(Request &) override { return isPermitted(); }
		virtual Status onTranslateName(Request &) override;
		virtual void onInsertFilter(Request &) override;
		virtual Status onHandler(Request &) override;

		virtual void onFilterComplete(InputFilter *f);

		const Request &getRequest() const { return _request; }
		const Value &getParams() const { return _params; }
		const Value &getQueryFields() const { return _queryFields; }
		const Value &getInputFields() const { return _inputFields; }

	protected:
		virtual bool allowJsonP() { return true; }

		bool processQueryFields(Value &&);
		bool processInputFields(InputFilter *);

		Status writeResult(Value &);

		db::InputFile *getInputFile(const StringView &);

		const HandlerInfo *_info = nullptr;
		InputFilter *_filter = nullptr;

		Value _params; // query path params
		Value _queryFields;
		Value _inputFields;
	};

	class HandlerInfo : public AllocBase {
	public:
		HandlerInfo(const StringView &name, RequestMethod, const StringView &pattern,
				Function<Handler *()> &&, Value && = Value());

		HandlerInfo &addQueryFields(std::initializer_list<db::Field> il);
		HandlerInfo &addQueryFields(Vector<db::Field> &&il);

		HandlerInfo &addInputFields(std::initializer_list<db::Field> il);
		HandlerInfo &addInputFields(Vector<db::Field> &&il);
		HandlerInfo &setInputConfig(db::InputConfig);

		Value match(const StringView &path, size_t &score) const;

		Handler *onHandler(Value &&) const;

		RequestMethod getMethod() const;
		const db::InputConfig &getInputConfig() const;

		StringView getName() const;
		StringView getPattern() const;
		const Value &getOptions() const;

		const db::Scheme &getQueryScheme() const;
		const db::Scheme &getInputScheme() const;

	protected:
		struct Fragment {
			enum Type : uint16_t {
				Text,
				Pattern,
			};

			Fragment(Type t, StringView s) : type(t), string(s.str<Interface>()) { }

			Type type;
			String string;
		};

		String name;
		RequestMethod method = RequestMethod::Get;
		String pattern;
		Function<Handler *()> handler;
		Value options;

		db::Scheme queryFields;
		db::Scheme inputFields;
		Vector<Fragment> fragments;
	};

	RequestHandlerMap();
	virtual ~RequestHandlerMap();

	RequestHandlerMap(RequestHandlerMap &&) = default;
	RequestHandlerMap &operator=(RequestHandlerMap &&) = default;

	RequestHandlerMap(const RequestHandlerMap &) = delete;
	RequestHandlerMap &operator=(const RequestHandlerMap &) = delete;

	Handler *onRequest(Request &req, const StringView &path) const;

	HandlerInfo &addHandler(const StringView &name, RequestMethod, const StringView &pattern,
			Function<Handler *()> &&, Value && = Value());

	HandlerInfo &addHandler(const StringView &name, RequestMethod, const StringView &pattern,
			Function<bool(Handler &)> &&, Function<Value(Handler &)> &&, Value && = Value());

	const Vector<HandlerInfo> &getHandlers() const;

protected:
	Vector<HandlerInfo> _handlers;
};

}

#endif /* EXTRA_WEBSERVER_WEBSERVER_REQUEST_SPWEBREQUESTHANDLER_H_ */
