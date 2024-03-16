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

#include "XLRTImageLayout.h"
#include "XLRTRenderer.h"
#include "XLTemporaryResource.h"
#include "XL2dSceneContent.h"

namespace STAPPLER_VERSIONIZED stappler::xenolith::richtext {

ImageLayout::~ImageLayout() { }

bool ImageLayout::init(RendererResult *res, const StringView &id, const StringView &src, const StringView &alt) {
	if (!SceneLayout2d::init() || !res) {
		return false;
	}

	_result = res;
	_src = src.str<Interface>();

	auto tooltip = constructTooltip(_result, id.empty()?Vector<String>():Vector<String>{id.str<Interface>()});

	if (auto r = tooltip->getRenderer()) {
		r->addOption("image-view");
	}

	_tooltip = addChild(move(tooltip), ZOrder(2));

	auto actions = _tooltip->getActions();
	actions->clear();
	if (!id.empty()) {
		_expandButton = actions->addButton("Expand", IconName::Navigation_expand_less_solid, std::bind(&ImageLayout::onExpand, this));
	} else {
		_expanded = false;
	}

	auto toolbar = _tooltip->getToolbar();
	toolbar->setTitle(alt);
	toolbar->setNavButtonIcon(IconName::Dynamic_Nav, 1.0f);
	toolbar->setNavCallback(std::bind(&ImageLayout::close, this));
	toolbar->setSwallowTouches(true);

	_sprite = addChild(Rc<material2d::ImageLayer>::create(), ZOrder(1));
	_sprite->setPosition(Vec2(0, 0));
	_sprite->setAnchorPoint(Vec2(0, 0));
	_sprite->setActionCallback([this] {
		if (!_expanded) {
			_tooltip->onFadeOut();
		}
	});

	if (auto tex = _result->resource->resource->acquireTexture(_src)) {
		_sprite->setTexture(move(tex));
	}

	return true;
}

void ImageLayout::onContentSizeDirty() {
	SceneLayout2d::onContentSizeDirty();

	if (_contentSize != Size2::ZERO) {
		auto incr = 48.0f;
		auto size = _contentSize;
		auto pos = Vec2(incr / 4.0f, incr / 2.0f);

		Size2 maxSize;
		maxSize.width = _contentSize.width - incr;
		maxSize.height = _contentSize.height - incr * 2.0f;

		if (maxSize.width > incr * 9) {
			maxSize.width = incr * 9;
		}

		if (maxSize.height > incr * 9) {
			maxSize.height = incr * 9;
		}

		size.width -= incr;

		_tooltip->setOriginPosition(Vec2(0, incr / 2.0f), size, _parent->convertToWorldSpace(pos));
		_tooltip->setMaxContentSize(maxSize);
		_tooltip->setPosition(pos);

		if (_tooltip->getRenderer()) {
			_tooltip->setMaxContentSize(maxSize);
		} else {
			//_tooltip->setContentSize(Size(maxSize.width, _tooltip->getToolbar()->getContentSize().height));
			_tooltip->setContentSize(Size2(maxSize.width, 28.0f));
		}
	}

	_sprite->setContentSize(_contentSize);
}

void ImageLayout::onEnter(Scene *scene) {
	SceneLayout2d::onEnter(scene);
}

void ImageLayout::close() {
	auto contentLayer = dynamic_cast<material2d::SceneContent2d *>(_parent);
	if (contentLayer) {
		contentLayer->popLayout(this);
	}
}

Rc<Tooltip> ImageLayout::constructTooltip(RendererResult *res, const Vector<String> &ids) const {
	return Rc<Tooltip>::create(res, ids, WideStringView());
}

void ImageLayout::onExpand() {
	if (_expanded) {
		_tooltip->setExpanded(false);
		if (_expandButton) {
			_expandButton->setNameIcon(IconName::Navigation_expand_more_solid);
		}
		_expanded = false;
	} else {
		_tooltip->setExpanded(true);
		if (_expandButton) {
			_expandButton->setNameIcon(IconName::Navigation_expand_less_solid);
		}
		_expanded = true;
	}
}

}
