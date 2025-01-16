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

namespace stappler::xenolith::app {

bool ActionEaseNode::init(StringView str, Function<Rc<ActionInterval>(Rc<ActionInterval> &&)> &&cb) {
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

void ActionEaseNode::onContentSizeDirty() {
	Node::onContentSizeDirty();

	_label->setPosition(Vec2(-4.0f, _contentSize.height / 2.0f));
	_layer->setContentSize(Size2(48.0f, _contentSize.height));
}

void ActionEaseNode::run() {
	_layer->stopAllActions();

	auto progress = _layer->getPosition().x / (_contentSize.width - _layer->getContentSize().width);
	if (progress < 0.5f) {
		auto a = _callback(Rc<MoveTo>::create(_time, Vec2(_contentSize.width - _layer->getContentSize().width, 0.0f)));
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

	auto node = addChild(Rc<ActionEaseNode>::create("Elastic:", [this] (Rc<ActionInterval> &&a) {
		return makeAction(getSelectedType(interpolation::Elastic_EaseInOut), move(a));
	}));
	node->setAnchorPoint(Anchor::Middle);
	_nodes.emplace_back(node);

	node = addChild(Rc<ActionEaseNode>::create("Ease:", [this] (Rc<ActionInterval> &&a) {
		return makeAction(getSelectedType(interpolation::EaseInOut), move(a));
	}));
	node->setAnchorPoint(Anchor::Middle);
	_nodes.emplace_back(node);

	node = addChild(Rc<ActionEaseNode>::create("Rate:", [this] (Rc<ActionInterval> &&a) {
		return makeAction(getSelectedType(interpolation::Linear), move(a));
	}));
	node->setAnchorPoint(Anchor::Middle);
	_nodes.emplace_back(node);

	node = addChild(Rc<ActionEaseNode>::create("Bounce:", [this] (Rc<ActionInterval> &&a) {
		return makeAction(getSelectedType(interpolation::Bounce_EaseInOut), move(a));
	}));
	node->setAnchorPoint(Anchor::Middle);
	_nodes.emplace_back(node);

	node = addChild(Rc<ActionEaseNode>::create("Back:", [this] (Rc<ActionInterval> &&a) {
		return makeAction(getSelectedType(interpolation::Back_EaseInOut), move(a));
	}));
	node->setAnchorPoint(Anchor::Middle);
	_nodes.emplace_back(node);

	node = addChild(Rc<ActionEaseNode>::create("Sine:", [this] (Rc<ActionInterval> &&a) {
		return makeAction(getSelectedType(interpolation::Sine_EaseInOut), move(a));
	}));
	node->setAnchorPoint(Anchor::Middle);
	_nodes.emplace_back(node);

	node = addChild(Rc<ActionEaseNode>::create("Exponential:", [this] (Rc<ActionInterval> &&a) {
		return makeAction(getSelectedType(interpolation::Expo_EaseInOut), move(a));
	}));
	node->setAnchorPoint(Anchor::Middle);
	_nodes.emplace_back(node);

	node = addChild(Rc<ActionEaseNode>::create("Quadratic:", [this] (Rc<ActionInterval> &&a) {
		return makeAction(getSelectedType(interpolation::Quad_EaseInOut), move(a));
	}));
	node->setAnchorPoint(Anchor::Middle);
	_nodes.emplace_back(node);

	node = addChild(Rc<ActionEaseNode>::create("Cubic:", [this] (Rc<ActionInterval> &&a) {
		return makeAction(getSelectedType(interpolation::Cubic_EaseInOut), move(a));
	}));
	node->setAnchorPoint(Anchor::Middle);
	_nodes.emplace_back(node);

	node = addChild(Rc<ActionEaseNode>::create("Quartic:", [this] (Rc<ActionInterval> &&a) {
		return makeAction(getSelectedType(interpolation::Quart_EaseInOut), move(a));
	}));
	node->setAnchorPoint(Anchor::Middle);
	_nodes.emplace_back(node);

	node = addChild(Rc<ActionEaseNode>::create("Quintic:", [this] (Rc<ActionInterval> &&a) {
		return makeAction(getSelectedType(interpolation::Quint_EaseInOut), move(a));
	}));
	node->setAnchorPoint(Anchor::Middle);
	_nodes.emplace_back(node);

	node = addChild(Rc<ActionEaseNode>::create("Circle:", [this] (Rc<ActionInterval> &&a) {
		return makeAction(getSelectedType(interpolation::Circ_EaseInOut), move(a));
	}));
	node->setAnchorPoint(Anchor::Middle);
	_nodes.emplace_back(node);

	return true;
}

void TestGeneralAction::onContentSizeDirty() {
	TestLayout::onContentSizeDirty();

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
		for (auto &it : _nodes) {
			it->run();
		}
	}, 0.17f, [this] {
		_mode = Mode::In;
		for (auto &it : _nodes) {
			it->run();
		}
	}, 0.34f, [this] {
		_mode = Mode::Out;
		for (auto &it : _nodes) {
			it->run();
		}
	}));

