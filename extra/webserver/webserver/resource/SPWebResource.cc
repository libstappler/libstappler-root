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

#include "SPWebResource.h"

namespace STAPPLER_VERSIONIZED stappler::web {

Resource::Resource(ResourceType t, const Transaction &a, QueryList &&list)
: _type(t), _transaction(a), _queries(move(list)) {
	if (_queries.isDeltaApplicable()) {
		_delta = Time::microseconds(_transaction.getDeltaValue(*_queries.getScheme()));
	}
}

Resource::~Resource() { }

ResourceType Resource::getType() const {
	return _type;
}

const db::Scheme &Resource::getScheme() const { return *_queries.getScheme(); }
Status Resource::getStatus() const { return _status; }

bool Resource::isDeltaApplicable() const {
	return _queries.isDeltaApplicable();
}

bool Resource::hasDelta() const {
	if (_queries.isDeltaApplicable()) {
		if (_queries.isView()) {
			return static_cast<const db::FieldView *>(_queries.getItems().front().field->getSlot())->delta;
		} else {
			return _queries.getScheme()->hasDelta();
		}
	}
	return false;
}

void Resource::setQueryDelta(Time d) {
	_queries.setDelta(d);
}

Time Resource::getQueryDelta() const {
	return _queries.getDelta();
}

Time Resource::getSourceDelta() const { return _delta; }

void Resource::resolveOptionForString(StringView str) {
	if (str.empty()) {
		return;
	}

	str.split<StringView::Chars<','>>([&] (const StringView &v) {
		StringView r(v);
		r.trimChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
		String token(toString('$', r));
		_resolve |= Query::decodeResolve(token);
		emplace_ordered(_extraResolves, StringView(token).pdup());
	});
}

void Resource::addExtraResolveField(StringView str) {
	if (emplace_ordered(_extraResolves, str.pdup())) {
		_isResolvesUpdated = true;
	}
}

void Resource::setUser(User *u) {
	_user = u;
}

void Resource::setFilterData(const Value &val) {
	_filterData = val;
}

void Resource::setResolveOptions(const Value & opts) {
	if (opts.isArray()) {
		for (auto &it : opts.asArray()) {
			if (it.isString()) {
				resolveOptionForString(it.getString());
			}
		}
	} else if (opts.isString()) {
		resolveOptionForString(opts.getString());
	}
}
void Resource::setResolveDepth(size_t size) {
	_queries.setResolveDepth(std::min(uint16_t(size), db::config::RESOURCE_RESOLVE_MAX_DEPTH));
}

void Resource::setPageFrom(size_t value) {
	if (value > 0) {
		_queries.offset(_queries.getScheme(), value);
	}
}
void Resource::setPageCount(size_t value) {
	if (value > 0 && value != maxOf<size_t>()) {
		_queries.limit(_queries.getScheme(), value);
	}
}

void Resource::applyQuery(const Value &query) {
	_queries.apply(query);
}

void Resource::prepare(QueryList::Flags flags) {
	if (_isResolvesUpdated) {
		_queries.resolve(_extraResolves);
		_isResolvesUpdated = false;
	}
	_queries.addFlag(flags);
}

const db::QueryList &Resource::getQueries() const {
	return _queries;
}

bool Resource::prepareUpdate() { return false; }
bool Resource::prepareCreate() { return false; }
bool Resource::prepareAppend() { return false; }
bool Resource::removeObject() { return false; }
Value Resource::updateObject(Value &, Vector<db::InputFile> &) { return Value(); }
Value Resource::createObject(Value &, Vector<db::InputFile> &) { return Value(); }
Value Resource::appendObject(Value &) { return Value(); }
Value Resource::getResultObject() { return Value(); }
void Resource::resolve(const Scheme &, Value &) { }

size_t Resource::getMaxRequestSize() const {
	return getRequestScheme().getMaxRequestSize();
}
size_t Resource::getMaxVarSize() const {
	return getRequestScheme().getMaxVarSize();
}
size_t Resource::getMaxFileSize() const {
	return getRequestScheme().getMaxFileSize();
}

void Resource::encodeFiles(Value &data, Vector<db::InputFile> &files) {
	for (auto &it : files) {
		const db::Field * f = getRequestScheme().getField(it.name);
		if (f && f->isFile()) {
			if (!data.hasValue(it.name)) {
				data.setInteger(it.negativeId(), it.name);
			}
		} else if (f && f->getType() == db::Type::Extra) {
			auto fobj = f->getSlot<db::FieldExtra>();
			auto cIt = fobj->fields.find("content");
			auto tIt = fobj->fields.find("type");
			auto mIt = fobj->fields.find("mtime");
			if (cIt != fobj->fields.end() && tIt != fobj->fields.end() && tIt->second.getType() == db::Type::Text) {
				if (cIt->second.getType() == db::Type::Text && !it.isBinary) {
					auto patch = Value({
						pair("content", Value(it.readText())),
						pair("type", Value(it.type)),
					});

					if (mIt != fobj->fields.end()) {
						patch.setInteger(Time::now().toMicros(), "mtime");
					}
					data.setValue(move(patch), f->getName().str<Interface>());
				} else if (cIt->second.getType() == db::Type::Bytes) {
					auto patch = Value({
						pair("content", Value(it.readBytes())),
						pair("type", Value(it.type)),
					});

					if (mIt != fobj->fields.end()) {
						patch.setInteger(Time::now().toMicros(), "mtime");
					}
					data.setValue(move(patch), f->getName().str<Interface>());
				}
			}
		}
	}
}

static bool Resource_isIdRequest(const db::QueryFieldResolver &next, ResolveOptions opts, ResolveOptions target) {
	if (next.getResolves().empty()) {
		if (auto vec = next.getIncludeVec()) {
			if (vec->size() == 1 && vec->front().name == "$id") {
				return true;
			}
		}
	}

	if ((opts & ResolveOptions::Ids) != ResolveOptions::None) {
		if ((opts & target) == ResolveOptions::None) {
			return true;
		}
	}
	return false;
}

void Resource::resolveSet(const QueryFieldResolver &res, int64_t id, const db::Field &field, Value &fobj) {
	QueryFieldResolver next(res.next(field.getName()));
	if (next) {
		auto &fields = next.getResolves();
		bool idOnly = Resource_isIdRequest(next, ResolveOptions::None, ResolveOptions::Sets);

		auto objs = idOnly
				? Worker(*res.getScheme(), _transaction).getField(fobj, field, Set<const Field *>{(const Field *)nullptr})
				: Worker(*res.getScheme(), _transaction).getField(fobj, field, fields);
		if (objs.isArray()) {
			Value arr;
			for (auto &sit : objs.asArray()) {
				if (sit.isDictionary()) {
					auto id = sit.getInteger("__oid");
					if (idOnly) {
						arr.addInteger(id);
					} else {
						if (_resolveObjects.insert(id).second == false) {
							sit.setInteger(id);
						}
						arr.addValue(std::move(sit));
					}
				}
			}
			fobj = std::move(arr);
			return;
		}
	}
	fobj = Value();
}

void Resource::resolveObject(const QueryFieldResolver &res, int64_t id, const db::Field &field, Value &fobj) {
	QueryFieldResolver next(res.next(field.getName()));
	if (next && _resolveObjects.find(fobj.asInteger()) == _resolveObjects.end()) {
		auto &fields = next.getResolves();
		if (!Resource_isIdRequest(next, _resolve, ResolveOptions::Objects)) {
			Value obj = Worker(*res.getScheme(), _transaction).getField(fobj, field, fields);
			if (obj.isDictionary()) {
				auto id = obj.getInteger("__oid");
				if (_resolveObjects.insert(id).second == false) {
					fobj.setInteger(id);
				} else {
					fobj.setValue(move(obj));
				}
				return;
			}
		} else {
			return;
		}
		fobj.setNull();
	}
}

void Resource::resolveArray(const QueryFieldResolver &res, int64_t id, const db::Field &field, Value &fobj) {
	fobj.setValue(Worker(*res.getScheme(), _transaction).getField(fobj, field));
}

void Resource::resolveFile(const QueryFieldResolver &res, int64_t id, const db::Field &field, Value &fobj) {
	QueryFieldResolver next(res.next(field.getName()));
	if (next) {
		auto fields = next.getResolves();
		if (!Resource_isIdRequest(next, _resolve, ResolveOptions::Files)) {
			Value obj = Worker(*res.getScheme(), _transaction).getField(fobj, field, fields);
			if (obj.isDictionary()) {
				fobj.setValue(move(obj));
				return;
			}
		} else {
			return;
		}
	}
	fobj.setNull();
}

static void Resource_resolveExtra(const db::QueryFieldResolver &res, Value &obj) {
	auto &fields = res.getResolves();
	auto &fieldData = res.getResolvesData();
	auto &dict = obj.asDict();
	auto it = dict.begin();
	while (it != dict.end()) {
		auto f = res.getField(it->first);
		if (fieldData.find(it->first) != fieldData.end()) {
			db::QueryFieldResolver next(res.next(it->first));
			if (next) {
				Resource_resolveExtra(next, it->second);
			}
			it ++;
		} else if (!f || f->isProtected() || (fields.find(f) == fields.end())) {
			it = dict.erase(it);
		} else if (f->getType() == db::Type::Extra && it->second.isDictionary()) {
			db::QueryFieldResolver next(res.next(it->first));
			if (next) {
				Resource_resolveExtra(next, it->second);
			}
			it ++;
		} else {
			it ++;
		}
	}
}

int64_t Resource::processResolveResult(const QueryFieldResolver &res, const Set<const Field *> &fields, Value &obj) {
	int64_t id = 0;
	auto &dict = obj.asDict();
	auto it = dict.begin();

	while (it != dict.end()) {
		if (it->first == "__oid") {
			id = it->second.asInteger();
			++ it;
			continue;
		} else if (it->first == "__delta") {
			if (res.getMeta() == QueryFieldResolver::Meta::None) {
				if (it->second.getString("action") == "delete") {
					it->second.setString("delete");
				} else {
					it = dict.erase(it);
					continue;
				}
			} else {
				auto meta = res.getMeta();
				if ((meta & QueryFieldResolver::Meta::Action) == QueryFieldResolver::Meta::None) {
					it->second.erase("action");
				} else if ((meta & QueryFieldResolver::Meta::Time) == QueryFieldResolver::Meta::None) {
					it->second.erase("time");
				}
			}
			++ it;
			continue;
		} else if (it->first == "__views") {
			auto meta = res.getMeta();
			if ((meta & QueryFieldResolver::Meta::View) == QueryFieldResolver::Meta::None) {
				it = dict.erase(it);
				continue;
			} else {
				++ it;
				continue;
			}
		} else if (it->first == "__ts_rank") {
			++ it;
			continue;
		}

		auto f = res.getField(it->first);
		if (!f || f->isProtected() || (fields.find(f) == fields.end())) {
			it = dict.erase(it);
		} else if ((f->getType() == db::Type::Extra || f->getType() == db::Type::Data || f->getType() == db::Type::Virtual) && it->second.isDictionary()) {
			QueryFieldResolver next(res.next(it->first));
			if (next) {
				Resource_resolveExtra(next, it->second);
			}
			it ++;
		} else {
			it ++;
		}
	}
	return id;
}

void Resource::resolveResult(const QueryFieldResolver &res, Value &obj, uint16_t depth, uint16_t max) {
	auto &searchField = res.getResolves();

	int64_t id = processResolveResult(res, searchField, obj);

	if (res && depth <= max) {
		auto & fields = *res.getFields();
		for (auto &it : fields) {
			const Field &f = it.second;
			auto type = f.getType();

			if (f.isSimpleLayout() || searchField.find(&f) == searchField.end()) {
				if (type == db::Type::Bytes && f.getTransform() == db::Transform::Uuid) {
					auto &fobj = obj.getValue(it.first);
					if (fobj.isBytes()) {
						fobj.setString(memory::uuid(fobj.getBytes()).str());
					}
				}
				continue;
			}

			if (!obj.hasValue(it.first) && (type == db::Type::Set || type == db::Type::Array || type == db::Type::View)) {
				obj.setInteger(id, it.first);
			}

			auto &fobj = obj.getValue(it.first);
			if (type == db::Type::Object && fobj.isInteger()) {
				resolveObject(res, id, f, fobj);
			} else if ((type == db::Type::Set || type == db::Type::View) && fobj.isInteger()) {
				resolveSet(res, id, f, fobj);
			} else if (type == db::Type::Array && fobj.isInteger()) {
				resolveArray(res, id, f, fobj);
			} else if ((type == db::Type::File || type == db::Type::Image) && fobj.isInteger()) {
				resolveFile(res, id, f, fobj);
			}
		}

		for (auto &it : fields) {
			auto &f = it.second;
			auto type = f.getType();

			if ((type == db::Type::Object && obj.isDictionary(it.first))
					|| ((type == db::Type::Set || type == db::Type::View) && obj.isArray(it.first))) {
				QueryFieldResolver next(res.next(it.first));
				if (next) {
					if (type == db::Type::Set || type == db::Type::View) {
						auto &fobj = obj.getValue(it.first);
						for (auto &sit : fobj.asArray()) {
							if (sit.isDictionary()) {
								resolveResult(next, sit, depth + 1, max);
							}
						}
					} else {
						auto &fobj = obj.getValue(it.first);
						if (fobj.isDictionary()) {
							resolveResult(next, fobj, depth + 1, max);
						}
					}
				}
			} else if (f.isFile() && obj.isDictionary()) {
				auto &dict = obj.asDict();
				auto f_it = dict.find(it.first);
				if (f_it != dict.end() && f_it->second.isNull()) {
					dict.erase(f_it);
				}
			}
		}
	} else if (obj.isDictionary()) {
		auto &dict = obj.asDict();
		auto it = dict.begin();
		while (it != dict.end()) {
			auto f = res.getField(it->first);
			if (f && f->isFile()) {
				it = dict.erase(it);
			} else {
				++ it;
			}
		}
	}
}

void Resource::resolveResult(const QueryList &l, Value &obj) {
	if (_isResolvesUpdated) {
		_queries.resolve(_extraResolves);
		_isResolvesUpdated = false;
	}
	resolveResult(l.getFields(), obj, 0, l.getResolveDepth());
}

const db::Scheme &Resource::getRequestScheme() const {
	return getScheme();
}

ResourceObject::ResourceObject(const Transaction &a, QueryList &&q)
: Resource(ResourceType::Object, a, move(q)) { }

bool ResourceObject::prepareUpdate() {
	return true;
}

bool ResourceObject::prepareCreate() {
	_status = HTTP_NOT_IMPLEMENTED;
	return false;
}

bool ResourceObject::prepareAppend() {
	_status = HTTP_NOT_IMPLEMENTED;
	return false;
}

bool ResourceObject::removeObject() {
	auto objs = getDatabaseId(_queries);
	if (objs.empty()) {
		return false;
	}

	bool ret = (objs.size() == 1);
	for (auto &it : objs) {
		_transaction.perform([&] {
			if (ret) {
				ret = Worker(getScheme(), _transaction).remove(it);
			} else {
				Worker(getScheme(), _transaction).remove(it);
			}
			return true;
		});
	}
	return (objs.size() == 1)?ret:true;
}

Value ResourceObject::performUpdate(const Vector<int64_t> &objs, Value &data, Vector<db::InputFile> &files) {
	Value ret;
	encodeFiles(data, files);

	for (auto &it : data.asDict()) {
		addExtraResolveField(it.first);
	}

	for (auto &it : objs) {
		_transaction.perform([&] {
			ret.addValue(Worker(getScheme(), _transaction).update(it, data));
			return true;
		});
	}

	return processResultList(_queries, ret);
}

Value ResourceObject::updateObject(Value &data, Vector<db::InputFile> &files) {
	Value ret;
	if (files.empty() && (!data.isDictionary() || data.empty())) {
		return Value();
	}

	// single-object or mass update
	auto objs = getDatabaseId(_queries);
	if (objs.empty()) {
		return Value();
	}

	return performUpdate(objs, data, files);
}

Value ResourceObject::getResultObject() {
	auto ret = getDatabaseObject();
	if (!ret.isArray()) {
		return Value();
	}

	return processResultList(_queries, ret);
}

int64_t ResourceObject::getObjectMtime() {
	auto tmpQuery = _queries;
	tmpQuery.clearFlags();
	tmpQuery.addFlag(QueryList::SimpleGet);
	auto str = tmpQuery.setQueryAsMtime();
	if (!str.empty()) {
		if (auto ret = _transaction.performQueryList(tmpQuery)) {
			if (ret.isArray()) {
				return ret.getValue(0).getInteger(str);
			} else {
				return ret.getInteger(str);
			}
		}
	}

	return 0;
}

Value ResourceObject::processResultList(const QueryList &s, Value &ret) {
	if (ret.isArray()) {
		auto &arr = ret.asArray();
		auto it = arr.begin();
		while (it != arr.end()) {
			if (it->isInteger()) {
				if (auto val = Worker(getScheme(), _transaction).get(it->getInteger())) {
					*it = move(val);
				}
			}

			if (!processResultObject(s, *it)) {
				it = arr.erase(it);
			} else {
				it ++;
			}
		}
		return std::move(ret);
	}
	return Value();
}

bool ResourceObject::processResultObject(const QueryList &s, Value &obj) {
	if (obj.isDictionary()) {
		resolveResult(s, obj);
		return true;
	}
	return false;
}

Value ResourceObject::getDatabaseObject() {
	return _transaction.performQueryList(_queries);
}

Vector<int64_t> ResourceObject::getDatabaseId(const QueryList &q, size_t count) {
	const Vector<QueryList::Item> &items = q.getItems();
	count = min(items.size(), count);

	return _transaction.performQueryListForIds(q, count);
}

ResourceReslist::ResourceReslist(const Transaction &a, QueryList &&q)
: ResourceObject(a, move(q)) {
	_type = ResourceType::ResourceList;
}

bool ResourceReslist::prepareCreate() {
	return true;
}

Value ResourceReslist::performCreateObject(Value &data, Vector<db::InputFile> &files, const Value &extra) {
	// single object
	if (data.isDictionary() || data.empty()) {
		if (extra.isDictionary()) {
			for (auto & it : extra.asDict()) {
				data.setValue(it.second, it.first);
			}
		}

		encodeFiles(data, files);

		for (auto &it : data.asDict()) {
			addExtraResolveField(it.first);
		}

		Value ret = Worker(getScheme(), _transaction).create(data);
		if (processResultObject(_queries, ret)) {
			return ret;
		}
	} else if (data.isArray()) {
		Value ret;
		for (auto &obj : data.asArray()) {
			Value n(Worker(getScheme(), _transaction).create(obj));
			if (n) {
				ret.addValue(std::move(n));
			}
		}
		return processResultList(_queries, ret);
	}

	return Value();
}

Value ResourceReslist::createObject(Value &data, Vector<db::InputFile> &file) {
	return performCreateObject(data, file, Value());
}

ResourceSet::ResourceSet(const Transaction &a, QueryList &&q)
: ResourceReslist(a, move(q)) {
	_type = ResourceType::Set;
}

bool ResourceSet::prepareAppend() {
	return true;
}

Value ResourceSet::createObject(Value &data, Vector<db::InputFile> &file) {
	// write object patch
	Value extra;
	auto &items = _queries.getItems();
	auto &item = items.back();
	if (items.size() > 1 && item.ref) {
		// has subqueries, try to calculate origin
		if (auto id = items.at(items.size() - 2).query.getSingleSelectId()) {
			extra.setInteger(id, item.ref->getName().str<Interface>());
		} else {
			auto ids = getDatabaseId(_queries, _queries.size() - 1);
			if (ids.size() == 1) {
				extra.setInteger(ids.front(), item.ref->getName().str<Interface>());
			}
		}
	}
	if (!item.query.getSelectList().empty()) {
		// has select query, try to extract extra data
		for (auto &it : item.query.getSelectList()) {
			if (it.compare == db::Comparation::Equal) {
				extra.setValue(it.value1, it.field);
			}
		}
	}
	return performCreateObject(data, file, extra);
}

Value ResourceSet::appendObject(Value &data) {
	// write object patch
	Value extra;
	auto &items = _queries.getItems();
	auto &item = items.back();
	if (items.size() > 1 && item.ref) {
		// has subqueries, try to calculate origin
		if (auto id = items.at(items.size() - 2).query.getSingleSelectId()) {
			extra.setInteger(id, item.ref->getName().str<Interface>());
		} else {
			auto ids = getDatabaseId(_queries);
			if (ids.size() == 1 && ids.front()) {
				extra.setInteger(ids.front(), item.ref->getName().str<Interface>());
			}
		}
	}

	if (extra.empty()) {
		return Value();
	}

	// collect object ids from input data
	Value val;
	if (data.isDictionary() && data.hasValue(item.ref->getName())) {
		val = std::move(data.getValue(item.ref->getName()));
	} else {
		val = std::move(data);
	}
	Vector<int64_t> ids;
	if (val.isArray()) {
		for (auto &it : val.asArray()) {
			auto i = it.asInteger();
			if (i) {
				ids.push_back(i);
			}
		}
	} else {
		auto i = val.asInteger();
		if (i) {
			ids.push_back(i);
		}
	}

	Vector<db::InputFile> files;
	return performUpdate(ids, extra, files);
}


ResourceRefSet::ResourceRefSet(const Transaction &a, QueryList &&q)
: ResourceSet(a, move(q)), _sourceScheme(_queries.getSourceScheme()), _field(_queries.getField()) {
	_type = ResourceType::ReferenceSet;
}

bool ResourceRefSet::prepareUpdate() {
	return true;
}
bool ResourceRefSet::prepareCreate() {
	return true;
}
bool ResourceRefSet::prepareAppend() {
	return true;
}
bool ResourceRefSet::removeObject() {
	auto id = getObjectId();
	if (id == 0) {
		return Value();
	}

	return _transaction.perform([&] () -> bool {
		Vector<int64_t> objs;
		if (!isEmptyRequest()) {
			objs = getDatabaseId(_queries);
			if (objs.empty()) {
				return false;
			}
		}
		return doCleanup(id, objs);
	});
}
Value ResourceRefSet::updateObject(Value &value, Vector<db::InputFile> &files) {
	if (value.isDictionary() && value.hasValue(_field->getName()) && (value.isBasicType(_field->getName()) || value.isArray(_field->getName()))) {
		value = value.getValue(_field->getName());
	}
	if (value.isBasicType() && !value.isNull()) {
		return doAppendObject(value, true);
	} else if (value.isArray()) {
		return doAppendObjects(value, true);
	} else {
		return ResourceSet::updateObject(value, files);
	}
}
Value ResourceRefSet::createObject(Value &value, Vector<db::InputFile> &files) {
	encodeFiles(value, files);
	return appendObject(value);
}
Value ResourceRefSet::appendObject(Value &value) {
	if (value.isBasicType()) {
		return doAppendObject(value, false);
	} else if (value.isArray()) {
		return doAppendObjects(value, false);
	} else if (value.isDictionary()) {
		return doAppendObject(value, false);
	}
	return Value();
}

int64_t ResourceRefSet::getObjectId() {
	if (!_objectId) {
		auto ids = _transaction.performQueryListForIds(_queries, _queries.getItems().size() - 1);
		if (!ids.empty()) {
			_objectId = ids.front();
		}
	}
	return _objectId;
}

Value ResourceRefSet::getObjectValue() {
	if (!_objectValue) {
		_objectValue = Worker(*_sourceScheme, _transaction).get(getObjectId(), db::UpdateFlags::None);
	}
	return _objectValue;
}

bool ResourceRefSet::isEmptyRequest() {
	if (_queries.getItems().back().query.empty()) {
		return true;
	}
	return false;
}

Vector<int64_t> ResourceRefSet::prepareAppendList(int64_t id, const Value &patch, bool cleanup) {
	Vector<int64_t> ids;
	if (patch.isArray() && patch.size() > 0) {
		for (auto &it : patch.asArray()) {
			Value obj;
			if (it.isNull() || (it.isDictionary() && !it.hasValue("__oid"))) {
				obj = Worker(getScheme(), _transaction).create(it);
			} else {
				obj = Worker(getScheme(), _transaction).get(it);
			}
			if (obj) {
				if (auto pushId = obj.getInteger("__oid")) {
					ids.push_back(pushId);
				}
			}
		}
	}

	return ids;
}

bool ResourceRefSet::doCleanup(int64_t id, const Vector<int64_t> &objs) {
	if (objs.empty()) {
		Worker(*_sourceScheme, _transaction).clearField(id, *_field);
	} else {
		Value objsData;
		for (auto &it : objs) {
			objsData.addInteger(it);
		}
		Worker(*_sourceScheme, _transaction).clearField(id, *_field, move(objsData));
	}
	return true;
}

Value ResourceRefSet::doAppendObject(const Value &val, bool cleanup) {
	Value arr;
	arr.addValue(val);
	return doAppendObjects(arr, cleanup);
}

Value ResourceRefSet::doAppendObjects(const Value &val, bool cleanup) {
	Value ret;
	_transaction.perform([&] { // all or nothing
		return doAppendObjectsTransaction(ret, val, cleanup);
	});

	if (!_queries.getFields().getFields()->empty()) {
		return processResultList(_queries, ret);
	}

	return ret;
}

bool ResourceRefSet::doAppendObjectsTransaction(Value &ret, const Value &val, bool cleanup) {
	auto id = getObjectId();
	if (id == 0) {
		return false;
	}

	Vector<int64_t> ids = prepareAppendList(id, val, cleanup);
	if (ids.empty()) {
		Root::getCurrent()->error("ResourceRefSet", "Empty changeset id list in update/append action", Value({
			pair("sourceScheme", Value(_sourceScheme->getName())),
			pair("targetScheme", Value(getScheme().getName())),
		}));
		return false;
	}

	Value patch;
	for (auto &it : ids) {
		patch.addInteger(it);
	}

	if (cleanup) {
		ret = Worker(*_sourceScheme, _transaction).setField(id, *_field, move(patch));
	} else {
		ret = Worker(*_sourceScheme, _transaction).appendField(id, *_field, move(patch));
	}

	return !ret.empty();
}

ResourceProperty::ResourceProperty(const Transaction &a, QueryList &&q, const Field *prop)
: Resource(ResourceType::File, a, move(q)), _field(prop) {
	_queries.setProperty(prop);
}

bool ResourceProperty::removeObject() {
	// perform one-line remove
	return _transaction.perform([&] () -> bool {
		if (auto id = getObjectId()) {
			if (Worker(getScheme(), _transaction).update(id, Value({ pair(_field->getName().str<Interface>(), Value()) }))) {
				return true;
			}
		}
		_status = HTTP_CONFLICT;
		return false;
	});
}

uint64_t ResourceProperty::getObjectId() {
	auto ids = _transaction.performQueryListForIds(_queries);
	return ids.empty() ? 0 : ids.front();
}

Value ResourceProperty::getObject(bool forUpdate) {
	Value ret = _transaction.performQueryList(_queries, _queries.size(), forUpdate);
	if (ret.isArray() && ret.size() > 0) {
		ret = move(ret.getValue(0));
	}
	return ret;
}


ResourceFile::ResourceFile(const Transaction &a, QueryList &&q, const Field *prop)
: ResourceProperty(a, move(q), prop) {
	_type = ResourceType::File;
}

bool ResourceFile::prepareUpdate() {
	return true;
}
bool ResourceFile::prepareCreate() {
	return true;
}
Value ResourceFile::updateObject(Value &, Vector<db::InputFile> &f) {
	if (f.empty()) {
		_status = HTTP_BAD_REQUEST;
		return Value();
	}

	db::InputFile *file = nullptr;
	for (auto &it : f) {
		if (it.name == _field->getName() || it.name == "content") {
			file = &it;
			break;
		} else if (it.name.empty()) {
			it.name = _field->getName().str<Interface>();
			file = &it;
			break;
		}
	}

	for (auto &it : f) {
		if (it.name != _field->getName() && &it != file) {
			it.close();
		}
	}

	if (!file) {
		_status = HTTP_BAD_REQUEST;
		return Value();
	}

	if (file->name != _field->getName()) {
		file->name = _field->getName().str<Interface>();
	}

	Value patch;
	patch.setInteger(file->negativeId(), _field->getName().str<Interface>());

	// perform one-line update
	if (auto id = getObjectId()) {
		auto ret = Worker(getScheme(), _transaction).update(id, patch);
		ret = getFileForObject(ret);
		return ret;
	}
	return Value();
}
Value ResourceFile::createObject(Value &val, Vector<db::InputFile> &f) {
	// same as update
	return updateObject(val, f);
}

Value ResourceFile::getResultObject() {
	if (_field->hasFlag(db::Flags::Protected)) {
		_status = HTTP_NOT_FOUND;
		return Value();
	}
	return getDatabaseObject();
}

Value ResourceFile::getFileForObject(Value &object) {
	if (object.isDictionary()) {
		auto id = object.getInteger(_field->getName());
		if (id) {
			auto fileScheme = Host::getCurrent().getFileScheme();
			Value ret(Worker(*fileScheme, _transaction).get(id));
			return ret;
		}
	}
	return Value();
}

Value ResourceFile::getDatabaseObject() {
	return _transaction.performQueryListField(_queries, *_field);
}

ResourceArray::ResourceArray(const Transaction &a, QueryList &&q, const Field *prop)
: ResourceProperty(a, move(q), prop) {
	_type = ResourceType::Array;
}

bool ResourceArray::prepareUpdate() {
	return true;
}
bool ResourceArray::prepareCreate() {
	return true;
}
Value ResourceArray::updateObject(Value &data, Vector<db::InputFile> &) {
	Value arr;
	if (data.isDictionary()) {
		auto &newArr = data.getValue(_field->getName());
		if (newArr.isArray()) {
			arr = std::move(newArr);
		} else if (!newArr.isNull()) {
			arr.addValue(newArr);
		}
	} else if (data.isArray()) {
		arr = std::move(data);
	}

	if (!arr.isArray()) {
		_status = HTTP_BAD_REQUEST;
		return Value();
	}

	// perform one-line update
	if (auto id = getObjectId()) {
		return Worker(getScheme(), _transaction).setField(id, *_field, std::move(arr));
	}
	return Value();
}
Value ResourceArray::createObject(Value &data, Vector<db::InputFile> &) {
	Value arr;
	if (data.isDictionary()) {
		auto &newArr = data.getValue(_field->getName());
		if (newArr.isArray()) {
			arr = std::move(newArr);
		} else if (!newArr.isNull()) {
			arr.addValue(newArr);
		}
	} else if (data.isArray()) {
		arr = std::move(data);
	} else if (data.isBasicType()) {
		arr.addValue(std::move(data));
	}

	if (!arr.isArray()) {
		_status = HTTP_BAD_REQUEST;
		return Value(false);
	}

	// perform one-line update
	if (auto id = getObjectId()) {
		return Worker(getScheme(), _transaction).appendField(id, *_field, move(arr));
	}
	return Value();
}

Value ResourceArray::getResultObject() {
	if (_field->hasFlag(db::Flags::Protected)) {
		_status = HTTP_NOT_FOUND;
		return Value();
	}
	return getDatabaseObject();
}

Value ResourceArray::getDatabaseObject() {
	return _transaction.performQueryListField(_queries, *_field);
}

Value ResourceArray::getArrayForObject(Value &object) {
	if (object.isDictionary()) {
		return Worker(getScheme(), _transaction).getField(object, *_field);
	}
	return Value();
}

ResourceFieldObject::ResourceFieldObject(const Transaction &a, QueryList &&q)
: ResourceObject(a, move(q)), _sourceScheme(_queries.getSourceScheme()), _field(_queries.getField()) {
	_type = ResourceType::ObjectField;
}

bool ResourceFieldObject::prepareUpdate() {
	return true;
}

bool ResourceFieldObject::prepareCreate() {
	return true;
}

bool ResourceFieldObject::prepareAppend() {
	return false;
}

bool ResourceFieldObject::removeObject() {
	auto id = getObjectId();
	if (id == 0) {
		return Value();
	}

	return _transaction.perform([&] () -> bool {
		return doRemoveObject();
	});
}

Value ResourceFieldObject::updateObject(Value &val, Vector<db::InputFile> &files) {
	// create or update object
	Value ret;
	_transaction.perform([&] () -> bool {
		if (getObjectId()) {
			ret = doUpdateObject(val, files);
		} else {
			ret = doCreateObject(val, files);
		}
		if (ret) {
			return true;
		}
		return false;
	});
	return ret;
}

Value ResourceFieldObject::createObject(Value &val, Vector<db::InputFile> &files) {
	// remove then recreate object
	Value ret;
	_transaction.perform([&] () -> bool {
		if (getObjectId()) {
			if (!doRemoveObject()) {
				return Value();
			}
		}
		ret = doCreateObject(val, files);
		if (ret) {
			return true;
		}
		return false;
	});
	return ret;
}

Value ResourceFieldObject::appendObject(Value &) {
	return Value();
}

int64_t ResourceFieldObject::getRootId() {
	if (!_rootId) {
		auto ids = _transaction.performQueryListForIds(_queries, _queries.getItems().size() - 1);
		if (!ids.empty()) {
			_rootId = ids.front();
		}
	}
	return _rootId;
}

int64_t ResourceFieldObject::getObjectId() {
	if (!_objectId) {
		if (auto id = getRootId()) {
			if (auto obj = Worker(getScheme(), _transaction).get(id, {_field->getName()})) {
				_objectId = obj.getInteger(_field->getName());
			}
		}
	}
	return _objectId;
}

Value ResourceFieldObject::getRootObject(bool forUpdate) {
	if (auto id = getRootId()) {
		return Worker(*_sourceScheme, _transaction).get(id, {_field->getName()},
				forUpdate ? db::UpdateFlags::GetForUpdate : db::UpdateFlags::None);
	}
	return Value();
}

Value ResourceFieldObject::getTargetObject(bool forUpdate) {
	if (auto id = getObjectId()) {
		return Worker(getScheme(), _transaction).get(id, { StringView() },
				forUpdate ? db::UpdateFlags::GetForUpdate : db::UpdateFlags::None);
	}
	return Value();
}

bool ResourceFieldObject::doRemoveObject() {
	return Worker(*_sourceScheme, _transaction).clearField(getRootId(), *_field);
}

Value ResourceFieldObject::doUpdateObject(Value &val, Vector<db::InputFile> &files) {
	encodeFiles(val, files);
	return Worker(getScheme(), _transaction).update(getObjectId(), val);
}

Value ResourceFieldObject::doCreateObject(Value &val, Vector<db::InputFile> &files) {
	encodeFiles(val, files);
	if (auto ret = Worker(getScheme(), _transaction).create(val)) {
		if (auto id = ret.getInteger("__oid")) {
			if (Worker(*_sourceScheme, _transaction).update(getRootId(), Value({
				pair(_field->getName().str<Interface>(), Value(id))
			}))) {
				return ret;
			}
		}
	}
	return Value();
}


ResourceView::ResourceView(const Transaction &h, QueryList &&q)
: ResourceSet(h, move(q)), _field(_queries.getField()) {
	if (_queries.isDeltaApplicable()) {
		auto tag = _queries.getItems().front().query.getSingleSelectId();
		_delta = Time::microseconds(_transaction.getDeltaValue(*_queries.getSourceScheme(), *static_cast<const db::FieldView *>(_field->getSlot()), tag));
	}
}

bool ResourceView::prepareUpdate() { return false; }
bool ResourceView::prepareCreate() { return false; }
bool ResourceView::prepareAppend() { return false; }
bool ResourceView::removeObject() { return false; }

Value ResourceView::updateObject(Value &data, Vector<db::InputFile> &) { return Value(); }
Value ResourceView::createObject(Value &data, Vector<db::InputFile> &) { return Value(); }

Value ResourceView::getResultObject() {
	auto ret = _transaction.performQueryListField(_queries, *_field);
	if (!ret.isArray()) {
		return Value();
	}

	return processResultList(_queries, ret);
}

ResourceSearch::ResourceSearch(const Transaction &a, QueryList &&q, const Field *prop)
: ResourceObject(a, move(q)), _field(prop) {
	_type = ResourceType::Search;
}

Value ResourceSearch::getResultObject() {
	auto slot = _field->getSlot<db::FieldFullTextView>();
	if (auto &searchData = _queries.getExtraData().getValue("search")) {
		Vector<db::FullTextData> q = slot->parseQuery(searchData);
		stappler::search::Language lang = stappler::search::Language::English;
		for (auto &it : q) {
			if (it.language != lang) {
				lang = it.language;
			}
			it.language = search::Language::Simple; // prevent to secondary stem
		}
		_config.setLanguage(lang);

		if (!q.empty()) {
			_queries.setFullTextQuery(_field, Vector<db::FullTextData>(q));
			auto ret = _transaction.performQueryList(_queries);
			if (!ret.isArray()) {
				return Value();
			}

			auto res = processResultList(_queries, ret);
			if (!res.empty()) {
				if (auto &headlines = _queries.getExtraData().getValue("headlines")) {
					auto ql = _config.stemQuery(q);
					for (auto &it : res.asArray()) {
						makeHeadlines(it, headlines, ql);
					}
				}
			}
			return res;
		}
	}
	return Value();
}

/* Vector<String> ResourceSearch::stemQuery(const Vector<db::FullTextData> &query) {
	Vector<String> ret; ret.reserve(256 / sizeof(String)); // memory manager hack

	for (auto &it : query) {
		StringView r(it.buffer);
		r.split<StringView::CharGroup<CharGroupId::WhiteSpace>>([&] (StringView &iword) {
			StringViewUtf8 word(iword.data(), iword.size());
			word.trimUntil<StringViewUtf8::MatchCharGroup<CharGroupId::Cyrillic>, StringViewUtf8::MatchCharGroup<CharGroupId::Alphanumeric>>();
			if (word.size() > 3) {
				ret.emplace_back(_stemmer.stemWord(word, it.language).str());
			}
		});
	}

	return ret;
} */

void ResourceSearch::makeHeadlines(Value &obj, const Value &headlineInfo, const Vector<String> &ql) {
	auto &h = obj.emplace("__headlines");
	for (auto &it : headlineInfo.asDict()) {
		auto d = getObjectLine(obj, it.first);
		if (d && d->isString()) {
			auto headStr = makeHeadline(d->getString(), it.second, ql);
			if (!headStr.empty()) {
				h.setString(headStr, it.first);
			}
		}
	}
}

String ResourceSearch::makeHeadline(const StringView &value, const Value &headlineInfo, const Vector<String> &ql) {
	stappler::search::Configuration::HeadlineConfig cfg;
	if (headlineInfo.isString()) {
		if (headlineInfo.getString() == "plain") {
			cfg.startToken = StringView("<b>"); cfg.stopToken = StringView("</b>");
			return _config.makeHeadline(cfg, value, ql);
		} else if (headlineInfo.getString() == "html") {
			cfg.startToken = StringView("<b>"); cfg.stopToken = StringView("</b>");
			cfg.startFragment = StringView("<p>"); cfg.stopFragment = StringView("</p>");
			return _config.makeHtmlHeadlines(cfg, value, ql);
		}
	} else if (headlineInfo.isDictionary()) {
		auto type = headlineInfo.getString("type");
		auto start = headlineInfo.getString("start");
		auto end = headlineInfo.getString("end");

		auto l = search::parseLanguage(headlineInfo.getString("lang"));
		if (l != search::Language::Unknown && l != _config.getLanguage()) {
			_config.setLanguage(l);
		}
		if (type == "html") {
			if (!start.empty() && start.size() < 24 && !end.empty() && end.size() < 24) {
				cfg.startToken = start; cfg.stopToken = end;
			} else {
				cfg.startToken = StringView("<b>"); cfg.stopToken = StringView("</b>");
			}

			auto fragStart = headlineInfo.getString("fragStart");
			auto fragEnd = headlineInfo.getString("fragStop");
			if (!fragStart.empty() && fragStart.size() < 24 && !fragEnd.empty() && fragEnd.size() < 24) {
				cfg.startFragment = fragStart; cfg.stopFragment = fragEnd;
			} else {
				cfg.startFragment = StringView("<p>"); cfg.stopFragment = StringView("</p>");
			}

			cfg.maxWords = headlineInfo.getInteger("maxWords", search::Configuration::HeadlineConfig::DefaultMaxWords);
			cfg.minWords = headlineInfo.getInteger("minWords", search::Configuration::HeadlineConfig::DefaultMinWords);
			cfg.shortWord = headlineInfo.getInteger("shortWord", search::Configuration::HeadlineConfig::DefaultShortWord);

			size_t frags = 1;
			auto fragments = headlineInfo.getValue("fragments");
			if (fragments.isString() && fragments.getString() == "max") {
				frags = maxOf<size_t>();
			} else if (auto f = fragments.asInteger()) {
				frags = f;
			}

			return _config.makeHtmlHeadlines(cfg, value, ql, frags);
		} else {
			if (!start.empty() && start.size() < 24 && !end.empty() && end.size() < 24) {
				cfg.startToken = start; cfg.stopToken = end;
			} else {
				cfg.startToken = StringView("<b>"); cfg.stopToken = StringView("</b>");
			}

			return _config.makeHeadline(cfg, value, ql);
		}
	}
	return String();
}

const Value *ResourceSearch::getObjectLine(const Value &obj, const StringView &key) {
	const Value *ptr = &obj;
	key.split<StringView::Chars<'.'>>([&] (const StringView &k) {
		if (ptr && ptr->isDictionary()) {
			if (auto &v = ptr->getValue(k)) {
				ptr = &v;
			}
		} else {
			ptr = nullptr;
		}
	});
	return ptr;
}

}
