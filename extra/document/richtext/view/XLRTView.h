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

#ifndef EXTRA_DOCUMENT_RICHTEXT_VIEW_XLRTVIEW_H_
#define EXTRA_DOCUMENT_RICHTEXT_VIEW_XLRTVIEW_H_

#include "XLRTCommonSource.h"
#include "XLRTListenerView.h"
#include "XL2dLinearProgress.h"
#include "XLEventHeader.h"
#include "MaterialLabel.h"

namespace STAPPLER_VERSIONIZED stappler::xenolith::richtext {

class SP_PUBLIC View : public ListenerView {
public:
	static EventHeader onImageLink;
	static EventHeader onContentLink;
	static EventHeader onError;
	static EventHeader onDocument;
	static EventHeader onLayout;

	using PositionCallback = std::function<void(float)>;
	using PageData = document::LayoutPageData;

	struct ViewPosition {
		size_t object;
		float position;
		float scroll;
	};

	/*class PageWithLabel : public Page {
	public:
		virtual bool init(const PageData &, float) override;
		virtual void onContentSizeDirty() override;

	protected:
		material::Label *_label;
	};*/

	class Highlight : public basic2d::Sprite {
	public:
		virtual bool init(View *);
		virtual bool visitDraw(FrameInfo &, NodeFlags parentFlags) override;

		virtual void clearSelection();
		virtual void addSelection(const Pair<SelectionPosition, SelectionPosition> &);

		virtual void setDirty();
	protected:
		virtual void emplaceRect(const Rect &, size_t idx, size_t count);
		virtual void updateVertexes(FrameInfo &frame) override;

		View *_view = nullptr;
		Vector<Pair<SelectionPosition, SelectionPosition>> _selectionBounds;

		bool _enabled = false;
	};

	virtual ~View();

	virtual bool init(CommonSource * = nullptr);
	virtual bool init(Layout, CommonSource *);
	virtual void handleContentSizeDirty() override;
	virtual void setLayout(Layout l) override;

	virtual void setProgressColor(const Color &color);

	virtual void setOverscrollFrontOffset(float value) override;
	virtual void setSource(CommonSource *) override;

	virtual float getViewContentPosition(float = nan()) const;
	virtual ViewPosition getViewObjectPosition(float) const;

	virtual ViewPosition getViewPosition() const;
	virtual void setViewPosition(const ViewPosition &, bool offseted = false);

	virtual void setPositionCallback(const PositionCallback &);
	virtual const PositionCallback &getPositionCallback() const;

	virtual void onContentFile(const StringView &);

	virtual void setRenderingEnabled(bool);

	virtual void clearHighlight();
	virtual void addHighlight(const Pair<SelectionPosition, SelectionPosition> &);
	virtual void addHighlight(const SelectionPosition &, const SelectionPosition &);

	virtual float getBookmarkScrollPosition(size_t, uint32_t, bool inView) const;
	virtual float getBookmarkScrollRelativePosition(size_t, uint32_t, bool inView) const;

protected:
	virtual void onPosition() override;

	virtual void onRestorePosition(document::LayoutResult *res, float pos);
	virtual void onRenderer(RendererResult *, bool) override;

	virtual void onLink(const StringView &ref, const StringView &target, const WideStringView &text, const Vec2 &) override;
	virtual void onId(const StringView &ref, const StringView &target, const WideStringView &text, const Vec2 &);
	virtual void onImage(const StringView &id, const Vec2 &);
	virtual void onGallery(const StringView &name, const StringView &image, const Vec2 &);
	virtual void onFile(const StringView &, const Vec2 &);
	virtual void onPositionRef(const StringView &, bool middle);

	virtual void onFigure(const document::Node *node);
	virtual void onImageFigure(const StringView &src, const StringView &alt, const document::Node *);
	virtual void onVideoFigure(const StringView &src);
	virtual void onTable(const StringView &);

	virtual void onSourceError(CommonSource::Error);
	virtual void onSourceUpdate();
	virtual void onSourceAsset();

	virtual void updateProgress();

	//virtual Rc<Page> onConstructPageNode(const PageData &, float) override;

	EventListener *_eventListener = nullptr;

	float _savedFontScale = nan();
	Size2 _renderSize;
	ViewPosition _savedPosition;
	bool _layoutChanged = false;
	bool _rendererUpdate = false;
	bool _renderingEnabled = true;
	EventHandlerNode *_sourceErrorListener = nullptr;
	EventHandlerNode *_sourceUpdateListener = nullptr;
	EventHandlerNode *_sourceAssetListener = nullptr;
	material2d::LinearProgress *_progress = nullptr;
	Tooltip *_tooltip = nullptr;
	PositionCallback _positionCallback = nullptr;
	Highlight *_highlight = nullptr;
};

}

#endif /* EXTRA_DOCUMENT_RICHTEXT_VIEW_XLRTVIEW_H_ */
