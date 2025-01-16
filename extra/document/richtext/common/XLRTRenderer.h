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

#ifndef EXTRA_DOCUMENT_RICHTEXT_COMMON_XLRTRENDERER_H_
#define EXTRA_DOCUMENT_RICHTEXT_COMMON_XLRTRENDERER_H_

#include "XLRichText.h"
#include "XLSubscriptionListener.h"
#include "XLFontController.h"

namespace STAPPLER_VERSIONIZED stappler::xenolith::richtext {

struct SP_PUBLIC RendererResource : public Ref {
	~RendererResource();

	Rc<ResourceCache> cache;
	Set<String> textures;
	Map<String, String> svgs;
	Rc<TemporaryResource> resource;
};

struct SP_PUBLIC RendererResult : public Ref {
	Rc<Application> app;
	Rc<font::FontController> fc;
	Rc<Document> document;
	Rc<CommonSource> source;
	Rc<document::LayoutResult> result;
	Rc<font::HyphenMap> hyphen;
	Rc<RendererResource> resource;
	Vector<Rc<AssetLock>> locks;
	MediaParameters media;
	Vector<String> ids;
	Map<String, DocumentAssetMeta> assets;
	Time ctime;

	RendererResult() = default;
	RendererResult(const RendererResult &);
	RendererResult& operator=(const RendererResult &);
};

class SP_PUBLIC Renderer : public Component {
public:
	using RenderingCallback = Function<void(RendererResult *, bool started)>;

	static bool shouldSplitPages(const Size2 &);

	virtual ~Renderer();

	virtual bool init(const Vector<String> &ids = {});
	virtual void handleContentSizeDirty() override;
	virtual void visitSelf(FrameInfo &, NodeFlags parentFlags) override;

	virtual void update(const UpdateTime &time) override;

	virtual void setSource(CommonSource *source);
	virtual CommonSource *getSource() const;

	Document *getDocument() const;
	RendererResult *getResult() const;
	MediaParameters getMedia() const;

	void setSurfaceSize(const Size2 &size);

	// size of rendering surface (size for media-queries)
	const Size2 &getSurfaceSize() const;
	const Padding & getPageMargin() const;
	bool isPageSplitted() const;

	void setDpi(int dpi);
	void setDensity(float density);
	void setDefaultBackground(const Color4B &);

	void setFontScale(float);

	void setMediaType(document::MediaType value);
	void setOrientationValue(document::Orientation value);
	void setPointerValue(document::Pointer value);
	void setHoverValue(document::Hover value);
	void setLightLevelValue(document::LightLevel value);
	void setScriptingValue(document::Scripting value);

	void setPageMargin(const Padding &);

	void addOption(const StringView &);
	void removeOption(const StringView &);
	bool hasOption(const StringView &) const;

	void addFlag(document::RenderFlags flag);
	void removeFlag(document::RenderFlags flag);
	bool hasFlag(document::RenderFlags flag) const;

	StringView getLegacyBackground(const document::Node &, const StringView &opt) const;

	virtual void setRenderingCallback(const RenderingCallback &);
	virtual void onResult(RendererResult *result);

protected:
	virtual Rc<core::Resource> prepareResource(RendererResource *res, StringView name, Time ctime, const Map<String, DocumentAssetMeta> &);
	virtual bool requestRendering();
	virtual void onSource();
	virtual void pushVersionOptions();

	virtual bool isAssetsSame(const Map<String, DocumentAssetMeta> &, const Map<String, DocumentAssetMeta> &);

	bool _renderingDirty = false;
	bool _renderingInProgress = false;
	bool _isPageSplitted = false;

	float _fontScale = 1.0f;

	Padding _pageMargin;
	Vector<String> _ids;
	Size2 _surfaceSize;
	MediaParameters _media;
	Rc<CommonSource> _source;
	Rc<RendererResult> _result;
	RenderingCallback _renderingCallback = nullptr;
};

}

#endif /* EXTRA_DOCUMENT_RICHTEXT_COMMON_XLRTRENDERER_H_ */
