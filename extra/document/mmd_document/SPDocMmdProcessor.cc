// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2017 Roman Katuntsev <sbkarr@stappler.org>

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

#include "SPDocMmdProcessor.h"
#include "MMDToken.h"
#include "MMDCore.h"
#include "SPDocMmd.h"
#include "SPDocPageContainer.h"

namespace STAPPLER_VERSIONIZED stappler::mmd {

bool DocumentProcessor::init(document::DocumentMmd *doc, document::DocumentData *data) {
	_document = doc;
	_data = data;
	_page = _document->acquireRootPage();
	_nodeStack.push_back(_page->getRoot());

	return true;
}

void DocumentProcessor::processHtml(const Content &c, const StringView &str, const Token &t) {
	source = str;
	exportTokenTree(buffer, t);
	exportFootnoteList(buffer);
	exportCitationList(buffer);
	flushBuffer();

	token * entry = nullptr;
	uint8_t init_level = 0, level = 0;

	std::array<document::DocumentContentRecord *, 8> level_list;

	for (auto &it : level_list) {
		it = nullptr;
	}

	auto &header_stack = content->getHeaders();
	if (header_stack.empty()) {
		_page->finalize();
		return;
	}

	init_level = rawLevelForHeader(header_stack.front());
	level_list[init_level - 1] = &_data->tableOfContents;

	memory::pool::context ctx(_data->pool);

	for (auto &it : header_stack) {
		entry = it;
		level = rawLevelForHeader(it);
		if (auto c = level_list[level - 1]) {
			buffer.clear();
			exportTokenTree(buffer, entry->child);
			auto str = buffer.weak();

			auto id = labelFromHeader(source, entry);

			c->childs.push_back(document::DocumentContentRecord{
				StringView(str.data(), str.size()).pdup(_data->pool),
				StringView(id.data(), id.size()).pdup(_data->pool)});
			level_list[level] = &c->childs.back();
		}
	}

	_page->finalize();
}

void DocumentProcessor::processStyle(const StringView &name, document::StyleList &style, const StringView &styleData) {
	auto tmp = document::PageContainer::StringReader(styleData);
	_page->readStyle(style, tmp);
}

template <typename T>
void LayoutProcessor_processAttr(DocumentProcessor &p, const T &container, const StringView &name,
		document::StyleList &style, const Callback<void(StringView, StringView)> &cb) {
	for (auto &it : container) {
		if (it.first == "style") {
			p.processStyle(name, style, it.second);
			cb(it.first, it.second);
		} else if (name == "img" && it.first == "src") {
			p._page->addAsset(it.second);
			cb(it.first, it.second);
		} else {
			cb(it.first, it.second);
		}
	}
}

document::Node *DocumentProcessor::makeNode(const StringView &name, InitList &&attr, VecList &&vec) {
	auto node = new (_data->pool) document::Node(name);

	LayoutProcessor_processAttr(*this, attr, name, node->getStyle(), [&] (StringView key, StringView value) {
		node->setAttribute(key, value);
	});
	LayoutProcessor_processAttr(*this, vec, name, node->getStyle(), [&] (StringView key, StringView value) {
		node->setAttribute(key, value);
	});

	if (name == "table" && node->getHtmlId().empty()) {
		++ _tableIdx;
		node->setAttribute("id", string::toString<memory::PoolInterface>("__table:", _tableIdx));
	}

	return _nodeStack.back()->pushNode(node);
}

void DocumentProcessor::pushNode(token *, const StringView &name, InitList &&attr, VecList &&vec) {
	memory::pool::context ctx(_data->pool);
	flushBuffer();
	auto node = makeNode(name, move(attr), move(vec));
	_nodeStack.push_back(node);
}

void DocumentProcessor::pushInlineNode(token *, const StringView &name, InitList &&attr, VecList &&vec) {
	memory::pool::context ctx(_data->pool);
	flushBuffer();
	makeNode(name, move(attr), move(vec));
}

void DocumentProcessor::popNode() {
	memory::pool::context ctx(_data->pool);
	flushBuffer();
	_nodeStack.pop_back();
}

void DocumentProcessor::flushBuffer() {
	memory::pool::context ctx(_data->pool);
	auto str = buffer.str();
	StringView r(str);
	if (!r.empty()) {
		r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
		if (r.empty()) {
			auto b = _nodeStack.back();
			if (b->hasValue()) {
				b->pushValue(" ");
			}
		} else {
			r = StringView(str);
			if (r.back() == '\n') {
				size_t s = r.size();
				while (s > 0 && (r[s - 1] == '\n' || r[s - 1] == '\r')) {
					-- s;
				}
				r = StringView(r.data(), s);
			}
			_nodeStack.back()->pushValue(string::toUtf16Html<memory::PoolInterface>(r));
		}
	}
	buffer.clear();
}

}
