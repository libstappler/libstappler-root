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

#ifndef EXTRA_WEBSERVER_WEBSERVER_TOOLS_SPWEBTOOLS_H_
#define EXTRA_WEBSERVER_WEBSERVER_TOOLS_SPWEBTOOLS_H_

#include "SPWebInfo.h"
#include "SPWebRequestHandler.h"
#include "SPWebWebsocketManager.h"

namespace STAPPLER_VERSIONIZED stappler::web {

class Resource;

StringView getCompileDate();
Time getCompileUnixTime();

}

namespace STAPPLER_VERSIONIZED stappler::web::tools {

/* Simple auth handler, creates ащгк virtual pages:
 *
 * $PREFIX$/auth/setup? name=$NAME$ & passwd=$PASSWD$
 *  - create new admin user, if there is no other users
 *
 * $PREFIX$/auth/login? name=$NAME$ & passwd=$PASSWD$ & maxAge=$MAXAGE$
 *  - init new session for user, creates new token pair for $MAXAGE$ (720 seconds max)
 *
 * $PREFIX$/auth/update? token=$TOKEN$ & maxAge=$MAXAGE$
 *  - updates session token pair for $MAXAGE$ (720 seconds max)
 *
 * $PREFIX$/auth/cancel? token=$TOKEN$
 *  - cancel session
 *
 */
class SP_PUBLIC AuthHandler : public DataHandler {
public:
	virtual bool isRequestPermitted(Request &) override;
	virtual Status onTranslateName(Request &) override;
	virtual bool processDataHandler(Request &, Value &result, Value &input) override;
};

/* WebSocket shell interface */
class SP_PUBLIC ShellSocket : public WebsocketManager {
public:
	SP_COVERAGE_TRIVIAL
	virtual ~ShellSocket() = default;

	ShellSocket(const Host &host) : WebsocketManager(host) { };

	virtual WebsocketHandler * onAccept(const Request &, pool_t *) override;
	virtual bool onBroadcast(const Value &) override;
};

/* WebSocket shell interface GUI */
class SP_PUBLIC ShellGui : public RequestHandler {
public:
	SP_COVERAGE_TRIVIAL
	virtual bool isRequestPermitted(Request &) override { return true; }
	virtual Status onPostReadRequest(Request &) override;

	virtual void onInsertFilter(Request &) override;
	virtual Status onHandler(Request &) override;

	virtual void onFilterComplete(InputFilter *f) override;

protected:
	Resource *_resource = nullptr;
};

/* WebSocket shell interface GUI */
class SP_PUBLIC ServerGui : public DataHandler {
public:
	static void defineBasics(pug::Context &exec, Request &req, db::User *u);

	ServerGui() {
		_allow = AllowMethod::Get | AllowMethod::Post;
		_config = db::InputConfig({
			db::InputConfig::Require::Data | db::InputConfig::Require::FilesAsData,
			512,
			256,
			0
		});
	}

	virtual bool isRequestPermitted(Request &) override;
	virtual Status onTranslateName(Request &) override;
	virtual void onFilterComplete(InputFilter *f) override;

protected:
	db::Transaction _transaction = nullptr;
};

class SP_PUBLIC TestHandler : public DataHandler {
public:
	TestHandler();
	virtual bool isRequestPermitted(Request &) override;
	virtual bool processDataHandler(Request &, Value &, Value &) override;

protected:
	bool processEmailTest(Request &rctx, Value &ret, const Value &input);
	bool processUrlTest(Request &rctx, Value &ret, const Value &input);
	bool processUserTest(Request &rctx, Value &ret, const Value &input);
	bool processImageTest(Request &rctx, Value &ret, const Value &input, db::InputFile &);
};

class SP_PUBLIC ErrorsGui : public RequestHandler {
public:
	virtual bool isRequestPermitted(Request &) override { return true; }
	virtual Status onTranslateName(Request &) override;
};

class SP_PUBLIC HandlersGui : public RequestHandler {
public:
	virtual bool isRequestPermitted(Request &) override { return true; }
	virtual Status onTranslateName(Request &) override;
};

class SP_PUBLIC ReportsGui : public RequestHandler {
public:
	virtual bool isRequestPermitted(Request &) override { return true; }
	virtual Status onTranslateName(Request &) override;
};

class SP_PUBLIC VirtualGui : public RequestHandler {
public:
	virtual bool isRequestPermitted(Request &) override { return true; }
	virtual Status onTranslateName(Request &) override;
	virtual Status onHandler(Request &) override;

	virtual void onInsertFilter(Request &) override;
	virtual void onFilterComplete(InputFilter *filter) override;

protected:
	Value readMeta(StringView) const;
	void writeData(Value &) const;

	bool createArticle(const Value &);
	bool createCategory(const Value &);

	void makeMdContents(Request &req, pug::Context &exec, StringView path) const;
	Value makeDirInfo(StringView path, bool forFile = false) const;

	bool _virtual = true;
#if DEBUG
	bool _editable = true;
#else
	bool _editable = false;
#endif
};

class SP_PUBLIC VirtualFilesystem : public RequestHandler {
public:
	virtual bool isRequestPermitted(Request &) override { return true; }
	virtual Status onTranslateName(Request &) override;
};

SP_PUBLIC void registerTools(StringView prefix, Host &);

}

#endif /* EXTRA_WEBSERVER_WEBSERVER_TOOLS_SPWEBTOOLS_H_ */
