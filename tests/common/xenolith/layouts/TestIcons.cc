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

#include "TestIcons.h"
#include "XLIcons.h"
#include "XL2dLabel.h"

namespace STAPPLER_VERSIONIZED stappler::xenolith::app {

class VgIconListNode : public Node {
public:
	virtual ~VgIconListNode() { }

	virtual bool init(IconName, Function<void(IconName)> &&);

	virtual void onContentSizeDirty() override;

protected:
	using Node::init;

	VectorSprite *_image = nullptr;
	IconName _iconName = IconName::Empty;
	Function<void(IconName)> _callback;
};

bool VgIconListNode::init(IconName iconName, Function<void(IconName)> &&cb) {
	if (!Node::init()) {
		return false;
	}

	auto image = Rc<VectorImage>::create(Size2(24, 24));

	auto path = image->addPath();
	getIconData(iconName, [&] (BytesView bytes) {
		path->getPath()->init(bytes);
	});
	path->setWindingRule(vg::Winding::EvenOdd);
	path->setAntialiased(false);
	auto t = Mat4::IDENTITY;
	t.scale(1, -1, 1);
	t.translate(0, -24, 0);
	path->setTransform(t);

	_image = addChild(Rc<VectorSprite>::create(move(image)));
	_image->setColor(Color::Black);
	_image->setAutofit(Sprite::Autofit::Contain);
	_image->setContentSize(Size2(64.0f, 64.0f));
	_image->setAnchorPoint(Anchor::Middle);
	_image->setWaitDeferred(false);
	//_image->setDeferred(true);
	_iconName = iconName;

	_callback = move(cb);

	setTag(toInt(iconName));

	return true;
}

void VgIconListNode::onContentSizeDirty() {
	Node::onContentSizeDirty();

	_image->setPosition(_contentSize / 2.0f);
}

bool TestIcons::init()  {
	if (!TestLayout::init(LayoutName::IconList, "")) {
		return false;
	}

	_scrollView = addChild(Rc<ScrollView>::create(ScrollView::Vertical));
	_scrollView->setAnchorPoint(Anchor::MiddleTop);
	_scrollView->setIndicatorColor(Color::Grey_500);
	auto controller = _scrollView->setController(Rc<ScrollController>::create());

	for (uint32_t i = toInt(IconName::Action_3d_rotation_outline); i < toInt(IconName::Max); ++ i) {
		controller->addItem([i] (const ScrollController::Item &) -> Rc<Node> {
			return Rc<VgIconListNode>::create(IconName(i), [] (IconName name) { });
		}, 72.0f, 0);
	}

	controller->setRebuildCallback([this] (ScrollController *c) {
		return rebuildScroll(c);
	});

	return true;
}

void TestIcons::onContentSizeDirty() {
	TestLayout::onContentSizeDirty();

	_scrollView->setPosition(Vec2(_contentSize.width / 2.0f, _contentSize.height));
	_scrollView->setContentSize(_contentSize);
	_scrollView->disableScissor();
}

void TestIcons::onEnter(xenolith::Scene *scene) {
	TestLayout::onEnter(scene);

	runAction(Rc<ActionProgress>::create(0.5f, [this] (float val) {
		_scrollView->setScrollRelativePosition(val);
	}));
}

bool TestIcons::rebuildScroll(ScrollController *c) {
	uint32_t nCols = uint32_t(floorf(_contentSize.width / 72.0f));

	uint32_t idx = 0;

	float xOffset = (_contentSize.width - nCols * 72.0f) / 2.0f;
	float xPos = xOffset;
	float yPos = 0.0f;
	for (auto &it : c->getItems()) {
		if (idx >= nCols) {
			idx = 0;
			xPos = xOffset;
			yPos += 72.0f;
		}

		it.size = Size2(72.0f, 72.0f);
		it.pos = Vec2(xPos, yPos);
		xPos += 72.0f;
		++ idx;
	}

	if (_nCols != nCols) {
		_nCols = nCols;
		return true;
	}

	return false;
}

}
