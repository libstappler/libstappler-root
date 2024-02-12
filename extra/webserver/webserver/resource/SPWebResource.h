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

#ifndef EXTRA_WEBSERVER_WEBSERVER_RESOURCE_SPWEBRESOURCE_H_
#define EXTRA_WEBSERVER_WEBSERVER_RESOURCE_SPWEBRESOURCE_H_

#include "SPWebInfo.h"
#include "SPDbFile.h"
#include "SPDbUser.h"

namespace STAPPLER_VERSIONIZED stappler::web {

using ResolveOptions = db::Resolve;

class Resource : public AllocBase {
public:
	using Transaction = db::Transaction;
	using Scheme = db::Scheme;
	using Worker = db::Worker;
	using Field = db::Field;
	using Object = db::Object;
	using User = db::User;
	using File = db::File;
	using Query = db::Query;
	using QueryList = db::QueryList;

	using QueryFieldResolver = db::QueryFieldResolver;

	static Resource *resolve(const Transaction &, const Scheme &scheme, const StringView &path);
	static Resource *resolve(const Transaction &, const Scheme &scheme, const StringView &path, Value & sub);

	/* PathVec should be inverted (so, first selectors should be last in vector */
	static Resource *resolve(const Transaction &, const Scheme &scheme, Vector<StringView> &path);

	virtual ~Resource();
	Resource(ResourceType, const Transaction &, QueryList &&);

	ResourceType getType() const;
	const Scheme &getScheme() const;
	Status getStatus() const;

	bool isDeltaApplicable() const;
	bool hasDelta() const;

	void setQueryDelta(Time);
	Time getQueryDelta() const;

	Time getSourceDelta() const;

	void setUser(User *);
	void setFilterData(const Value &);

	void setResolveOptions(const Value & opts);
	void setResolveDepth(size_t size);

	void setPageFrom(size_t);
	void setPageCount(size_t);

	void applyQuery(const Value &);

	void prepare(QueryList::Flags = QueryList::None);

	const QueryList &getQueries() const;

public: // common interface
	virtual bool prepareUpdate();
	virtual bool prepareCreate();
	virtual bool prepareAppend();
	virtual bool removeObject();
	virtual Value updateObject(Value &, Vector<db::InputFile> &);
	virtual Value createObject(Value &, Vector<db::InputFile> &);
	virtual Value appendObject(Value &);

	virtual Value getResultObject();
	virtual void resolve(const Scheme &, Value &); // called to apply resolve rules to object

public:
	size_t getMaxRequestSize() const;
	size_t getMaxVarSize() const;
	size_t getMaxFileSize() const;

protected:
	void encodeFiles(Value &, Vector<db::InputFile> &);

	void resolveSet(const QueryFieldResolver &, int64_t, const Field &, Value &);
	void resolveObject(const QueryFieldResolver &, int64_t, const Field &, Value &);
	void resolveArray(const QueryFieldResolver &, int64_t, const Field &, Value &);
	void resolveFile(const QueryFieldResolver &, int64_t, const Field &, Value &);

	int64_t processResolveResult(const QueryFieldResolver &res, const Set<const Field *> &, Value &obj);

	void resolveResult(const QueryFieldResolver &res, Value &obj, uint16_t depth, uint16_t max);
	void resolveResult(const QueryList &, Value &);

protected:
	virtual const Scheme &getRequestScheme() const;
	void resolveOptionForString(StringView);
	void addExtraResolveField(StringView);

	Time _delta;
	ResourceType _type = ResourceType::Object;
	Status _status = HTTP_OK;

	Transaction _transaction;
	QueryList _queries;

	User *_user = nullptr;
	Set<int64_t> _resolveObjects;
	Value _filterData;

	bool _isResolvesUpdated = true;
	Vector<StringView> _extraResolves;
	ResolveOptions _resolve = ResolveOptions::None;
};

class ResourceProperty : public Resource {
public:
	ResourceProperty(const Transaction &h, QueryList &&q, const Field *prop);

	virtual bool removeObject() override;

protected:
	uint64_t getObjectId();
	Value getObject(bool forUpdate);

	const Field *_field = nullptr;
};

class ResourceFile : public ResourceProperty {
public:
	ResourceFile(const Transaction &h, QueryList &&q, const Field *prop);

	virtual bool prepareUpdate() override;
	virtual bool prepareCreate() override;
	virtual Value updateObject(Value &, Vector<db::InputFile> &f) override;
	virtual Value createObject(Value &val, Vector<db::InputFile> &f) override;

	virtual Value getResultObject() override;

protected:
	Value getFileForObject(Value &object);
	Value getDatabaseObject();
};

class ResourceArray : public ResourceProperty {
public:
	ResourceArray(const Transaction &h, QueryList &&q, const Field *prop);

