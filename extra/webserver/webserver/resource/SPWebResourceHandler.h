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

#ifndef EXTRA_WEBSERVER_WEBSERVER_RESOURCE_SPWEBRESOURCEHANDLER_H_
#define EXTRA_WEBSERVER_WEBSERVER_RESOURCE_SPWEBRESOURCEHANDLER_H_

#include "SPWebRequestHandler.h"

namespace STAPPLER_VERSIONIZED stappler::web {

class Resource;

class ResourceHandler : public RequestHandler {
public:
	using Scheme = db::Scheme;

	ResourceHandler(const Scheme &scheme, const Value & = Value());

	virtual bool isRequestPermitted(Request &) override;

	virtual Status onTranslateName(Request &) override;
	virtual void onInsertFilter(Request &) override;
	virtual Status onHandler(Request &) override;

	virtual void onFilterComplete(InputFilter *f) override;

	Status writeInfoToReqest(Request &rctx);
	Status writeToRequest(Request &rctx);

protected:
	Status writeDataToRequest(Request &rctx, Value &&objs);
	Status getHintedStatus(Status) const;

	virtual Resource *getResource(Request &);

	RequestMethod _method = RequestMethod::Get;
	const db::Scheme &_scheme;
	Resource *_resource = nullptr;
	Value _value;
	db::Transaction _transaction = nullptr;
};

class ResourceMultiHandler : public RequestHandler {
public:
	using Scheme = db::Scheme;

	ResourceMultiHandler(const Map<StringView, const Scheme *> &);

	virtual bool isRequestPermitted(Request &) override;
	virtual Status onTranslateName(Request &) override;

protected:
	Status writeDataToRequest(Request &rctx, Value &&objs);

	Value resultData;
	Map<StringView, const Scheme *> _schemes;
	db::Transaction _transaction = nullptr;
};

}

#endif /* EXTRA_WEBSERVER_WEBSERVER_RESOURCE_SPWEBRESOURCEHANDLER_H_ */
