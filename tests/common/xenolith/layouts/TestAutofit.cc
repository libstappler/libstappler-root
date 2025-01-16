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

#include "TestAutofit.h"
#include "XL2dVectorSprite.h"
#include "XLInputListener.h"
#include "XLIcons.h"

namespace STAPPLER_VERSIONIZED stappler::xenolith::app {

class GeneralAutofitTestResize : public VectorSprite {
public:
	virtual ~GeneralAutofitTestResize() { }

	virtual bool init() override;

protected:
	using VectorSprite::init;
};

class GeneralAutofitTestNode : public Node {
public:
	virtual ~GeneralAutofitTestNode() { }

	virtual bool init() override;

	virtual void onContentSizeDirty() override;

protected:
	Size2 _targetSize;
	Layer *_background = nullptr;
	Layer *_layers[5] = { nullptr };
	Sprite *_sprites[5] = { nullptr };
	Label *_labels[5] = { nullptr };
};

bool GeneralAutofitTestNode::init() {
	if (!Node::init()) {
		return false;
	}

	_background = addChild(Rc<Layer>::create(Color::Red_50));
	_background->setAnchorPoint(Anchor::Middle);

	for (size_t i = 0; i < 5; ++ i) {
		_layers[i] = addChild(Rc<Layer>::create(Color::Teal_500), ZOrder(1));
		_layers[i]->setAnchorPoint(Anchor::Middle);

		_sprites[i] = addChild(Rc<Sprite>::create("xenolith-2-480.png"), ZOrder(2));
		_sprites[i]->setAnchorPoint(Anchor::Middle);
		_sprites[i]->setTextureAutofit(Autofit(i));

		_labels[i] = addChild(Rc<Label>::create(), ZOrder(3));
		_labels[i]->setAnchorPoint(Anchor::MiddleBottom);
		_labels[i]->setColor(Color::Red_500, true);
		_labels[i]->setFontSize(20);
		_labels[i]->setOpacity(0.75);

		switch (Autofit(i)) {
		case Autofit::None: _labels[i]->setString("Autofit::None"); break;
		case Autofit::Width: _labels[i]->setString("Autofit::Width"); break;
		case Autofit::Height: _labels[i]->setString("Autofit::Height"); break;
		case Autofit::Cover: _labels[i]->setString("Autofit::Cover"); break;
		case Autofit::Contain: _labels[i]->setString("Autofit::Contain"); break;
		}
	}

	return true;
}

void GeneralAutofitTestNode::onContentSizeDirty() {
	Node::onContentSizeDirty();

	if (_background) {
		_background->setContentSize(_contentSize);
		_background->setPosition(_contentSize / 2.0f);
	}

	Size2 size(_contentSize * 0.3f);

	Vec2 positions[5] = {
		Vec2(_contentSize.width * 0.2f, _contentSize.height * 0.2f),
		Vec2(_contentSize.width * 0.2f, _contentSize.height * 0.8f),
		Vec2(_contentSize.width * 0.5f, _contentSize.height * 0.5f),
		Vec2(_contentSize.width * 0.8f, _contentSize.height * 0.2f),
		Vec2(_contentSize.width * 0.8f, _contentSize.height * 0.8f),
	};

	for (size_t i = 0; i < 5; ++ i) {
		if (_sprites[i]) {
			_sprites[i]->setContentSize(size);
			_sprites[i]->setPosition(positions[i]);
		}

		if (_layers[i]) {
			_layers[i]->setContentSize(size);
			_layers[i]->setPosition(positions[i]);
		}

		if (_labels[i]) {
			_labels[i]->setPosition(positions[i] + Vec2(0.0f, _contentSize.height * 0.15f + 10));
		}
	}
}

bool GeneralAutofitTestResize::init() {
	auto image = Rc<VectorImage>::create(Size2(24, 24));

	getIconData(IconName::Navigation_unfold_more_solid, [&] (BytesView view) {
		image->addPath("", "org.stappler.xenolith.test.GeneralAutofitTestResize.Resize")->setPath(view)
				.openForWriting([] (vg::PathWriter &writer) { writer.addOval(Rect(0, 0, 24, 24)); })
				.setWindingRule(vg::Winding::EvenOdd).setFillColor(Color::White);
	});

	return VectorSprite::init(move(image));
}

bool TestAutofit::init() {
	if (!TestLayout::init(LayoutName::Autofit, "")) {
		return false;
	}

	_nodeAutofit = addChild(Rc<GeneralAutofitTestNode>::create());
	_nodeAutofit->setAnchorPoint(Anchor::Middle);

	_nodeResize = addChild(Rc<GeneralAutofitTestResize>::create(), ZOrder(1));
	_nodeResize->setAnchorPoint(Anchor::Middle);
	_nodeResize->setColor(Color::Grey_400);
	_nodeResize->setContentSize(Size2(48, 48));
	_nodeResize->setRotation(-45.0_to_rad);

	auto l = _nodeResize->addInputListener(Rc<InputListener>::create());
	l->addMouseOverRecognizer([this] (const GestureData &data) {
		switch (data.event) {
		case GestureEvent::Began:
			_nodeResize->setColor(Color::Grey_600);
			break;
		default:
			_nodeResize->setColor(Color::Grey_400);
			break;
		}
		return true;
	});
	l->addSwipeRecognizer([this] (const GestureSwipe &swipe) {
		if (swipe.event == GestureEvent::Activated) {
			auto tmp = _contentSize * 0.90f * 0.5f;
			auto max = Vec2(_contentSize / 2.0f) + Vec2(tmp.width, -tmp.height);
			auto min = Vec2(_contentSize / 2.0f) + Vec2(32.0f, -32.0f);
			auto pos = _nodeResize->getPosition().xy();
			auto newPos = pos + swipe.delta / swipe.density;

			if (newPos.x < min.x) {
				newPos.x = min.x;
			}
			if (newPos.x > max.x) {
				newPos.x = max.x;
			}
			if (newPos.y > min.y) {
				newPos.y = min.y;
			}
			if (newPos.y < max.y) {
				newPos.y = max.y;
			}
			_nodeResize->setPosition(newPos);

			auto newContentSize = Size2(newPos.x - _contentSize.width / 2.0f, _contentSize.height / 2.0f - newPos.y);
			_nodeAutofit->setContentSize(newContentSize * 2.0f);
		}

		return true;
	});

	return true;
}

void TestAutofit::onContentSizeDirty() {
	TestLayout::onContentSizeDirty();

	_nodeAutofit->setPosition(_contentSize / 2.0f);
	_nodeAutofit->setContentSize(_contentSize * 0.90f);

	auto tmp = _nodeAutofit->getContentSize() / 2.0f;

	_nodeResize->setPosition(Vec2(_contentSize / 2.0f) + Vec2(tmp.width, -tmp.height));
}

void TestAutofit::handleEnter(xenolith::Scene *scene) {
	TestLayout::handleEnter(scene);
}

}
