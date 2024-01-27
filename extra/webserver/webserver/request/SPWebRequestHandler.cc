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

#include "SPWebRequestHandler.h"
#include "SPWebInputFilter.h"
#include "SPDbFile.h"

namespace stappler::web {

Status RequestHandler::onRequestRecieved(Request & rctx, StringView originPath, StringView path, const Value &data) {
	_request = rctx;
	_originPath = originPath;
	_subPath = path;
	_options = data;
	_subPathVec = UrlView::parsePath<Interface>(_subPath);
	return OK;
}

Status DataHandler::writeResult(Value &data) {
	auto status = _request.getInfo().status;
	if (status >= 400) {
		return status;
	}

	data.setInteger(Time::now().toMicros(), "date");
#if DEBUG
	auto &debug = _request.getDebugMessages();
	if (!debug.empty()) {
		data.setArray(debug, "debug");
	}
#endif
	auto &error = _request.getErrorMessages();
	if (!error.empty()) {
		data.setArray(error, "errors");
	}

	_request.writeData(data, allowJsonP());
	return DONE;
}

static bool isMethodAllowed(RequestMethod r, DataHandler::AllowMethod a) {
	if ((r == RequestMethod::Get && (a & DataHandler::AllowMethod::Get) != 0)
			|| (r == RequestMethod::Delete && (a & DataHandler::AllowMethod::Delete) != 0)
			|| (r == RequestMethod::Put && (a & DataHandler::AllowMethod::Put) != 0)
			|| (r == RequestMethod::Post && (a & DataHandler::AllowMethod::Post) != 0)) {
		return true;
	}

	return false;
}

Status DataHandler::onTranslateName(Request &rctx) {
	if (!isMethodAllowed(rctx.getInfo().method, _allow)) {
		return HTTP_METHOD_NOT_ALLOWED;
	}

	if ((rctx.getInfo().method == RequestMethod::Get && (_allow & AllowMethod::Get) != AllowMethod::None)
			|| (rctx.getInfo().method == RequestMethod::Delete && (_allow & AllowMethod::Delete) != AllowMethod::None)) {
		bool result = false;
		Value data;

		Value input;
		result = processDataHandler(rctx, data, input);
		data.setBool(result, "OK");
		return writeResult(data);
	}

	return DECLINED;
}

void DataHandler::onInsertFilter(Request &rctx) {
	if ((rctx.getInfo().method == RequestMethod::Post && (_allow & AllowMethod::Post) != AllowMethod::None)
			|| (rctx.getInfo().method == RequestMethod::Put && (_allow & AllowMethod::Put) != AllowMethod::None)) {
		rctx.setInputConfig(_config);
	}

	if (rctx.getInfo().method == RequestMethod::Put || rctx.getInfo().method == RequestMethod::Post) {
		auto ex = InputFilter::insert(rctx);
		if (ex != InputFilter::Exception::None) {
			if (ex == InputFilter::Exception::TooLarge) {
				rctx.setStatus(HTTP_REQUEST_ENTITY_TOO_LARGE);
			} else if (ex == InputFilter::Exception::Unrecognized) {
				rctx.setStatus(HTTP_UNSUPPORTED_MEDIA_TYPE);
			}
		}
	}
}

Status DataHandler::onHandler(Request &) {
	return OK;
}

void DataHandler::onFilterComplete(InputFilter *filter) {
	bool result = false;
	Value data;
	Request rctx(filter->getRequest());
	_filter = filter;

	Value input(filter->getData());
	for (auto &it : filter->getFiles()) {
		input.setInteger(it.negativeId(), it.name);
	}

	result = processDataHandler(rctx, data, input);

	data.setBool(result, "OK");
	writeResult(data);
}

FilesystemHandler::FilesystemHandler(const String &path, size_t cacheTime) : _path(path), _cacheTime(cacheTime) { }
FilesystemHandler::FilesystemHandler(const String &path, const String &ct, size_t cacheTime)
: _path(path), _contentType(ct), _cacheTime(cacheTime) { }

bool FilesystemHandler::isRequestPermitted(Request &) {
	return true;
}
Status FilesystemHandler::onTranslateName(Request &rctx) {
	auto &info = rctx.getInfo();
	if (info.url.path == "/") {
		return rctx.sendFile(stappler::filesystem::writablePath<Interface>(_path), std::move(_contentType), _cacheTime);
	} else {
		auto npath = stappler::filesystem::writablePath<Interface>(info.url.path, true);
		if (stappler::filesystem::exists(npath) && _subPath != "/") {
			return DECLINED;
		}
		return rctx.sendFile(stappler::filesystem::writablePath<Interface>(_path), std::move(_contentType), _cacheTime);
	}
}

void RequestHandlerMap::Handler::onParams(const HandlerInfo *info, Value &&val) {
	_info = info;
	_params = std::move(val);
}

Status RequestHandlerMap::Handler::onTranslateName(Request &rctx) {
	auto &info = rctx.getInfo();
	if (info.method != _info->getMethod()) {
		return HTTP_METHOD_NOT_ALLOWED;
	}

	switch (info.method) {
	case RequestMethod::Post:
	case RequestMethod::Put:
	case RequestMethod::Patch:
		return OK;
		break;
	default: {
		if (!processQueryFields(Value(info.queryData))) {
			return HTTP_BAD_REQUEST;
		}
		bool hasLocation = false;
		auto ret = onRequest();
		if (ret == DECLINED) {
			if (auto data = onData()) {
				auto loc = _request.getResponseHeader("Location");
				if (!loc.empty()) {
					hasLocation = true;
				} else {
					data.setBool(true, "OK");
					return writeResult(data);
				}
			} else {
				auto loc = _request.getResponseHeader("Location");
				if (!loc.empty()) {
					hasLocation = true;
				} else {
					Value retVal({ stappler::pair("OK", Value(false)) });
					return writeResult(retVal);
				}
			}
		}
		if (ret <= 0 && hasLocation) {
			return HTTP_SEE_OTHER;
		}
		return ret;
		break;
	}
	}
	return HTTP_BAD_REQUEST;
}

void RequestHandlerMap::Handler::onInsertFilter(Request &rctx) {
	auto &info = rctx.getInfo();
	switch (info.method) {
	case RequestMethod::Post:
	case RequestMethod::Put:
	case RequestMethod::Patch: {
		auto cfg = _info->getInputConfig();
		cfg.required = db::InputConfig::Require::Data | db::InputConfig::Require::Files;
		rctx.setInputConfig(cfg);

		auto ex = InputFilter::insert(rctx);
		if (ex != InputFilter::Exception::None) {
			if (ex == InputFilter::Exception::TooLarge) {
				rctx.setStatus(HTTP_REQUEST_ENTITY_TOO_LARGE);
			} else if (ex == InputFilter::Exception::Unrecognized) {
				rctx.setStatus(HTTP_UNSUPPORTED_MEDIA_TYPE);
			}
		}
		break;
	}
	default:
		break;
	}
}

Status RequestHandlerMap::Handler::onHandler(Request &) {
	return OK;
}

void RequestHandlerMap::Handler::onFilterComplete(InputFilter *filter) {
	auto &info = _request.getInfo();

	_filter = filter;

	if (!processQueryFields(Value(info.queryData))) {
		_request.setStatus(HTTP_BAD_REQUEST);
		return;
	}
	if (!processInputFields(_filter)) {
		_request.setStatus(HTTP_BAD_REQUEST);
		return;
	}

	bool hasLocation = false;
	auto ret = onRequest();
	if (ret == DECLINED) {
		if (auto data = onData()) {
			auto loc = _request.getResponseHeader("Location");
			if (!loc.empty()) {
				hasLocation = true;
			} else {
				data.setBool(true, "OK");
				writeResult(data);
			}
		} else {
			auto loc = _request.getResponseHeader("Location");
			if (!loc.empty()) {
				hasLocation = true;
			} else {
				Value retVal({ stappler::pair("OK", Value(false)) });
				writeResult(retVal);
				return;
			}
		}
	}
	if (ret > 0) {
		_request.setStatus(ret);
	} else if (hasLocation) {
		_request.setStatus(HTTP_SEE_OTHER);
	}
}

bool RequestHandlerMap::Handler::processQueryFields(Value &&args) {
	_queryFields = std::move(args);
	if (_info->getQueryScheme().getFields().empty()) {
		return true;
	}

	_info->getQueryScheme().transform(_queryFields, db::Scheme::TransformAction::ProtectedCreate);

	bool success = true;
	for (auto &it : _info->getQueryScheme().getFields()) {
		auto &val = _queryFields.getValue(it.first);
		if (val.isNull() && it.second.hasFlag(db::Flags::Required)) {
			_request.addError("HandlerMap", "No value for required field",
					Value({ std::make_pair("field", Value(it.first)) }));
			success = false;
		}
	}
	return success;
}

bool RequestHandlerMap::Handler::processInputFields(InputFilter *filter) {
	_inputFields = std::move(filter->getData());

	if (_info->getInputScheme().getFields().empty()) {
		return true;
	}

	_info->getInputScheme().transform(_inputFields, db::Scheme::TransformAction::ProtectedCreate);

	for (auto &it : filter->getFiles()) {
		if (auto f = _info->getInputScheme().getField(it.name)) {
			if (db::File::validateFileField(_request.host().getRoot(), *f, it)) {
				_inputFields.setInteger(it.negativeId(), it.name);
			}
		}
	}

	bool success = true;
	for (auto &it : _info->getInputScheme().getFields()) {
		auto &val = _inputFields.getValue(it.first);
		if (val.isNull() && it.second.hasFlag(db::Flags::Required)) {
			_request.addError("HandlerMap", "No value for required field",
					Value({ std::make_pair("field", Value(it.first)) }));
			success = false;
		}
	}
	return success;
}

Status RequestHandlerMap::Handler::writeResult(Value &data) {
	auto status = _request.getInfo().status;
	if (status >= 400) {
		return status;
	}

	data.setInteger(Time::now().toMicros(), "date");
#if DEBUG
	auto &debug = _request.getDebugMessages();
	if (!debug.empty()) {
		data.setArray(debug, "debug");
	}
#endif
	auto &error = _request.getErrorMessages();
	if (!error.empty()) {
		data.setArray(error, "errors");
	}

	_request.writeData(data, allowJsonP());
	return DONE;
}

db::InputFile *RequestHandlerMap::Handler::getInputFile(const StringView &name) {
	if (!_filter) {
		return nullptr;
	}

	for (auto &it : _filter->getFiles()) {
		if (it.name == name) {
			return &it;
		}
	}

	return nullptr;
}


RequestHandlerMap::HandlerInfo::HandlerInfo(const StringView &name, RequestMethod m, const StringView &pt,
		Function<Handler *()> &&cb, Value &&opts)
: name(name.str<Interface>()), method(m), pattern(pt.str<Interface>()), handler(std::move(cb)), options(std::move(opts))
, queryFields(name), inputFields(name) {
	StringView p(pattern);
	while (!p.empty()) {
		auto tmp = p.readUntil<StringView::Chars<':'>>();
		if (!tmp.empty()) {
			fragments.emplace_back(Fragment::Text, tmp);
		}
		if (p.is(':')) {
			auto tmp = p;
			++ p;
			auto ptrn = p.readUntil<StringView::Chars<'/', '.', ':', '#', '?', ','>>();
			if (!ptrn.empty()) {
				fragments.emplace_back(Fragment::Pattern, StringView(tmp.data(), ptrn.size() + 1));
			}
		}
	}
}

RequestHandlerMap::HandlerInfo &RequestHandlerMap::HandlerInfo::addQueryFields(std::initializer_list<db::Field> il) {
	queryFields.define(il);
	return *this;
}
RequestHandlerMap::HandlerInfo &RequestHandlerMap::HandlerInfo::addQueryFields(Vector<db::Field> &&il) {
	queryFields.define(std::move(il));
	return *this;
}

RequestHandlerMap::HandlerInfo &RequestHandlerMap::HandlerInfo::addInputFields(std::initializer_list<db::Field> il) {
	inputFields.define(il);
	return *this;
}
RequestHandlerMap::HandlerInfo &RequestHandlerMap::HandlerInfo::addInputFields(Vector<db::Field> &&il) {
	inputFields.define(std::move(il));
	return *this;
}

RequestHandlerMap::HandlerInfo &RequestHandlerMap::HandlerInfo::setInputConfig(db::InputConfig cfg) {
	inputFields.setConfig(cfg);
	return *this;
}

Value RequestHandlerMap::HandlerInfo::match(const StringView &path, size_t &match) const {
	size_t nmatch = 0;
	Value ret({ stappler::pair("path", Value(path)) });
	auto it = fragments.begin();
	StringView r(path);
	while (!r.empty() && it != fragments.end()) {
		switch (it->type) {
		case Fragment::Text:
			if (r.starts_with(StringView(it->string))) {
				r += it->string.size();
				nmatch += it->string.size();
				++ it;
			} else {
				return Value();
			}
			break;
		case Fragment::Pattern:
			if (StringView(it->string).is(':')) {
				StringView name(it->string.data() + 1, it->string.size() - 1);
				if (name.empty()) {
					return Value();
				}

				++ it;
				if (it != fragments.end()) {
					auto tmp = r.readUntilString(it->string);
					if (tmp.empty()) {
						return Value();
					}
					ret.setString(tmp, name.str<Interface>());
				} else {
					ret.setString(r, name.str<Interface>());
					r += r.size();
				}
			}
			break;
		}
	}

	if (!r.empty() || it != fragments.end()) {
		return Value();
	}

	match = nmatch;
	return ret;
}

RequestHandlerMap::Handler *RequestHandlerMap::HandlerInfo::onHandler(Value &&p) const {
	if (auto h = handler()) {
		h->onParams(this, std::move(p));
		return h;
	}
	return nullptr;
}

RequestMethod RequestHandlerMap::HandlerInfo::getMethod() const {
	return method;
}

const db::InputConfig &RequestHandlerMap::HandlerInfo::getInputConfig() const {
	return inputFields.getConfig();
}

StringView RequestHandlerMap::HandlerInfo::getName() const {
	return name;
}
StringView RequestHandlerMap::HandlerInfo::getPattern() const {
	return pattern;
}
const Value &RequestHandlerMap::HandlerInfo::getOptions() const {
	return options;
}

const db::Scheme &RequestHandlerMap::HandlerInfo::getQueryScheme() const {
	return queryFields;
}
const db::Scheme &RequestHandlerMap::HandlerInfo::getInputScheme() const {
	return inputFields;
}

RequestHandlerMap::RequestHandlerMap() { }

RequestHandlerMap::~RequestHandlerMap() { }

RequestHandlerMap::Handler *RequestHandlerMap::onRequest(Request &req, const StringView &ipath) const {
	auto &reqInfo = req.getInfo();
	StringView path(ipath.empty() ? StringView("/") : ipath);
	const HandlerInfo *info = nullptr;
	Value params;
	size_t score = 0;
	for (auto &it : _handlers) {
		size_t pscore = 0;
		if (auto val = it.match(path, pscore)) {
			if (pscore > score || (pscore == score && info && it.getMethod() == reqInfo.method && it.getMethod() != info->getMethod())) {
				params = std::move(val);
				if (it.getMethod() == reqInfo.method || !info) {
					info = &it;
					score = pscore;
				}
			}
		}
	}

	if (info) {
		return info->onHandler(std::move(params));
	}

	return nullptr;
}

const Vector<RequestHandlerMap::HandlerInfo> &RequestHandlerMap::getHandlers() const {
	return _handlers;
}

RequestHandlerMap::HandlerInfo &RequestHandlerMap::addHandler(const StringView &name, RequestMethod m, const StringView &pattern,
		Function<Handler *()> &&cb, Value &&opts) {
	_handlers.emplace_back(name, m, pattern, std::move(cb), std::move(opts));
	return _handlers.back();
}


class HandlerCallback : public RequestHandlerMap::Handler {
public: // simplified interface
	HandlerCallback(const Function<bool(Handler &)> &accessControl, const Function<Value(Handler &)> &process)
	: _accessControl(accessControl), _process(process) { }

	virtual ~HandlerCallback() { }

	virtual bool isPermitted() override { return _process && _accessControl && _accessControl(*this); }
	virtual Value onData() override {
		auto ret = _process(*this);
		if (ret) {
			if (_info->getOptions().isString("location")) {
				auto locVar = _info->getOptions().getString("location");
				auto loc = StringView(_request.getInfo().queryData.getString(locVar));
				if (!loc.empty()) {
					if (loc.starts_with("/") || loc.starts_with(StringView(_request.getFullHostname()))) {
						_request.redirectTo(loc);
					}
				}
			}
		}
		return ret;
	}

public:
	Function<bool(Handler &)> _accessControl;
	Function<Value(Handler &)> _process;
};

RequestHandlerMap::HandlerInfo &RequestHandlerMap::addHandler(const StringView &name, RequestMethod m, const StringView &pattern,
		Function<bool(Handler &)> &&accessControl, Function<Value(Handler &)> &&process, Value &&opts) {
	return addHandler(name, m, pattern, [accessControl = std::move(accessControl), process = std::move(process)] () -> Handler * {
		return new HandlerCallback(accessControl, process);
	}, std::move(opts));
}

}
