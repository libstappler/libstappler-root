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

#include "SPDocPageContainer.h"

namespace STAPPLER_VERSIONIZED stappler::document {

PageContainer::PageContainer(DocumentData *doc)
: StyleContainer(doc) {
	_root = new (memory::pool::acquire()) Node(StringView("body"));
}

void PageContainer::finalize() {
	_root->foreach([&] (Node &node, size_t level) {
		/*for (size_t i = 0; i < level; ++ i) {
			std::cout << "\t";
		}
		std::cout << node.getHtmlName() << ": " << string::toUtf8<Interface>(node.getValue()) << "\n";*/
		node.setNodeId(_document->maxNodeId ++);
		if (!node.getHtmlId().empty()) {
			_ids.emplace(node.getHtmlId(), &node);
		}
	});
}

void PageContainer::setTitle(const StringView &data) {
	_title = string::decodeHtml<Interface>(data);
}

void PageContainer::setMeta(const StringView &data) {
	bool isHttp = false;
	String name;

	StringReader attrStart(data);
	String attrName;
	String attrValue;
	while (!attrStart.empty()) {
		attrName.clear();
		attrValue.clear();

		attrName = parser::readHtmlTagParamName(attrStart);
		if (attrName.empty()) {
			continue;
		}

		attrValue = parser::readHtmlTagParamValue(attrStart);

		if (isStringCaseEqual(attrName, "charset")) {
			_charset = move(attrValue);
		} else if (isStringCaseEqual(attrName, "http-equiv")) {
			name = move(attrValue);
			isHttp = true;
		} else if (isStringCaseEqual(attrName, "name")) {
			name = move(attrValue);
		} else if (isStringCaseEqual(attrName, "content")) {
			if (!name.empty() && !attrValue.empty()) {
				if (isHttp) {
					std::cout << "Http: " << name << ": " << attrValue << "\n";
					_http.emplace(move(name), move(attrValue));
				} else {
					std::cout << "Meta: " << name << ": " << attrValue << "\n";
					_meta.emplace(move(name), move(attrValue));
				}
			}
		}
	}
}

void PageContainer::setBaseOrigin(const StringView &val) {
	_baseOrigin = val.str<Interface>();
}

void PageContainer::setBaseTarget(const StringView &val) {
	_baseTarget = val.str<Interface>();
}

void PageContainer::addLink(const StringView &data) {
	String rel;
	String href;
	String media;

	StringReader attrStart(data);
	String attrName;
	String attrValue;
	while (!attrStart.empty()) {
		attrName.clear();
		attrValue.clear();

		attrName = parser::readHtmlTagParamName(attrStart);
		if (attrName.empty()) {
			continue;
		}

		attrValue = parser::readHtmlTagParamValue(attrStart);

		if (isStringCaseEqual(attrName, "href")) {
			href = move(attrValue);
		} else if (isStringCaseEqual(attrName, "rel")) {
			rel = move(attrValue);
		} else if (isStringCaseEqual(attrName, "media")) {
			media = move(attrValue);
		}
	}

	if (isStringCaseEqual(rel, "stylesheet") && !href.empty()) {
		MediaQueryId mediaId = MediaQueryIdNone;
		if (!media.empty()) {
			StyleBuffers buffers;
			StringReader r(media);
			mediaId = _document->addQuery(MediaQuery{ readMediaQueryList(buffers, r) });
		}

		_assets.emplace_back(href);
		_styleLinks.emplace_back(StyleLink{move(href), mediaId});
	}
}

void PageContainer::addAsset(const StringView &str) {
	StringView r(str);
	auto url = r.readUntil<StringView::Chars<'?'>>();

	_assets.emplace_back(url.str<Interface>());
}

StringView PageContainer::getMeta(StringView key) const {
	auto it = _meta.find(key);
	if (it != _meta.end()) {
		return it->second;
	}
	return StringView();
}

Node *PageContainer::getNodeById(StringView key) const {
	auto it = _ids.find(key);
	if (it != _ids.end()) {
		return it->second;
	}
	return nullptr;
}

}
