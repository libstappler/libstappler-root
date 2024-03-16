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

#ifndef MMD_LAYOUT_MMDLAYOUTDOCUMENT_H_
#define MMD_LAYOUT_MMDLAYOUTDOCUMENT_H_

#include "MMDCommon.h"
#include "SPDocument.h"

namespace STAPPLER_VERSIONIZED stappler::document {

class DocumentMmd : public Document {
public:
	static bool isMmd(FilePath);
	static bool isMmd(BytesView);

	virtual ~DocumentMmd() = default;

	virtual bool init(FilePath, StringView ct = StringView());
	virtual bool init(BytesView, StringView ct = StringView());
	virtual bool init(memory::pool_t *, FilePath, StringView ct = StringView());
	virtual bool init(memory::pool_t *, BytesView, StringView ct = StringView());

	// Default style, that can be redefined with css
	virtual void beginStyle(StyleList &, const Node &, SpanView<const Node *>, const MediaParameters &) const override;

	// Default style, that can NOT be redefined with css
	virtual void endStyle(StyleList &, const Node &, SpanView<const Node *>, const MediaParameters &) const override;

	PageContainer *acquireRootPage();

protected:
	friend class LayoutProcessor;

	void onTag(StyleList &style, StringView tag, StringView parent, const MediaParameters &media) const;

	virtual bool read(BytesView, StringView ct);

	MediaQueryId _minWidthQuery;
	MediaQueryId _mediumWidthQuery;
	MediaQueryId _maxWidthQuery;
	MediaQueryId _imageViewQuery;
	StringId _monospaceId;
};

}

#endif /* MMD_LAYOUT_MMDLAYOUTDOCUMENT_H_ */