	virtual bool prepareUpdate() override;
	virtual bool prepareCreate() override;
	virtual Value updateObject(Value &data, Vector<db::InputFile> &) override;
	virtual Value createObject(Value &data, Vector<db::InputFile> &) override;
	virtual Value getResultObject() override;

protected:
	Value getDatabaseObject();
	Value getArrayForObject(Value &object);
};

class ResourceObject : public Resource {
public:
	ResourceObject(const Transaction &a, QueryList &&q);

	virtual bool prepareUpdate() override;
	virtual bool prepareCreate() override;
	virtual bool prepareAppend() override;
	virtual bool removeObject() override;
	virtual Value updateObject(Value &data, Vector<db::InputFile> &) override;
	virtual Value getResultObject() override;

	virtual int64_t getObjectMtime();

protected:
	Value performUpdate(const Vector<int64_t> &, Value &, Vector<db::InputFile> &);

	Value processResultList(const QueryList &s, Value &ret);
	bool processResultObject(const QueryList &s, Value &obj);
	Value getDatabaseObject();
	Vector<int64_t> getDatabaseId(const QueryList &q, size_t count = maxOf<size_t>());
};

class ResourceReslist : public ResourceObject {
public:
	ResourceReslist(const Transaction &a, QueryList &&q);

	virtual bool prepareCreate() override;
	virtual Value createObject(Value &, Vector<db::InputFile> &) override;

protected:
	Value performCreateObject(Value &data, Vector<db::InputFile> &files, const Value &extra);
};

class ResourceSet : public ResourceReslist {
public:
	ResourceSet(const Transaction &a, QueryList &&q);

	virtual bool prepareAppend() override;
	virtual Value createObject(Value &, Vector<db::InputFile> &) override;
	virtual Value appendObject(Value &) override;
};

class ResourceRefSet : public ResourceSet {
public:
	ResourceRefSet(const Transaction &a, QueryList &&q);

	virtual bool prepareUpdate() override;
	virtual bool prepareCreate() override;
	virtual bool prepareAppend() override;
	virtual bool removeObject() override;
	virtual Value updateObject(Value &, Vector<db::InputFile> &) override;
	virtual Value createObject(Value &, Vector<db::InputFile> &) override;
	virtual Value appendObject(Value &) override;

protected:
	int64_t getObjectId();
	Value getObjectValue();

	Vector<int64_t> prepareAppendList(int64_t id, const Value &, bool cleanup);

	bool doCleanup(int64_t,const Vector<int64_t> &);

	Value doAppendObject(const Value &, bool cleanup);
	Value doAppendObjects(const Value &, bool cleanup);
	bool doAppendObjectsTransaction(Value &, const Value &, bool cleanup);

	bool isEmptyRequest();

	int64_t _objectId = 0;
	Value _objectValue;
	const Scheme *_sourceScheme = nullptr;
	const Field *_field = nullptr;
};

class ResourceFieldObject : public ResourceObject {
public:
	ResourceFieldObject(const Transaction &a, QueryList &&q);

	virtual bool prepareUpdate() override;
	virtual bool prepareCreate() override;
	virtual bool prepareAppend() override;
	virtual bool removeObject() override;
	virtual Value updateObject(Value &, Vector<db::InputFile> &) override;
	virtual Value createObject(Value &, Vector<db::InputFile> &) override;
	virtual Value appendObject(Value &) override;

protected:
	int64_t getRootId();
	int64_t getObjectId();

	Value getRootObject(bool forUpdate);
	Value getTargetObject(bool forUpdate);

	bool doRemoveObject();
	Value doUpdateObject(Value &, Vector<db::InputFile> &);
	Value doCreateObject(Value &, Vector<db::InputFile> &);

	int64_t _objectId = 0;
	int64_t _rootId = 0;
	const Scheme *_sourceScheme = nullptr;
	const Field *_field = nullptr;
};

class ResourceView : public ResourceSet {
public:
	ResourceView(const Transaction &h, QueryList &&q);

	virtual bool prepareUpdate() override;
	virtual bool prepareCreate() override;
	virtual bool prepareAppend() override;
	virtual bool removeObject() override;

	virtual Value updateObject(Value &data, Vector<db::InputFile> &) override;
	virtual Value createObject(Value &data, Vector<db::InputFile> &) override;

	virtual Value getResultObject() override;

protected:
	const Field *_field = nullptr;
};

class ResourceSearch : public ResourceObject {
public:
	ResourceSearch(const Transaction &h, QueryList &&q, const Field *prop);

	virtual Value getResultObject() override;

protected:
	// Vector<String> stemQuery(const Vector<db::FullTextData> &);

	void makeHeadlines(Value &obj, const Value &headlineInfo, const Vector<String> &);
	String makeHeadline(const StringView &value, const Value &headlineInfo, const Vector<String> &);

	const Value *getObjectLine(const Value &obj, const StringView &);

	const Field *_field = nullptr;
};

}

#endif /* EXTRA_WEBSERVER_WEBSERVER_RESOURCE_SPWEBRESOURCE_H_ */
