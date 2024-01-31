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

#include "SPWebResourceHandler.h"
#include "SPWebResource.h"
#include "SPWebRequest.h"
#include "SPWebRequestController.h"
#include "SPWebOutput.h"
#include "SPWebRoot.h"
#include "SPWebInputFilter.h"

namespace STAPPLER_VERSIONIZED stappler::web {

ResourceHandler::ResourceHandler(const db::Scheme &scheme, const Value &val)
: _scheme(scheme), _value(val) { }

bool ResourceHandler::isRequestPermitted(Request &rctx) {
	_transaction = db::Transaction::acquire(db::Adapter(rctx.getController()->acquireDatabase()));
	return true;
}

Status ResourceHandler::onTranslateName(Request &rctx) {
	if (!isRequestPermitted(rctx)) {
		return HTTP_FORBIDDEN;
	}

	_method = rctx.getInfo().method;
	if (_method != RequestMethod::Get && _method != RequestMethod::Post && _method != RequestMethod::Put
			&& _method != RequestMethod::Delete && _method != RequestMethod::Patch) {
		return HTTP_NOT_IMPLEMENTED;
	}

	auto &data = rctx.getInfo().queryData;
	if (data.isString("METHOD")) {
		auto method = data.getString("METHOD");
		if (_method == RequestMethod::Get) {
			if (method == "DELETE") {
				_method = RequestMethod::Delete;
			}
		} else if (_method == RequestMethod::Post) {
			if (method == "PUT") {
				_method = RequestMethod::Put;
			} else if (method == "PATCH") {
				_method = RequestMethod::Patch;
			}
		}
	}

	_resource = getResource(rctx);
	if (!_resource) {
		return HTTP_NOT_FOUND;
	}

	auto user = rctx.getAuthorizedUser();
	if (!user && data.isString("token")) {
		user = rctx.getUser();
	}

	_resource->setUser(user);
	_resource->setFilterData(_value);

	if (data.hasValue("pageCount")) {
		_resource->setPageCount(data.getInteger("pageCount"));
	}
	if (data.hasValue("pageFrom")) {
		_resource->setPageFrom(data.getInteger("pageFrom"));
	}

	auto args = rctx.getInfo().url.query;
	if (!args.empty() && args.front() == '(') {
		_resource->applyQuery(data);
	} else {
		if (data.hasValue("resolve")) {
			_resource->setResolveOptions(data.getValue("resolve"));
		}
		if (data.hasValue("resolveDepth")) {
			_resource->setResolveDepth(data.getInteger("resolveDepth"));
		}
	}

	switch (_method) {
	case RequestMethod::Get: {
		auto modified = rctx.getRequestHeader("if-modified-since");
		auto mt = modified.empty() ? 0 : Time::fromHttp(modified).toSeconds();

		if (auto d = _resource->getSourceDelta()) {
			rctx.setResponseHeader("Last-Modified", d.toHttp<Interface>());
			if (mt >= d.toSeconds()) {
				return HTTP_NOT_MODIFIED;
			}
		}

		if (mt > 0 && _resource->getType() == ResourceType::Object) {
			if (auto res = dynamic_cast<ResourceObject *>(_resource)) {
				if (auto objMtime = res->getObjectMtime()) {
					if (mt >= uint64_t(objMtime / 1000000)) {
						return HTTP_NOT_MODIFIED;
					}
				}
			}
		}

		_resource->prepare(db::QueryList::SimpleGet);

		if (!rctx.getInfo().headerRequest) {
			return writeToRequest(rctx);
		} else {
			return writeInfoToReqest(rctx);
		}
		break;
	}
	case RequestMethod::Delete:
		_resource->prepare();
		if (_resource->removeObject()) {
			if (data.isString("location")) {
				return rctx.redirectTo(String(data.getString("location")));
			} else if (data.isString("target")) {
				auto &target = data.getString("target");
				if (!target.empty() && (StringView(target).starts_with(StringView(rctx.getFullHostname())) || target[0] == '/')) {
					return rctx.redirectTo(String(target));
				}
			}
			return HTTP_NO_CONTENT;
		} else {
			return getHintedStatus(HTTP_FORBIDDEN);
		}
		break;
	case RequestMethod::Post:
		_resource->prepare();
		if (_resource->prepareCreate()) {
			return DECLINED;
		} else {
			return getHintedStatus(HTTP_FORBIDDEN);
		}
		break;
	case RequestMethod::Put:
		_resource->prepare();
		if (_resource->prepareUpdate()) {
			return DECLINED;
		} else {
			return getHintedStatus(HTTP_FORBIDDEN);
		}
		break;
	case RequestMethod::Patch:
		_resource->prepare();
		if (_resource->prepareAppend()) {
			return DECLINED;
		} else {
			return getHintedStatus(HTTP_FORBIDDEN);
		}
		break;
	default:
		break;
	}

	return HTTP_NOT_IMPLEMENTED;
}

void ResourceHandler::onInsertFilter(Request &rctx) {
	if (_method == RequestMethod::Post || _method == RequestMethod::Put || _method == RequestMethod::Patch) {
		rctx.setInputConfig(db::InputConfig{
			db::InputConfig::Require::Data | db::InputConfig::Require::Files,
			_resource->getMaxRequestSize(),
			_resource->getMaxVarSize(),
			_resource->getMaxFileSize()
		});

		auto ex = InputFilter::insert(rctx);
		if (ex != InputFilter::Exception::None) {
			if (ex == InputFilter::Exception::TooLarge) {
				rctx.setStatus(HTTP_REQUEST_ENTITY_TOO_LARGE);
			} else if (ex == InputFilter::Exception::Unrecognized) {
				rctx.setStatus(HTTP_UNSUPPORTED_MEDIA_TYPE);
			} else {
				rctx.setStatus(HTTP_BAD_REQUEST);
			}
		}
	} else if (_method != RequestMethod::Get && _method != RequestMethod::Delete) {
		rctx.setStatus(HTTP_BAD_REQUEST);
		Root::getCurrent()->error("Resource", "Input data can not be recieved, no available filters");
	}
}

Status ResourceHandler::onHandler(Request &rctx) {
	auto num = rctx.getInfo().method;
	if (num == RequestMethod::Get) {
		return DECLINED;
	} else {
		return OK;
	}
}

void ResourceHandler::onFilterComplete(InputFilter *filter) {
	auto rctx = filter->getRequest();
	if (_method == RequestMethod::Put) {
		// we should update our resource
		auto result = _resource->updateObject(filter->getData(), filter->getFiles());
		if (result) {
			auto &target = rctx.getInfo().queryData.getValue("target");
			if (target.isString() && (StringView(target.getString()).starts_with(StringView(rctx.getFullHostname())) || target.getString()[0] == '/')) {
				rctx.setStatus(rctx.redirectTo(String(target.getString())));
			} else {
				writeDataToRequest(rctx, move(result));
				rctx.setStatus(HTTP_OK);
			}
		} else {
			rctx.setStatus(HTTP_BAD_REQUEST);
			if (result.isNull()) {
				Root::getCurrent()->error("Resource", "Fail to perform update", Value(filter->getData()));
			}
		}
	} else if (_method == RequestMethod::Post) {
		auto &d = filter->getData();
		auto tmp = d;
		auto result = _resource->createObject(d, filter->getFiles());
		if (result) {
			auto &target = rctx.getInfo().queryData.getValue("target");
			if (target.isString() && (StringView(target.getString()).starts_with(StringView(rctx.getFullHostname())) || target.getString()[0] == '/')) {
				rctx.setStatus(rctx.redirectTo(String(target.getString())));
			} else {
				writeDataToRequest(rctx, move(result));
				rctx.setStatus(HTTP_CREATED);
			}
		} else {
			rctx.setStatus(HTTP_BAD_REQUEST);
			if (result.isNull()) {
				Root::getCurrent()->error("Resource", "Fail to perform create", Value(move(tmp)));
			}
		}
	} else if (_method == RequestMethod::Patch) {
		auto result = _resource->appendObject(filter->getData());
		if (result) {
			auto &target = rctx.getInfo().queryData.getValue("target");
			if (target.isString() && (StringView(target.getString()).starts_with(StringView(rctx.getFullHostname())) || target.getString()[0] == '/')) {
				rctx.setStatus(rctx.redirectTo(String(target.getString())));
			} else {
				writeDataToRequest(rctx, move(result));
				rctx.setStatus(HTTP_OK);
			}
		} else {
			rctx.setStatus(HTTP_BAD_REQUEST);
			if (result.isNull()) {
				Root::getCurrent()->error("Resource", "Fail to perform append", Value(filter->getData()));
			}
		}
	}
}

Resource *ResourceHandler::getResource(Request &rctx) {
	return Resource::resolve(_transaction, _scheme, _subPath, _value);
}

Status ResourceHandler::writeDataToRequest(Request &rctx, Value &&result) {
	Value origin;
	origin.setInteger(_resource->getSourceDelta().toMicroseconds(), "delta");

	if (auto &t = _resource->getQueries().getContinueToken()) {
		if (t.isInit()) {
			Value cursor({
				pair("start", Value(t.getStart())),
				pair("end", Value(t.getEnd())),
				pair("total", Value(t.getTotal())),
				pair("count", Value(t.getCount())),
				pair("field", Value(t.getField())),
			});

			if (t.hasNext()) {
				cursor.setString(t.encodeNext(), "next");
			}

			if (t.hasPrev()) {
				cursor.setString(t.encodePrev(), "prev");
			}

			origin.setValue(move(cursor), "cursor");
		}
	}
	return output::writeResourceData(rctx, move(result), move(origin));
}

Status ResourceHandler::writeInfoToReqest(Request &rctx) {
	Value result(_resource->getResultObject());
	if (_resource->getType() == ResourceType::File) {
		return output::writeResourceFileHeader(rctx, result);
	}

	return OK;
}

Status ResourceHandler::writeToRequest(Request &rctx) {
	if (_resource->getType() == ResourceType::File) {
		Value result(_resource->getResultObject());
		if (result) {
			return output::writeResourceFileData(rctx, move(result));
		}
	} else {
		Value result(_resource->getResultObject());
		if (result) {
			return writeDataToRequest(rctx, move(result));
		}
	}

	return getHintedStatus(HTTP_NOT_FOUND);
}

Status ResourceHandler::getHintedStatus(Status s) const {
	auto status = _resource->getStatus();
	if (status != HTTP_OK) {
		return status;
	}
	return s;
}

ResourceMultiHandler::ResourceMultiHandler(const Map<StringView, const Scheme *> &schemes)
: _schemes(schemes) { }

bool ResourceMultiHandler::isRequestPermitted(Request &rctx) {
	_transaction = db::Transaction::acquire(db::Adapter(rctx.getController()->acquireDatabase()));
	return true;
}

Status ResourceMultiHandler::onTranslateName(Request &rctx) {
	if (!isRequestPermitted(rctx)) {
		return HTTP_FORBIDDEN;
	}

	auto &data = rctx.getInfo().queryData;
	auto user = rctx.getAuthorizedUser();
	if (!user && data.isString("token")) {
		user = rctx.getUser();
	}

	int64_t targetDelta = 0;
	Time deltaMax;
	Value result;
	Value delta;
	Vector<Pair<String, Resource *>> resources;
	resources.reserve(data.size());

	if (data.isInteger("delta")) {
		targetDelta = data.getInteger("delta");
	}

	for (auto &it : data.asDict()) {
		StringView path(it.first);
		auto scheme = path.readUntil<StringView::Chars<'/'>>();
		if (path.is('/')) {
			++ path;
		}
		auto s_it = _schemes.find(scheme.str<Interface>());
		if (s_it != _schemes.end()) {
			if (auto resource = Resource::resolve(_transaction, *s_it->second, path)) {
				resource->setUser(user);
				resource->applyQuery(it.second);
				if (targetDelta > 0 && resource->isDeltaApplicable() && !resource->getQueryDelta()) {
					resource->setQueryDelta(Time::microseconds(targetDelta));
				}
				resource->prepare();

				if (resource->hasDelta()) {
					deltaMax = max(deltaMax, resource->getSourceDelta());
					delta.setInteger(resource->getSourceDelta().toMicroseconds(), it.first);
				}

				resources.emplace_back(it.first, resource);
			}
		}
	}

	if (deltaMax && delta.size() == resources.size()) {
		rctx.setResponseHeader("Last-Modified", deltaMax.toHttp<Interface>());
		if (targetDelta > 0) {
			auto modified = rctx.getRequestHeader("if-modified-since");
			if (Time::fromHttp(modified).toSeconds() >= deltaMax.toSeconds()) {
				return HTTP_NOT_MODIFIED;
			}
		}
	}

	for (auto &it : resources) {
		if (!rctx.getInfo().headerRequest) {
			result.setValue(it.second->getResultObject(), it.first);
		}
	}

	if (!result.empty()) {
		resultData.setValue(move(delta), "delta");
		return writeDataToRequest(rctx, move(result));
	} else {
		return HTTP_NOT_FOUND;
	}

	return HTTP_NOT_IMPLEMENTED;
}

Status ResourceMultiHandler::writeDataToRequest(Request &rctx, Value &&result) {
	return output::writeResourceData(rctx, move(result), move(resultData));
}

}
