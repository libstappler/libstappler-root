/**
 Copyright (c) 2023-2024 Stappler LLC <admin@stappler.dev>

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
#include "SPDocFormat.h"
#include "SPDocHtml.h"
#include "SPDocPageContainer.h"

namespace STAPPLER_VERSIONIZED stappler::document {

class Node;

struct HtmlTag : html::Tag<StringView> {
	enum Type {
		Html,
		Head,
		Meta,
		Title,
		Base,
		Link,
		Style,
		Script,
		Body,
		Image,
		Special,
		Block,
	};

	Type type = Block;
	Node *node = nullptr;

	HtmlTag(StringView name)
	: Tag(name) { }

	explicit operator bool() const { return !name.empty(); }

	static Type getType(const StringView &tagName);
	static bool isForceUnclosed(const StringView &tagName);;
};

struct HtmlReader {
	using Parser = html::Parser<HtmlReader, StringView, HtmlTag>;

	using Interface = memory::PoolInterface;

	template <typename T>
	using Vector = Interface::VectorType<T>;

	using String = Interface::StringType;

	static String encodePathString(StringView r) {
		String ret; ret.resize(r.size());

		size_t offset = 0;
		auto fn = [&] (StringView v) {
			memcpy(ret.data() + offset, v.data(), v.size());
			offset += v.size();
		};

		Callback<void(StringView)> out(fn);

		while (!r.empty()) {
			auto tmp = r.readChars<StringView::Alphanumeric>();
			if (!tmp.empty()) {
				out << tmp;
			}
			while (!r.empty() && !r.is<StringView::Alphanumeric>()) {
				out << StringView("_");
				++ r;
			}
		}
		string::apply_tolower_c(ret);
		return ret;
	}

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
		case HtmlTag::Html:
		case HtmlTag::Head:
		case HtmlTag::Title:
		case HtmlTag::Body:
			break;
		case HtmlTag::Block:
		case HtmlTag::Image:
			if ((_htmlTag > 0 && _headTag == 0) || _bodyTag > 0 || (_htmlTag == 0 && _bodyTag == 0 && _headTag == 0)) {
				tag.node = new (memory::pool::acquire()) Node(tag.name);
			}
			break;
		}

		if (HtmlTag::isForceUnclosed(tag.name)) {
			tag.closable = false;
		}
	}

	inline void onEndTag(Parser &p, HtmlTag &tag, bool isClosable) {
		if (tag.node) {
			if ((tag.type == HtmlTag::Image || tag.node->getXType() == "image" || tag.name == "table") && tag.node->getHtmlId().empty()) {
				tag.node->setAttribute("id", string::toString<Interface>("__id__", _pseudoId, "__", encodePathString(page->getPath())));
				_pseudoId ++;
			}

			if (tag.node->getXType() == "image") {
				tag.node->setAttribute("href", string::toString<Interface>("#", tag.node->getHtmlId()));
			}

			if (tag.name == "img") {
				auto src = tag.node->getAttribute("src");
				if (!src.empty()) {
					page->addAsset(src);
				}
			}

			tag.node->finalize();
		}
	}

	inline void onTagAttribute(Parser &p, HtmlTag &tag, StringView &name, StringView &value) {
		switch (tag.type) {
		case HtmlTag::Base:
			if (isStringCaseEqual(name, "href")) {
				page->setBaseOrigin(value);
			} else if (isStringCaseEqual(name, "target")) {
				page->setBaseTarget(value);
			}
			break;
		default:
			break;
		}
		if (!tag.node) {
			return;
		}

		if (name == "style") {
			auto tmp = PageContainer::StringReader(value);
			page->readStyle(tag.node->getStyle(), tmp);
		} else {
			tag.node->setAttribute(name, value);
		}
	}

	inline void onTagAttributeList(Parser &p, HtmlTag &tag, StringView &data) {
		switch (tag.type) {
		case HtmlTag::Meta:
			data.trimChars<StringView::WhiteSpace>();
			page->setMeta(data);
			break;
		case HtmlTag::Link:
			data.trimChars<StringView::WhiteSpace>();
			page->addLink(data);
			break;
		default:
			//log::debug("onTagAttributeList", tag.name, ": ", data);
			break;
		}
	}

	inline void onPushTag(Parser &p, HtmlTag &tag) {
		switch (tag.type) {
		case HtmlTag::Html: ++ _htmlTag; break;
		case HtmlTag::Head: ++ _headTag; break;
		case HtmlTag::Body: ++ _bodyTag; break;
		default: break;
		}

		if (tag.node) {
			nodeStack.back()->pushNode(tag.node);
			nodeStack.emplace_back(tag.node);
		}
	}

	inline void onPopTag(Parser &p, HtmlTag &tag) {
		switch (tag.type) {
		case HtmlTag::Html: -- _htmlTag; break;
		case HtmlTag::Head: -- _headTag; break;
		case HtmlTag::Body: -- _bodyTag; break;
		default: break;
		}
		if (tag.node) {
			nodeStack.pop_back();
		}
	}

	inline void onInlineTag(Parser &p, HtmlTag &tag) {
		if (tag.node) {
			nodeStack.back()->pushNode(tag.node);
		}
	}

	inline void onTagContent(Parser &p, HtmlTag &tag, StringView &s) {
		switch (tag.type) {
		case HtmlTag::Title:
			s.trimChars<StringView::WhiteSpace>();
			page->setTitle(s);
			break;
		case HtmlTag::Style: {
			s.trimChars<StringView::WhiteSpace>();
			PageContainer::StringReader r(s);
			page->readStyle(r);
			break;
		}
		default:
			if (tag.node) {
				StringViewUtf8 str(s);
				str.trimChars<StringViewUtf8::WhiteSpace>();
				if (str.empty()) {
					if (!s.empty()) {
						tag.node->pushValue(StringView(" "));
					}
				} else {
					tag.node->pushValue(StringView(str));
				}
			}
			break;
		}
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
	if (isStringCaseEqual(tagName, "html")) {
		return HtmlTag::Html;
	} else if (isStringCaseEqual(tagName, "head")) {
		return HtmlTag::Head;
	} else if (isStringCaseEqual(tagName, "meta")) {
		return HtmlTag::Meta;
	} else if (isStringCaseEqual(tagName, "title")) {
		return HtmlTag::Title;
	} else if (isStringCaseEqual(tagName, "base")) {
		return HtmlTag::Base;
	} else if (isStringCaseEqual(tagName, "link")) {
		return HtmlTag::Link;
	} else if (isStringCaseEqual(tagName, "style")) {
		return HtmlTag::Style;
	} else if (isStringCaseEqual(tagName, "script")) {
		return HtmlTag::Script;
	} else if (isStringCaseEqual(tagName, "body")) {
		return HtmlTag::Body;
	} else if (tagName[0] == '!' || tagName[0] == '-' || isStringCaseEqual(tagName, "xml")) {
		return HtmlTag::Special;
	} else if (isStringCaseEqual(tagName, "img")) {
		return HtmlTag::Image;
	} else {
		return HtmlTag::Block;
	}
}

bool HtmlTag::isForceUnclosed(const StringView &tagName) {
	if (isStringCaseEqual(tagName, "br") || isStringCaseEqual(tagName, "hr") || isStringCaseEqual(tagName, "col")) {
		return true;
	}
	return false;
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
	auto page = new (_pool) PageContainer(_data);

	HtmlReader reader(page);

	html::parse<HtmlReader, StringView, HtmlTag>(reader,
			StringView(reinterpret_cast<const char *>(data.data()), data.size()), false);

	page->finalize();

	_data->pages.emplace(StringView("/"), page);
	_data->spine.emplace_back(StringView("/"));

	return true;
}

}
