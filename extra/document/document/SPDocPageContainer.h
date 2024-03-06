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

#ifndef EXTRA_DOCUMENT_DOCUMENT_SPDOCUMENTPAGECONTAINER_H_
#define EXTRA_DOCUMENT_DOCUMENT_SPDOCUMENTPAGECONTAINER_H_

#include "SPDocStyleContainer.h"
#include "SPDocNode.h"

namespace STAPPLER_VERSIONIZED stappler::document {

class PageContainer : public StyleContainer {
public:
	struct StyleLink {
		String href;
		MediaQueryId media = MediaQueryIdNone;
	};

	virtual ~PageContainer() = default;

	PageContainer(DocumentData *);

	virtual void finalize();

	void setTitle(const StringView &);
	void setMeta(const StringView &);
	void setBaseOrigin(const StringView &);
	void setBaseTarget(const StringView &);
	void addLink(const StringView &);

	void addAsset(const StringView &);

	Node *getRoot() const { return _root; }
	StringView getPath() const { return _path; }

	StringView getMeta(StringView) const;

	SpanView<StyleLink> getStyleLinks() const { return _styleLinks; }

	SpanView<String> getAssets() const { return _assets; }

	Node *getNodeById(StringView) const;

protected:
	String _path;
	String _title;
	String _charset;
	String _baseOrigin;
	String _baseTarget;
	Map<String, String> _meta;
	Map<String, String> _http;

	Node *_root = nullptr;

	bool linear = true;

	Vector<StyleLink> _styleLinks;
	Vector<String> _assets;
	Map<StringView, Node *> _ids;
};

}

#endif /* EXTRA_DOCUMENT_DOCUMENT_SPDOCUMENTPAGECONTAINER_H_ */
