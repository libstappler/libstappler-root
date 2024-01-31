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

#include "SPWebResourceResolver.h"
#include "SPWebResource.h"
#include "SPWebRoot.h"
#include "SPValid.h"

namespace STAPPLER_VERSIONIZED stappler::web {

static Vector<StringView> parsePath(StringView path) {
	Vector<StringView> pathVec;
	if (!path.empty() && path.front() == ':') {
		path.split<StringView::Chars<':'>>([&] (const StringView &v) {
			pathVec.emplace_back(v);
		});
	} else {
		path.split<StringView::Chars<'/'>>([&] (const StringView &v) {
			pathVec.emplace_back(v);
		});
	}

	while (!pathVec.empty() && pathVec.back().empty()) {
		pathVec.pop_back();
	}

	if (!pathVec.empty()) {
		std::reverse(pathVec.begin(), pathVec.end());
		while (pathVec.back().empty()) {
			pathVec.pop_back();
		}
	}

	return pathVec;
}

static bool getSelectResource(ResourceResolver *resv, Vector<StringView> &path, bool &isSingleObject) {
	if (path.size() < 2) {
		Root::getCurrent()->error("ResourceResolver", "invalid 'select' query");
		return false;
	}

	auto field = resv->getScheme()->getField(path.back());
	if (!field || !field->isIndexed()) {
		Root::getCurrent()->error("ResourceResolver", "invalid 'select' query");
		return false;
	}
	path.pop_back();

	StringView cmpStr(path.back()); path.pop_back();

	auto decComp = stappler::sql::decodeComparation(cmpStr);
	db::Comparation cmp = decComp.first;
	size_t valuesRequired = (decComp.second ? 2 : 1);

	if (cmp == db::Comparation::Invalid) {
		cmp = db::Comparation::Equal;
		if (field->hasFlag(db::Flags::Unique) || field->getTransform() == db::Transform::Alias) {
			isSingleObject = true;
		}
		if (field->getType() == db::Type::Text) {
			return resv->selectByQuery(db::Query::Select(field->getName(), cmp, cmpStr));
		} else if (field->getType() == db::Type::Boolean) {
			if (valid::validateNumber(cmpStr)) {
				return resv->selectByQuery(db::Query::Select(field->getName(), cmp, cmpStr.readInteger().get(), 0));
			} else if (cmpStr == "t" || cmpStr == "true") {
				return resv->selectByQuery(db::Query::Select(field->getName(), cmp, Value(true), Value(false)));
			} else if (cmpStr == "f" || cmpStr == "false") {
				return resv->selectByQuery(db::Query::Select(field->getName(), cmp, Value(false), Value(false)));
			}
		} else if (valid::validateNumber(cmpStr) && db::checkIfComparationIsValid(field->getType(), cmp, field->getFlags())) {
			return resv->selectByQuery(db::Query::Select(field->getName(), cmp, cmpStr.readInteger().get(), 0));
		} else {
			Root::getCurrent()->error("ResourceResolver", "invalid 'select' query");
			return false;
		}
	}

	if (path.size() < valuesRequired || !db::checkIfComparationIsValid(field->getType(), cmp, field->getFlags())) {
		Root::getCurrent()->error("ResourceResolver", "invalid 'select' query");
		return false;
	}

	if (valuesRequired == 1) {
		StringView value(std::move(path.back())); path.pop_back();
		if (field->getType() == db::Type::Text) {
			return resv->selectByQuery(db::Query::Select(field->getName(), cmp, value));
		} else if (valid::validateNumber(value)) {
			return resv->selectByQuery(db::Query::Select(field->getName(), cmp, value.readInteger().get(), 0));
		} else {
			Root::getCurrent()->error("ResourceResolver", "invalid 'select' query");
			return false;
		}
	}

	if (valuesRequired == 2) {
		StringView value1(std::move(path.back())); path.pop_back();
		StringView value2(std::move(path.back())); path.pop_back();
		if (valid::validateNumber(value1) && valid::validateNumber(value2)) {
			return resv->selectByQuery(db::Query::Select(field->getName(), cmp, value1.readInteger().get(), value2.readInteger().get()));
		}
	}

	return false;
}

static bool getSearchResource(ResourceResolver *resv, Vector<StringView> &path, bool &isSingleObject) {
	if (path.size() < 1) {
		Root::getCurrent()->error("ResourceResolver", "invalid 'search' query");
		return false;
	}

	auto field = resv->getScheme()->getField(path.back());
	if (!field || field->getType() != db::Type::FullTextView) {
		Root::getCurrent()->error("ResourceResolver", "invalid 'search' query");
		return false;
	}
	path.pop_back();

	return resv->searchByField(field);
}

static bool getOrderResource(ResourceResolver *resv, Vector<StringView> &path) {
	if (path.size() < 1) {
		Root::getCurrent()->error("ResourceResolver", "invalid 'order' query");
		return false;
	}

	auto field = resv->getScheme()->getField(path.back());
	if (!field || !field->isIndexed()) {
		Root::getCurrent()->error("ResourceResolver", "invalid 'order' query");
		return false;
	}
	path.pop_back();

	db::Ordering ord = db::Ordering::Ascending;
	if (!path.empty()) {
		if (path.back() == "asc" ) {
			ord = db::Ordering::Ascending;
			path.pop_back();
		} else if (path.back() == "desc") {
			ord = db::Ordering::Descending;
			path.pop_back();
		}
	}

	if (!path.empty()) {
		auto &n = path.back();
		if (valid::validateNumber(n)) {
			if (resv->order(field->getName(), ord)) {
				auto ret = resv->limit(n.readInteger().get());
				path.pop_back();
				return ret;
			} else {
				return false;
			}
		}
	}

	return resv->order(field->getName(), ord);
}

static bool getOrderResource(ResourceResolver *resv, Vector<StringView> &path, const StringView &fieldName, db::Ordering ord) {
	auto field = resv->getScheme()->getField(fieldName);
	if (!field || !field->isIndexed()) {
		Root::getCurrent()->error("ResourceResolver", "invalid 'order' query");
		return false;
	}

	if (!path.empty()) {
		auto &n = path.back();
		if (valid::validateNumber(n)) {
			if (resv->order(field->getName(), ord)) {
				auto ret = resv->limit(n.readInteger().get());
				path.pop_back();
				return ret;
			} else {
				return false;
			}
		}
	}

	return resv->order(field->getName(), ord);
}

static bool getLimitResource(ResourceResolver *resv, Vector<StringView> &path, bool &isSingleObject) {
	if (path.size() < 1) {
		Root::getCurrent()->error("ResourceResolver", "invalid 'limit' query");
		return false;
	}

	StringView value(path.back()); path.pop_back();
	if (valid::validateNumber(value)) {
		auto val = value.readInteger().get();
		if (val == 1) {
			isSingleObject = true;
		}
		return resv->limit(val);
	} else {
		Root::getCurrent()->error("ResourceResolver", "invalid 'limit' query");
		return false;
	}
}

static bool getOffsetResource(ResourceResolver *resv, Vector<StringView> &path) {
	if (path.size() < 1) {
		Root::getCurrent()->error("ResourceResolver", "invalid 'offset' query");
		return false;
	}

	StringView value(std::move(path.back())); path.pop_back();
	if (valid::validateNumber(value)) {
		return resv->offset(value.readInteger().get());
	} else {
		Root::getCurrent()->error("ResourceResolver", "invalid 'offset' query");
		return false;
	}
}

static bool getFirstResource(ResourceResolver *resv, Vector<StringView> &path, bool &isSingleObject) {
	if (path.size() < 1) {
		Root::getCurrent()->error("ResourceResolver", "invalid 'first' query");
		return false;
	}

	auto field = resv->getScheme()->getField(path.back());
	if (!field || !field->isIndexed()) {
		Root::getCurrent()->error("ResourceResolver", "invalid 'first' query");
		return false;
	}
	path.pop_back();

	if (!path.empty()) {
		if (valid::validateNumber(path.back())) {
			size_t val = path.back().readInteger().get();
			path.pop_back();

			if (val == 1) {
				isSingleObject = true;
			}
			return resv->first(field->getName(), val);
		}
	}

	isSingleObject = true;
	return resv->first(field->getName(), 1);
}

static bool getLastResource(ResourceResolver *resv, Vector<StringView> &path, bool &isSingleObject) {
	if (path.size() < 1) {
		Root::getCurrent()->error("ResourceResolver", "invalid 'last' query");
		return false;
	}

	auto field = resv->getScheme()->getField(path.back());
	if (!field || !field->isIndexed()) {
		Root::getCurrent()->error("ResourceResolver", "invalid 'last' query");
		return false;
	}
	path.pop_back();

	if (!path.empty()) {
		if (valid::validateNumber(path.back())) {
			size_t val = path.back().readInteger().get();
			path.pop_back();

			if (val == 1) {
				isSingleObject = true;
			}
			return resv->last(field->getName(), val);
		}
	}

	isSingleObject = true;
	return resv->last(field->getName(), 1);
}

static Resource *parseResource(ResourceResolver *resv, Vector<StringView> &path) {
	bool isSingleObject = false;
	while (!path.empty()) {
		StringView filter(std::move(path.back()));
		path.pop_back();

		if (!isSingleObject) {
			if (filter.starts_with("id")) {
				filter += "id"_len;
				if (uint64_t id = filter.readInteger().get()) {
					if (!resv->selectById(id)) {
						return nullptr;
					}
					isSingleObject = true;
				} else {
					return nullptr;
				}
			} else if (filter.starts_with("named-")) {
				filter += "named-"_len;
				if (valid::validateIdentifier(filter)) {
					if (!resv->selectByAlias(filter)) {
						return nullptr;
					}
					isSingleObject = true;
				} else {
					return nullptr;
				}
			} else if (filter == "all") {
				if (!resv->getAll()) {
					return nullptr;
				}
				// do nothing
			} else if (filter == "select") {
				if (!getSelectResource(resv, path, isSingleObject)) {
					return nullptr;
				}
			} else if (filter == "search") {
				if (!getSearchResource(resv, path, isSingleObject)) {
					return nullptr;
				}
			} else if (filter == "order") {
				if (!getOrderResource(resv, path)) {
					return nullptr;
				}
			} else if (filter.size() > 2 && filter.front() == '+') {
				if (!getOrderResource(resv, path, filter.sub(1), db::Ordering::Ascending)) {
					return nullptr;
				}
			} else if (filter.size() > 2 && filter.front() == '-') {
				if (!getOrderResource(resv, path, filter.sub(1), db::Ordering::Descending)) {
					return nullptr;
				}
			} else if (filter == "limit") {
				if (!getLimitResource(resv, path, isSingleObject)) {
					return nullptr;
				}
			} else if (filter == "offset") {
				if (!getOffsetResource(resv, path)) {
					return nullptr;
				}
			} else if (filter == "first") {
				if (!getFirstResource(resv, path, isSingleObject)) {
					return nullptr;
				}
			} else if (filter == "last") {
				if (!getLastResource(resv, path, isSingleObject)) {
					return nullptr;
				}
			} else if (filter.size() > 2 && filter.front() == '+') {
				if (!getOrderResource(resv, path, filter.sub(1), db::Ordering::Ascending)) {
					return nullptr;
				}
			} else if (filter.size() > 2 && filter.front() == '-') {
				if (!getOrderResource(resv, path, filter.sub(1), db::Ordering::Descending)) {
					return nullptr;
				}
			} else {
				Root::getCurrent()->error("ResourceResolver", "Invalid query", Value(filter));
				return nullptr;
			}
		} else {
			if (filter == "offset" && !path.empty() && valid::validateNumber(path.back())) {
				if (resv->offset(path.back().readInteger().get())) {
					path.pop_back();
					continue;
				}
			}
			auto f = resv->getScheme()->getField(filter);
			if (!f) {
				Root::getCurrent()->error("ResourceResolver", "No such field", Value(filter));
				return nullptr;
			}

			auto type = f->getType();
			if (type == db::Type::File || type == db::Type::Image || type == db::Type::Array) {
				isSingleObject = true;
				if (!resv->getField(filter, f)) {
					return nullptr;
				} else {
					return resv->getResult();
				}
			} else if (type == db::Type::Object) {
				isSingleObject = true;
				if (!resv->getObject(f)) {
					return nullptr;
				}
			} else if (type == db::Type::Set) {
				isSingleObject = false;
				if (!resv->getSet(f)) {
					return nullptr;
				}
			} else if (type == db::Type::View) {
				isSingleObject = false;
				if (!resv->getView(f)) {
					return nullptr;
				}
			} else {
				Root::getCurrent()->error("ResourceResolver", "Invalid query", Value(filter));
				return nullptr;
			}
		}
	}

	return resv->getResult();
}

static Resource *getResolvedResource(ResourceResolver *resv, Vector<StringView> &path) {
	if (path.empty()) {
		return resv->getResult();
	}
	return parseResource(resv, path);
}

Resource *Resource::resolve(const db::Transaction &a, const db::Scheme &scheme, const StringView &path) {
	Value tmp;
	return resolve(a, scheme, path, tmp);
}

Resource *Resource::resolve(const Transaction &a, const Scheme &scheme, const StringView &path, Value & sub) {
	auto pathVec = parsePath(path);

	ResourceResolver resolver(a, scheme);
	if (sub.isDictionary() && !sub.empty()) {
		for (auto &it : sub.asDict()) {
			if (auto f = resolver.getScheme()->getField(it.first)) {
				if (f->isIndexed()) {
					switch (f->getType()) {
					case db::Type::Integer:
					case db::Type::Boolean:
					case db::Type::Object:
						resolver.selectByQuery(
								db::Query::Select(it.first, db::Comparation::Equal, it.second.getInteger(), 0));
						break;
					case db::Type::Text:
						resolver.selectByQuery(
								db::Query::Select(it.first, db::Comparation::Equal, it.second.getString()));
						break;
					default:
						break;
					}
				}
			}
		}
	} else if (sub.isInteger()) {
		auto id = sub.getInteger();
		if (id) {
			resolver.selectById(id);
		}
	} else if (sub.isString()) {
		auto & str = sub.getString();
		if (!str.empty()) {
			resolver.selectByAlias(str);
		}
	}

	return getResolvedResource(&resolver, pathVec);
}

Resource *Resource::resolve(const db::Transaction &a, const db::Scheme &scheme, Vector<StringView> &pathVec) {
	ResourceResolver resolver(a, scheme);
	return getResolvedResource(&resolver, pathVec);
}

ResourceResolver::ResourceResolver(const db::Transaction &a, const db::Scheme &scheme)
: _storage(a), _scheme(&scheme), _queries(a.getAdapter().getApplicationInterface(), &scheme) {
	_type = Objects;
}

bool ResourceResolver::selectById(uint64_t oid) {
	if (_type == Objects) {
		if (_queries.selectById(_scheme, oid)) {
			return true;
		}
	}
	Root::getCurrent()->error("ResourceResolver", "Invalid 'select by id', invalid resource type");
	return false;
}

bool ResourceResolver::selectByAlias(const StringView &str) {
	if (_type == Objects) {
		if (_queries.selectByName(_scheme, str)) {
			return true;
		}
	}
	Root::getCurrent()->error("ResourceResolver", "Invalid 'select by alias', invalid resource type");
	return false;
}

bool ResourceResolver::selectByQuery(db::Query::Select &&q) {
	if (_type == Objects) {
		if (_queries.selectByQuery(_scheme, move(q))) {
			return true;
		}
	}
	Root::getCurrent()->error("ResourceResolver", "Invalid 'select by query', invalid resource type");
	return false;
}

bool ResourceResolver::searchByField(const db::Field *field) {
	if (_type == Objects) {
		_resource = makeResource(ResourceType::Search, move(_queries), field);
		_type = Search;
		return true;
	}
	Root::getCurrent()->error("ResourceResolver", "Invalid 'search', invalid resource type");
	return false;
}

bool ResourceResolver::order(const StringView &f, db::Ordering o) {
	if (_type == Objects) {
		if (_queries.order(_scheme, f, o)) {
			return true;
		}
	}
	Root::getCurrent()->error("ResourceResolver", "Invalid 'order', invalid resource type");
	return false;
}

bool ResourceResolver::first(const StringView &f, size_t v) {
	if (_type == Objects) {
		if (_queries.first(_scheme, f, v)) {
			return true;
		}
	}
	Root::getCurrent()->error("ResourceResolver", "Invalid 'first', invalid resource type");
	return false;
}

bool ResourceResolver::last(const StringView &f, size_t v) {
	if (_type == Objects) {
		if (_queries.last(_scheme, f, v)) {
			return true;
		}
	}
	Root::getCurrent()->error("ResourceResolver", "Invalid 'last', invalid resource type");
	return false;
}

bool ResourceResolver::limit(size_t limit) {
	if (_type == Objects) {
		if (_queries.limit(_scheme, limit)) {
			return true;
		}
	}
	Root::getCurrent()->error("ResourceResolver", "Invalid 'limit', invalid resource type");
	return false;
}

bool ResourceResolver::offset(size_t offset) {
	if (_type == Objects) {
		if (_queries.offset(_scheme, offset)) {
			return true;
		}
	}
	Root::getCurrent()->error("ResourceResolver", "Invalid 'offset', invalid resource type");
	return false;
}

bool ResourceResolver::getObject(const db::Field *f) {
	if (_type == Objects) {
		if (auto fo = static_cast<const db::FieldObject *>(f->getSlot())) {
			if (_queries.setField(fo->scheme, f)) {
				_scheme = fo->scheme;
				return true;
			}
		}
	}
	Root::getCurrent()->error("ResourceResolver", "Invalid 'getObject', invalid resource type");
	return false;
}

bool ResourceResolver::getSet(const db::Field *f) {
	if (_type == Objects) {
		if (auto fo = static_cast<const db::FieldObject *>(f->getSlot())) {
			if (_queries.setField(fo->scheme, f)) {
				_scheme = fo->scheme;
				return true;
			}
		}
	}
	Root::getCurrent()->error("ResourceResolver", "Invalid 'getSet', invalid resource type");
	return false;
}

bool ResourceResolver::getView(const db::Field *f) {
	if (_type == Objects) {
		if (auto fo = static_cast<const db::FieldView *>(f->getSlot())) {
			if (_queries.setField(fo->scheme, f)) {
				_scheme = fo->scheme;
				return true;
			}
		}
	}
	Root::getCurrent()->error("ResourceResolver", "Invalid 'getView', invalid resource type");
	return false;
}

bool ResourceResolver::getField(const StringView &str, const db::Field *f) {
	if ((_type == Objects) && f->getType() == db::Type::Array) {
		_resource = makeResource(ResourceType::Array, move(_queries), f);
		_type = Array;
		return true;
	}

	if (_type == Objects && (f->getType() == db::Type::File || f->getType() == db::Type::Image)) {
		_resource = makeResource(ResourceType::File, move(_queries), f);
		_type = File;
		return true;
	}
	Root::getCurrent()->error("ResourceResolver", "Invalid 'getField', invalid resource type");
	return false;
}

bool ResourceResolver::getAll() {
	if (_type == Objects) {
		if (_queries.setAll()) {
			return true;
		}
	}
	Root::getCurrent()->error("ResourceResolver", "Invalid 'getAll', invalid resource type or already enabled");
	return false;
}

Resource *ResourceResolver::getResult() {
	if (_type == Objects) {
		if (_queries.empty()) {
			return makeResource(ResourceType::ResourceList, move(_queries), nullptr);
		} else if (_queries.isView()) {
			return makeResource(ResourceType::View, move(_queries), nullptr);
		} else if (_queries.isRefSet()) {
			if (auto f = _queries.getField()) {
				if (f->getType() == db::Type::Object) {
					return makeResource(ResourceType::ObjectField, move(_queries), nullptr);
				}
			}
			return makeResource(ResourceType::ReferenceSet, move(_queries), nullptr);
		} else if (_queries.isObject()) {
			return makeResource(ResourceType::Object, move(_queries), nullptr);
		} else {
			return makeResource(ResourceType::Set, move(_queries), nullptr);
		}
	}
	return _resource;
}

const db::Scheme *ResourceResolver::getScheme() const {
	return _scheme;
}

Resource *ResourceResolver::makeResource(ResourceType type, db::QueryList &&list, const db::Field *f) {
	switch (type) {
	case ResourceType::ResourceList: return new ResourceReslist(_storage, std::move(list));  break;
	case ResourceType::ReferenceSet: return new ResourceRefSet(_storage, std::move(list)); break;
	case ResourceType::ObjectField: return new ResourceFieldObject(_storage, std::move(list)); break;
	case ResourceType::Object: return new ResourceObject(_storage, std::move(list)); break;
	case ResourceType::Set: return new ResourceSet(_storage, std::move(list)); break;
	case ResourceType::View: return new ResourceView(_storage, std::move(list)); break;
	case ResourceType::File: return new ResourceFile(_storage, std::move(list), f); break;
	case ResourceType::Array: return new ResourceArray(_storage, std::move(list), f); break;
	case ResourceType::Search: return new ResourceSearch(_storage, std::move(list), f); break;
	}
	return nullptr;
}

}
