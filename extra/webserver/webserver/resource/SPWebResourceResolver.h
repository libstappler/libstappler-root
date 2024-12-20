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

#ifndef EXTRA_WEBSERVER_WEBSERVER_RESOURCE_SPWEBRESOURCERESOLVER_H_
#define EXTRA_WEBSERVER_WEBSERVER_RESOURCE_SPWEBRESOURCERESOLVER_H_

#include "SPWebRequest.h"
#include "SPWebInfo.h"
#include "SPDbScheme.h"

namespace STAPPLER_VERSIONIZED stappler::web {

class Resource;

class SP_PUBLIC ResourceResolver : public AllocBase {
public:
	ResourceResolver(const db::Transaction &a, const db::Scheme &scheme);

	bool selectById(uint64_t); // objects/id123
	bool selectByAlias(const StringView &); // objects/named-alias
	bool selectByQuery(db::Query::Select &&); // objects/select/counter/2 or objects/select/counter/bw/10/20

	bool searchByField(const db::Field *);

	bool order(const StringView &f, db::Ordering o); // objects/order/counter/desc
	bool first(const StringView &f, size_t v); // objects/first/10
	bool last(const StringView &f, size_t v); // objects/last/10
	bool limit(size_t limit); // objects/order/counter/desc/10
	bool offset(size_t offset); // objects/order/counter/desc/10

	bool getObject(const db::Field *); // objects/id123/owner
	bool getSet(const db::Field *); // objects/id123/childs
	bool getView(const db::Field *); // objects/id123/childs
	bool getField(const StringView &, const db::Field *); // objects/id123/image
	bool getAll(); // objects/id123/childs/all

	Resource *getResult();

	const db::Scheme *getScheme() const;

protected:
	Resource *makeResource(ResourceType type, db::QueryList &&list, const db::Field *f);

	enum InternalResourceType {
		Objects,
		File,
		Array,
		Search,
	};

	bool _all = false;
	db::Transaction _storage;
	const db::Scheme *_scheme = nullptr;
	InternalResourceType _type = Objects;
	Resource *_resource = nullptr;
	db::QueryList _queries;
};

}

#endif /* EXTRA_WEBSERVER_WEBSERVER_RESOURCE_SPWEBRESOURCERESOLVER_H_ */
