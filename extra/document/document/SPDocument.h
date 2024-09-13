/**
 Copyright (c) 2023-2023 Stappler LLC <admin@stappler.dev>

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

#ifndef EXTRA_DOCUMENT_DOCUMENT_SPDOCUMENT_H_
#define EXTRA_DOCUMENT_DOCUMENT_SPDOCUMENT_H_

#include "SPRef.h"
#include "SPFilesystem.h"
#include "SPBytesView.h"
#include "SPMemory.h"
#include "SPDocStyle.h"

namespace STAPPLER_VERSIONIZED stappler::document {

using NodeId = uint32_t;
constexpr NodeId NodeIdNone = maxOf<NodeId>();

class Node;
class StyleContainer;
class PageContainer;

using StringDocument = ValueWrapper<StringView, class StringDocumentTag>;

class Document;

struct SP_PUBLIC DocumentImage : public memory::AllocPool {
	enum Type {
		Embed,
		Local,
		Web,
	};

	Type type = Local;

	uint16_t width = 0;
	uint16_t height = 0;

	size_t offset = 0;
	size_t length = 0;

	StringView path;
	StringView ref;
	StringView ct;
	BytesView data;

	DocumentImage() = default;

	DocumentImage(uint16_t w, uint16_t h, size_t size, StringView p, StringView r = StringView())
	: width(w), height(h), length(size), path(p.pdup()), ref(r.pdup()) { }
};

struct SP_PUBLIC DocumentContentRecord {
	template <typename Value>
	using Vector = typename memory::PoolInterface::VectorType<Value>;

	StringView label;
	StringView href;
	Vector<DocumentContentRecord> childs;
};

struct SP_PUBLIC DocumentData : public memory::AllocPool, public InterfaceObject<memory::PoolInterface> {
	memory::pool_t *pool = nullptr;
	StringView name;
	Vector<StringView> spine;
	Vector<StringView> strings;
	Vector<MediaQuery> queries;
	Map<StringView, StyleContainer *> styles;
	Map<StringView, PageContainer *> pages;
	Map<StringView, DocumentImage> images;
	Map<StringView, StringView> meta;
	DocumentContentRecord tableOfContents;

	NodeId maxNodeId = NodeIdNone;

	StringId addString(const StringView &str);
	MediaQueryId addQuery(MediaQuery &&);
};

class SP_PUBLIC Document : public RefBase<memory::StandartInterface> {
public:
	static bool canOpen(FilePath path, StringView ct = StringView());
	static bool canOpen(BytesView data, StringView ct = StringView());
	static bool canOpen(memory::pool_t *, FilePath path, StringView ct = StringView());
	static bool canOpen(memory::pool_t *, BytesView data, StringView ct = StringView());

	static Rc<Document> open(FilePath path, StringView ct = StringView());
	static Rc<Document> open(BytesView data, StringView ct = StringView());
	static Rc<Document> open(memory::pool_t *, FilePath path, StringView ct = StringView());
	static Rc<Document> open(memory::pool_t *, BytesView data, StringView ct = StringView());

	virtual ~Document();

	virtual bool init();
	virtual bool init(memory::pool_t *);

	virtual StringView getName() const;
	virtual SpanView<StringView> getSpine() const;
	virtual const DocumentContentRecord & getTableOfContents() const;

	virtual StringView getMeta(StringView) const;

	virtual bool isFileExists(StringView) const;
	virtual const DocumentImage *getImage(StringView) const;
	virtual const PageContainer *getContentPage(StringView) const;
	virtual const StyleContainer *getStyleDocument(StringView) const;

	virtual const PageContainer *getRoot() const;

	virtual const Node *getNodeById(StringView pagePath, StringView id) const;
	virtual Pair<const PageContainer *, const Node *> getNodeByIdGlobal(StringView id) const;

	virtual void foreachPage(const Callback<void(StringView, const PageContainer *)> &);

	NodeId getMaxNodeId() const;

	const DocumentData *getData() const { return _data; }

	// Default style, that can be redefined with css
	virtual void beginStyle(StyleList &, const Node &, SpanView<const Node *>, const MediaParameters &) const;

	// Default style, that can NOT be redefined with css
	virtual void endStyle(StyleList &, const Node &, SpanView<const Node *>, const MediaParameters &) const;

protected:
	virtual DocumentData *allocateData(memory::pool_t *);

	virtual void onStyleAttribute(StyleList &style, StringView tag, StringView name, StringView value, const MediaParameters &) const;

	memory::pool_t *_pool = nullptr;
	DocumentData *_data = nullptr;
};

}

#endif /* EXTRA_DOCUMENT_DOCUMENT_SPDOCUMENT_H_ */