	runAction(Rc<ActionEase>::create(Rc<DelayTime>::create(0.5f)));
}

Rc<ActionInterval> TestGeneralAction::makeAction(interpolation::Type type, Rc<ActionInterval> &&a) const {
	switch (type) {
	case interpolation::Linear: return Rc<EaseRateAction>::create(move(a), 0.5f); break;

	case interpolation::EaseIn: return Rc<EaseIn>::create(move(a), 0.5f); break;
	case interpolation::EaseOut: return Rc<EaseOut>::create(move(a), 0.5f); break;
	case interpolation::EaseInOut: return Rc<EaseInOut>::create(move(a), 0.5f); break;

	case interpolation::Sine_EaseIn: return Rc<EaseSineIn>::create(move(a)); break;
	case interpolation::Sine_EaseOut: return Rc<EaseSineOut>::create(move(a)); break;
	case interpolation::Sine_EaseInOut: return Rc<EaseSineInOut>::create(move(a)); break;

	case interpolation::Quad_EaseIn: return Rc<EaseQuadraticActionIn>::create(move(a)); break;
	case interpolation::Quad_EaseOut: return Rc<EaseQuadraticActionOut>::create(move(a)); break;
	case interpolation::Quad_EaseInOut: return Rc<EaseQuadraticActionInOut>::create(move(a)); break;

	case interpolation::Cubic_EaseIn: return Rc<EaseCubicActionIn>::create(move(a)); break;
	case interpolation::Cubic_EaseOut: return Rc<EaseCubicActionOut>::create(move(a)); break;
	case interpolation::Cubic_EaseInOut: return Rc<EaseCubicActionInOut>::create(move(a)); break;

	case interpolation::Quart_EaseIn: return Rc<EaseQuarticActionIn>::create(move(a)); break;
	case interpolation::Quart_EaseOut: return Rc<EaseQuarticActionOut>::create(move(a)); break;
	case interpolation::Quart_EaseInOut: return Rc<EaseQuarticActionInOut>::create(move(a)); break;

	case interpolation::Quint_EaseIn: return Rc<EaseQuinticActionIn>::create(move(a)); break;
	case interpolation::Quint_EaseOut: return Rc<EaseQuinticActionOut>::create(move(a)); break;
	case interpolation::Quint_EaseInOut: return Rc<EaseQuinticActionInOut>::create(move(a)); break;

	case interpolation::Expo_EaseIn: return Rc<EaseExponentialIn>::create(move(a)); break;
	case interpolation::Expo_EaseOut: return Rc<EaseExponentialOut>::create(move(a)); break;
	case interpolation::Expo_EaseInOut: return Rc<EaseExponentialInOut>::create(move(a)); break;

	case interpolation::Circ_EaseIn: return Rc<EaseCircleActionIn>::create(move(a)); break;
	case interpolation::Circ_EaseOut: return Rc<EaseCircleActionOut>::create(move(a)); break;
	case interpolation::Circ_EaseInOut: return Rc<EaseCircleActionInOut>::create(move(a)); break;

	case interpolation::Elastic_EaseIn: return Rc<EaseElasticIn>::create(move(a)); break;
	case interpolation::Elastic_EaseOut: return Rc<EaseElasticOut>::create(move(a)); break;
	case interpolation::Elastic_EaseInOut: return Rc<EaseElasticInOut>::create(move(a)); break;

	case interpolation::Back_EaseIn: return Rc<EaseBackIn>::create(move(a)); break;
	case interpolation::Back_EaseOut: return Rc<EaseBackOut>::create(move(a)); break;
	case interpolation::Back_EaseInOut: return Rc<EaseBackInOut>::create(move(a)); break;

	case interpolation::Bounce_EaseIn: return Rc<EaseBounceIn>::create(move(a)); break;
	case interpolation::Bounce_EaseOut: return Rc<EaseBounceOut>::create(move(a)); break;
	case interpolation::Bounce_EaseInOut: return Rc<EaseBounceInOut>::create(move(a)); break;
	default: break;
	}
	return nullptr;
}

interpolation::Type TestGeneralAction::getSelectedType(interpolation::Type type) const {
	switch (type) {
	case interpolation::Type::Linear:
		return interpolation::Type::Linear;
		break;
	default:
		return interpolation::Type(type - toInt(_mode));
		break;
	}
}

}
