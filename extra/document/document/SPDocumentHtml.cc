/**
 Copyright (c) 2023 Stappler LLC <admin@stappler.dev>

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

#include "SPHtmlParser.h"
#include "SPDocumentFormat.h"
#include "SPDocumentHtml.h"
#include "SPDocumentPageContainer.h"

namespace STAPPLER_VERSIONIZED stappler::document {

class Node;

struct HtmlTag : html::Tag<StringView> {
	enum Type {
		Style,
		Block,
		Image,
		Special,
		Html,
		Head,
		Body,
		Title,
		Meta,
		Link,
		Script,
		Base,
		Markup,
	};

	StringView id; // <tag id="">
	StringView xType;
	Type type = Markup;

	bool autoRefs = false;

	Node *node = nullptr;

	operator bool() const { return !name.empty(); }

	static Type getType(const StringView &tagName);
	static bool caseCompare(const StringView &a, const StringView &b);
};

struct HtmlReader {
	using Parser = html::Parser<HtmlReader, StringView, HtmlTag>;

	template <typename T>
	using Vector = memory::PoolInterface::VectorType<T>;

	inline void onBeginTag(Parser &p, HtmlTag &tag) {
		tag.type = HtmlTag::getType(tag.name);
		switch (tag.type) {
		case HtmlTag::Style:
		case HtmlTag::Script:
			tag.nestedTagsAllowed = false;
			break;
		case HtmlTag::Special:
		case HtmlTag::Meta:
		case HtmlTag::Link:
		case HtmlTag::Base:
			tag.closable = false;
			break;
		default:
			break;
		}

		if (HtmlTag::caseCompare(tag.name, "br") || HtmlTag::caseCompare(tag.name, "hr") || HtmlTag::caseCompare(tag.name, "col")) {
			tag.closable = false;
		}
		log::debug("onBeginTag", tag.name);
	}

	inline void onEndTag(Parser &p, HtmlTag &tag, bool isClosable) {
		log::debug("onEndTag", tag.name);
	}

	inline void onTagAttribute(Parser &p, HtmlTag &tag, StringView &name, StringView &value) {
		log::debug("onTagAttribute", tag.name, ": ", name, " = ", value);
	}

	inline void onTagAttributeList(Parser &p, HtmlTag &tag, StringView &data) {
		switch (tag.type) {
		case HtmlTag::Meta:
			data.trimChars<StringView::WhiteSpace>();
			page->setMeta(data);
			break;
		case HtmlTag::Base:
			data.trimChars<StringView::WhiteSpace>();
			//page->setBase(data);
			break;
		case HtmlTag::Link:
			data.trimChars<StringView::WhiteSpace>();
			//page->addLink(data);
			break;
		default:
			break;
		}
		log::debug("onTagAttributeList", tag.name, ": ", data);
	}

	inline void onPushTag(Parser &p, HtmlTag &tag) {
		switch (tag.type) {
		case HtmlTag::Html:
			++ _htmlTag;
			break;
		case HtmlTag::Head:
			++ _headTag;
			break;
		case HtmlTag::Body:
			++ _bodyTag;
			break;
		default:
			break;
		}
		log::debug("onPushTag", tag.name);
	}

	inline void onPopTag(Parser &p, HtmlTag &tag) {
		switch (tag.type) {
		case HtmlTag::Html:
			-- _htmlTag;
			break;
		case HtmlTag::Head:
			-- _headTag;
			break;
		case HtmlTag::Body:
			-- _bodyTag;
			break;
		default:
			break;
		}
		log::debug("onPopTag", tag.name);
	}

	inline void onInlineTag(Parser &p, HtmlTag &tag) {
		log::debug("onInlineTag", tag.name);
	}

	inline void onTagContent(Parser &p, HtmlTag &tag, StringView &s) {
		if (tag.type == HtmlTag::Title) {
			s.trimChars<StringView::WhiteSpace>();
			page->setTitle(s);
		} else if (tag.type == HtmlTag::Title) {
			s.trimChars<StringView::WhiteSpace>();
			PageContainer::StringReader r(s);
			page->readStyle(r);
		}
		log::debug("onTagContent", tag.name, ": ", s);
	}

	inline void onSchemeTag(Parser &p, StringView &name, StringView &value) {
		//log::debug("onSchemeTag", name, ": ", value);
	}

	inline void onCommentTag(Parser &p, StringView &data) {
		//log::debug("onCommentTag", data);
	}

	HtmlReader(PageContainer *);

	PageContainer *page = nullptr;
	Vector<Node *> nodeStack;

	uint32_t _htmlTag = 0;
	uint32_t _bodyTag = 0;
	uint32_t _headTag = 0;
	uint32_t _pseudoId = 0;
};

static Format s_htmlFormat([] (memory::pool_t *, FilePath str, StringView ct) -> bool {
	return DocumentHtml::isHtml(str);
}, [] (memory::pool_t *p, FilePath str, StringView ct) -> Rc<Document> {
	return Rc<DocumentHtml>::create(p, str, ct);
}, [] (memory::pool_t *, BytesView str, StringView ct) -> bool {
	return DocumentHtml::isHtml(str);
}, [] (memory::pool_t *p, BytesView str, StringView ct) -> Rc<Document> {
	return Rc<DocumentHtml>::create(p, str, ct);
}, 0);

HtmlReader::HtmlReader(PageContainer *p) : page(p) {
	nodeStack.emplace_back(p->getRoot());
}

HtmlTag::Type HtmlTag::getType(const StringView &tagName) {
	if (caseCompare(tagName, "img")) {
		return HtmlTag::Image;
	} else if (tagName[0] == '!' || tagName[0] == '-' || caseCompare(tagName, "xml")) {
		return HtmlTag::Special;
	} else if (caseCompare(tagName, "html")) {
		return HtmlTag::Html;
	} else if (caseCompare(tagName, "head")) {
		return HtmlTag::Head;
	} else if (caseCompare(tagName, "body")) {
		return HtmlTag::Body;
	} else if (caseCompare(tagName, "title")) {
		return HtmlTag::Title;
	} else if (caseCompare(tagName, "style")) {
		return HtmlTag::Style;
	} else if (caseCompare(tagName, "meta")) {
		return HtmlTag::Meta;
	} else if (caseCompare(tagName, "link")) {
		return HtmlTag::Link;
	} else if (caseCompare(tagName, "script")) {
		return HtmlTag::Script;
	} else if (caseCompare(tagName, "base")) {
		return HtmlTag::Base;
	} else {
		return HtmlTag::Block;
	}
}

bool HtmlTag::caseCompare(const StringView &a, const StringView &b) {
	if (a.size() != b.size()) {
		return false;
	} else {
		return ::strncasecmp(a.data(), b.data(), a.size()) == 0;
	}
}

bool DocumentHtml::isHtml(StringView r) {
	r.skipChars<StringView::WhiteSpace>();
	if (r.starts_with("<!DOCTYPE")) {
		r += "<!DOCTYPE"_len;
		r.skipChars<StringView::WhiteSpace>();
		auto tmp = r.readUntil<StringView::Chars<'>'>>();
		tmp.trimChars<StringView::WhiteSpace>();
		return ::strncasecmp(tmp.data(), "html", 4) == 0;
	}
	return false;
}

bool DocumentHtml::isHtml(FilePath str) {
	uint8_t buf[256] = { 0 };
	filesystem::readIntoBuffer(buf, str.get(), 0, 256);
	return isHtml(StringView(reinterpret_cast<const char *>(buf), 256));
}

bool DocumentHtml::isHtml(BytesView str) {
	return isHtml(StringView(reinterpret_cast<const char *>(str.data()), str.size()));
}

bool DocumentHtml::init(FilePath path, StringView ct) {
	if (!Document::init()) {
		return false;
	}

	auto data = filesystem::readIntoMemory<memory::PoolInterface>(path.get());
	return read(data, ct);
}

bool DocumentHtml::init(BytesView data, StringView ct) {
	if (!Document::init()) {
		return false;
	}

	return read(data, ct);
}

bool DocumentHtml::init(memory::pool_t *pool, FilePath path, StringView ct) {
	if (!Document::init(pool)) {
		return false;
	}

	auto data = filesystem::readIntoMemory<memory::PoolInterface>(path.get());
	return read(data, ct);
}

bool DocumentHtml::init(memory::pool_t *pool, BytesView data, StringView ct) {
	if (!Document::init(pool)) {
		return false;
	}

	return read(data, ct);
}

bool DocumentHtml::read(BytesView data, StringView ct) {
	memory::pool::context ctx(_pool);
	auto page = Rc<PageContainer>::create();

	HtmlReader reader(page);

	html::parse<HtmlReader, StringView, HtmlTag>(reader,
			StringView(reinterpret_cast<const char *>(data.data()), data.size()), false);

	_root = move(page);

	return true;
}

}
