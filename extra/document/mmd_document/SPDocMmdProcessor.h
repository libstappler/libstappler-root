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

#ifndef MMD_LAYOUT_MMDLAYOUTPROCESSOR_H_
#define MMD_LAYOUT_MMDLAYOUTPROCESSOR_H_

#include "SPDocMmd.h"
#include "MMDHtmlProcessor.h"

namespace STAPPLER_VERSIONIZED stappler::mmd {

class DocumentProcessor : public HtmlProcessor {
public:
	using Page = document::PageContainer;

	virtual ~DocumentProcessor() { }

	virtual bool init(document::DocumentMmd *, document::DocumentData *data);

protected:
	template <typename T>
	friend void LayoutProcessor_processAttr(DocumentProcessor &p, const T &container, const StringView &name,
			document::StyleList &style, const Callback<void(StringView, StringView)> &cb);

	virtual void processHtml(const Content &, const StringView &, const Token &);

	void processStyle(const StringView &name, document::StyleList &, const StringView &);

	document::Node *makeNode(const StringView &name, InitList &&attr, VecList &&);

	virtual void pushNode(token *, const StringView &name, InitList &&attr = InitList(), VecList && = VecList());
	virtual void pushInlineNode(token *, const StringView &name, InitList &&attr = InitList(), VecList && = VecList());
	virtual void popNode();
	virtual void flushBuffer();

	Vector<document::Node *> _nodeStack;
	document::DocumentMmd *_document = nullptr;
	document::DocumentData *_data = nullptr;
	Page *_page = nullptr;
	uint32_t _tableIdx = 0;
};

}

#endif /* MMD_LAYOUT_MMDLAYOUTPROCESSOR_H_ */
