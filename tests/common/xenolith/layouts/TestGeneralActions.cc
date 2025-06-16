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

#include "TestGeneralActions.h"
#include "XLInterpolation.h"

namespace stappler::xenolith::app {

bool ActionEaseNode::init(StringView str,
		Function<Rc<ActionInterval>(Rc<ActionInterval> &&)> &&cb) {
	if (!Node::init()) {
		return false;
	}

	_label = addChild(Rc<Label>::create());
	_label->setString(str);
	_label->setAlignment(Label::TextAlign::Right);
	_label->setAnchorPoint(Anchor::MiddleRight);
	_label->setFontSize(20);

	_layer = addChild(Rc<Layer>::create(Color::Red_500));
	_layer->setAnchorPoint(Anchor::BottomLeft);
	_layer->setContentSize(Size2(48.0f, 48.0f));

	_callback = sp::move(cb);

	return true;
}

void ActionEaseNode::handleContentSizeDirty() {
	Node::handleContentSizeDirty();

	_label->setPosition(Vec2(-4.0f, _contentSize.height / 2.0f));
	_layer->setContentSize(Size2(48.0f, _contentSize.height));
}

void ActionEaseNode::run() {
	_layer->stopAllActions();

	auto progress = _layer->getPosition().x / (_contentSize.width - _layer->getContentSize().width);
	if (progress < 0.5f) {
		auto a = _callback(Rc<MoveTo>::create(_time,
				Vec2(_contentSize.width - _layer->getContentSize().width, 0.0f)));
		_layer->runAction(move(a));
	} else {
		auto a = _callback(Rc<MoveTo>::create(_time, Vec2(0.0f, 0.0f)));
		_layer->runAction(move(a));
	}
}

bool TestGeneralAction::init() {
	if (!TestLayout::init(LayoutName::GeneralActionTest, "")) {
		return false;
	}

	auto node = addChild(Rc<ActionEaseNode>::create("Elastic:", [this](Rc<ActionInterval> &&a) {
		return makeAction(getSelectedType(interpolation::ElasticEaseInOut), move(a));
	}));
	node->setAnchorPoint(Anchor::Middle);
	_nodes.emplace_back(node);

	node = addChild(Rc<ActionEaseNode>::create("Ease:", [this](Rc<ActionInterval> &&a) {
		return makeAction(getSelectedType(interpolation::EaseInOut), move(a));
	}));
	node->setAnchorPoint(Anchor::Middle);
	_nodes.emplace_back(node);

	node = addChild(Rc<ActionEaseNode>::create("Rate:", [this](Rc<ActionInterval> &&a) {
		return makeAction(getSelectedType(interpolation::Linear), move(a));
	}));
	node->setAnchorPoint(Anchor::Middle);
	_nodes.emplace_back(node);

	node = addChild(Rc<ActionEaseNode>::create("Bounce:", [this](Rc<ActionInterval> &&a) {
		return makeAction(getSelectedType(interpolation::BounceEaseInOut), move(a));
	}));
	node->setAnchorPoint(Anchor::Middle);
	_nodes.emplace_back(node);

	node = addChild(Rc<ActionEaseNode>::create("Back:", [this](Rc<ActionInterval> &&a) {
		return makeAction(getSelectedType(interpolation::BackEaseInOut), move(a));
	}));
	node->setAnchorPoint(Anchor::Middle);
	_nodes.emplace_back(node);

	node = addChild(Rc<ActionEaseNode>::create("Sine:", [this](Rc<ActionInterval> &&a) {
		return makeAction(getSelectedType(interpolation::SineEaseInOut), move(a));
	}));
	node->setAnchorPoint(Anchor::Middle);
	_nodes.emplace_back(node);

	node = addChild(Rc<ActionEaseNode>::create("Exponential:", [this](Rc<ActionInterval> &&a) {
		return makeAction(getSelectedType(interpolation::ExpoEaseInOut), move(a));
	}));
	node->setAnchorPoint(Anchor::Middle);
	_nodes.emplace_back(node);

	node = addChild(Rc<ActionEaseNode>::create("Quadratic:", [this](Rc<ActionInterval> &&a) {
		return makeAction(getSelectedType(interpolation::QuadEaseInOut), move(a));
	}));
	node->setAnchorPoint(Anchor::Middle);
	_nodes.emplace_back(node);

	node = addChild(Rc<ActionEaseNode>::create("Cubic:", [this](Rc<ActionInterval> &&a) {
		return makeAction(getSelectedType(interpolation::CubicEaseInOut), move(a));
	}));
	node->setAnchorPoint(Anchor::Middle);
	_nodes.emplace_back(node);

	node = addChild(Rc<ActionEaseNode>::create("Quartic:", [this](Rc<ActionInterval> &&a) {
		return makeAction(getSelectedType(interpolation::QuartEaseInOut), move(a));
	}));
	node->setAnchorPoint(Anchor::Middle);
	_nodes.emplace_back(node);

	node = addChild(Rc<ActionEaseNode>::create("Quintic:", [this](Rc<ActionInterval> &&a) {
		return makeAction(getSelectedType(interpolation::QuintEaseInOut), move(a));
	}));
	node->setAnchorPoint(Anchor::Middle);
	_nodes.emplace_back(node);

	node = addChild(Rc<ActionEaseNode>::create("Circle:", [this](Rc<ActionInterval> &&a) {
		return makeAction(getSelectedType(interpolation::CircEaseInOut), move(a));
	}));
	node->setAnchorPoint(Anchor::Middle);
	_nodes.emplace_back(node);

	return true;
}

void TestGeneralAction::handleContentSizeDirty() {
	TestLayout::handleContentSizeDirty();

	auto size = 28.0f * _nodes.size();
	auto offset = size / 2.0f;

	for (auto &it : _nodes) {
		it->setPosition(_contentSize / 2.0f + Size2(72.0f, offset));
		it->setContentSize(Size2(std::min(_contentSize.width - 160.0f, 600.0f), 24.0f));
		offset -= 28.0f;
	}
}

void TestGeneralAction::handleEnter(xenolith::Scene *scene) {
	TestLayout::handleEnter(scene);

	runAction(Rc<Sequence>::create(0.01f, [this] {
		for (auto &it : _nodes) { it->run(); }
	}, 0.17f, [this] {
		_mode = Mode::In;
		for (auto &it : _nodes) { it->run(); }
	}, 0.34f, [this] {
		_mode = Mode::Out;
		for (auto &it : _nodes) { it->run(); }
	}));

	runAction(Rc<EaseActionTyped>::create(Rc<DelayTime>::create(0.5f), interpolation::EaseInOut,
			0.5f));
}

Rc<ActionInterval> TestGeneralAction::makeAction(interpolation::Type type,
		Rc<ActionInterval> &&a) const {
	return Rc<EaseActionTyped>::create(move(a), type, 0.5f);
}

interpolation::Type TestGeneralAction::getSelectedType(interpolation::Type type) const {
	switch (type) {
	case interpolation::Type::Linear: return interpolation::Type::Linear; break;
	default: return interpolation::Type(type - toInt(_mode)); break;
	}
}

} // namespace stappler::xenolith::app
