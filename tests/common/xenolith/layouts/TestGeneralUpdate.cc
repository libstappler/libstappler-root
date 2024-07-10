/**
 Copyright (c) 2023 Stappler LLC <admin@stappler.dev>

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

#include "TestGeneralUpdate.h"
#include "XLDirector.h"
#include "XLScheduler.h"
#include "XLActionManager.h"
#include "SPTime.h"

namespace STAPPLER_VERSIONIZED stappler::xenolith::app {

bool TestGeneralUpdate::init() {
	if (!TestLayout::init(LayoutName::GeneralUpdateTest, "Gradient should spin monotonically")) {
		return false;
	}

	setIgnoreParentState(true);

	setOnEnterCallback([] (xenolith::Scene *) { });
	setOnExitCallback([] () { });
	setOnContentSizeDirtyCallback([] () { });
	setOnReorderChildDirtyCallback([] () { });
	setOnTransformDirtyCallback([] (const Mat4 &) { });

	_sprite = addChild(Rc<Sprite>::create("xenolith-1-480.png"), ZOrder(2));
	//_sprite->setAutofit(Sprite::Autofit::Contain);
	_sprite->setSamplerIndex(Sprite::SamplerIndexDefaultFilterLinear);
	_sprite->setAnchorPoint(Anchor::Middle);
	_sprite->disableScissor();
	_sprite->enableScissor();
	_sprite->setCascadeOpacityEnabled(true);
	_sprite->setCascadeColorEnabled(true);
	_sprite->setOpacity(0.8f);
	if (_sprite->getStateApplyMode() != StateApplyMode::ApplyForNodesAbove) {
		_sprite->setStateApplyMode(StateApplyMode::ApplyForNodesAbove);
	}

	_sprite2 = _sprite->addChild(Rc<Sprite>::create(), ZOrder(1), 123);
	_sprite2->setAutofit(Sprite::Autofit::Contain);
	_sprite2->setAnchorPoint(Anchor::Middle);
	_sprite2->enableScissor();
	_sprite2->setStateApplyMode(StateApplyMode::ApplyForAll);
	if (_sprite2->isIgnoreParentState()) {
		_sprite2->setIgnoreParentState(false);
	}
	if (_sprite2->isScissorEnabled()) {
		_sprite2->setScissorOutlone(_sprite2->getScissorOutline());
	}

	_spriteLayer = _sprite2->addChild(Rc<Layer>::create(Color::White));
	_spriteLayer->setAnchorPoint(Anchor::Middle);

	_spriteLayer2 = _sprite->addChild(Rc<Layer>::create(Color::Red_100));
	_spriteLayer2->setAnchorPoint(Anchor::Middle);

	updateAngle(0.5f);

	_background = addChild(Rc<Layer>::create(Color::White));
	_background->setAnchorPoint(Anchor::Middle);

	_component = addComponent(Rc<Component>::create());
	_component->setEnabled(true);
	_component->scheduleUpdate();

	_listener = _spriteLayer->addInputListener(Rc<InputListener>::create());

	_sub = addComponent(Rc<SubscriptionListener>::create());
	_sub->setCallback([] (SubscriptionFlags flags) { });
	_sub->check();

	scheduleUpdate();

	return true;
}

void TestGeneralUpdate::onContentSizeDirty() {
	TestLayout::onContentSizeDirty();

	_background->setContentSize(_contentSize);
	_background->setPosition(_contentSize / 2.0f);

	_sprite->setPosition(_contentSize / 2.0f);
	_sprite->setContentSize(Size2(750.0f, 600.0f));

	_sprite2->setPosition(_sprite->getContentSize() / 2.0f);
	_sprite2->setContentSize(Size2(250.0f, 250.0f));

	if (_sprite3) {
		_sprite3->setPosition(_sprite->getContentSize() / 2.0f);
		_sprite3->setContentSize(Size2(50.0f, 50.0f));
	}

	_spriteLayer->setPosition(Vec3(1.0f, 2.0f, 0.0f));
	_spriteLayer->setPosition(_sprite2->getContentSize() / 2.0f);
	_spriteLayer->setContentSize(Size2(50.0f, 50.0f));

	_spriteLayer2->setPosition(_sprite->getContentSize() / 2.0f - Vec2(0.0f, -50.0f));
	_spriteLayer2->setContentSize(Size2(50.0f, 50.0f));

	runAction(Rc<RenderContinuously>::create());

	if (Node::isParent(this, _spriteLayer)) {
		Node::getChainNodeToParentTransform(this, _spriteLayer, true);
		Node::getChainNodeToParentTransform(this, _spriteLayer, false);
		Node::getChainParentToNodeTransform(this, _spriteLayer, true);
		Node::getChainParentToNodeTransform(this, _spriteLayer, false);
	}

	_sprite2->setScale(Vec3(0.75f, 0.8f, 0.85f));

	_spriteLayer->getScale();
	_spriteLayer->getSkew();
	_spriteLayer->getRotation();
	_spriteLayer->getRotation3D();
	_spriteLayer->getRotationQuat();
	_spriteLayer->getChildren();
	_spriteLayer->getChildrenCount();
	_spriteLayer->getTag();
	_spriteLayer->getDepthIndex();
	_spriteLayer->getFocus();

	_spriteLayer->setName("Name");
	_spriteLayer->getName();
	_spriteLayer->setDataValue(Value("test"));
	_spriteLayer->getDataValue();

	_spriteLayer->setScaleX(1.1f);
	_spriteLayer->setScaleY(1.1f);
	_spriteLayer->setScaleZ(1.1f);
	_spriteLayer->setScale(Vec3(1.2f, 1.2f, 1.2f));
	_spriteLayer->setScale(Vec2(1.4f, 1.4f));
	_spriteLayer->setScale(1.5);
	_spriteLayer->setPositionZ(0.0f);
	_spriteLayer->setSkewX(0.0f);
	_spriteLayer->setSkewY(0.0f);
	_spriteLayer->setSkewX(1.0f);
	_spriteLayer->setSkewY(1.0f);
	_spriteLayer->setSkewX(0.0f);
	_spriteLayer->setSkewY(0.0f);
	_spriteLayer->setRotation(1.0f);
	_spriteLayer->setRotation(Vec3(1.0f, 2.0f, 3.0f));
	_spriteLayer->setRotation(Quaternion(1.0f, 2.0f, 3.0f, 4.0f));
	_spriteLayer->setRotation(0.0f);

	_spriteLayer->getBoundingBox();
	_spriteLayer->convertToWorldSpaceAR(_contentSize / 2.0f);
	_spriteLayer->convertToNodeSpaceAR(_contentSize / 2.0f);
}

void TestGeneralUpdate::onEnter(xenolith::Scene *scene) {
	TestLayout::onEnter(scene);

	auto cache = _director->getResourceCache();

	StringView name("external://resources/xenolith-2-480.png");

	_sprite2->setTextureLoadedCallback([this] () {
		auto s = Rc<Sprite>::create(Rc<Texture>(_sprite2->getTexture()));
		s = Rc<Sprite>::create();
		s->setTexture(_sprite2->getTexture()->getName());

		_sprite3 = addChild(Rc<Sprite>::create());
		_sprite3->setTexture(_sprite2->getTexture()->getName());
		_sprite3->setTextureRect(Rect(0.1f, 0.1f, 0.8f, 0.8f));
		_sprite3->getLinearGradient();
		_sprite3->setAutofitPosition(Anchor::TopRight);

		auto b = _sprite2->getBlendInfo();
		auto b2 = b;
		b2.opColor = toInt(core::BlendOp::Max);
		_sprite2->setBlendInfo(b2);
		_sprite2->setBlendInfo(b);

		auto w = _sprite2->getLineWidth();
		_sprite2->setLineWidth(1.0f);
		_sprite2->setLineWidth(w);

	});

	if (auto res = cache->getTemporaryResource(name)) {
		_resource = res;
		if (auto tex = res->acquireTexture(name)) {
			_sprite2->setTexture(move(tex));
		}
	} else {
		auto tex = cache->addExternalImage(name,
				core::ImageInfo(core::ImageFormat::R8G8B8A8_UNORM, core::ImageUsage::Sampled, core::ImageHints::Opaque),
				FilePath("resources/xenolith-2-480.png"), TimeInterval::floatSeconds(0.1f));
		if (tex) {
			_sprite2->setTexture(move(tex));
		}

		if (auto cachedRes = cache->getTemporaryResource(name)) {
			_resource = cachedRes;
		}
	}

	runAction(Rc<Sequence>::create(0.1f, [this] {
		getNumberOfRunningActions();

		_spriteLayer->removeInputListener(_listener);
		_spriteLayer->clearFocus();

		auto n = _sprite->getChildByTag(123);
		if (_sprite2 && n == _sprite2) {
			_sprite3->removeFromParent(true);
			_sprite3 = nullptr;

			_sprite->removeChildByTag(123);
			_sprite2 = nullptr;
		}

		_sprite->pause();
		_sprite->resume();

		_sprite->setCascadeColorEnabled(false);
		_sprite->setCascadeColorEnabled(true);
		_sprite->setCascadeOpacityEnabled(false);
		_sprite->setCascadeOpacityEnabled(true);
	}));

	_sprite->runAction(Rc<Speed>::create(Rc<MoveTo>::create(0.5f, _contentSize / 2.0f + Vec2(0.0f, 100.0f)), 2.0f), 123);

	_sprite->runAction(Rc<Spawn>::create(
		Rc<TintTo>::create(1.0f, Color::Amber_500),
		Rc<ScaleTo>::create(1.5f, 1.2f),
		Rc<RenderContinuously>::create(0.5f),
		0.5f,
		[] () { }
	));

	_spriteLayer2->runAction(Rc<Repeat>::create(Rc<ScaleTo>::create(0.1f, Vec3(1.0f, 1.2f, 1.1f)), 3));
	_spriteLayer2->runAction(Rc<RepeatForever>::create(Rc<MoveTo>::create(0.1f, Vec3(1.0f, 1.2f, 0.0f))), 234);
	auto a = _spriteLayer2->runAction(Rc<ActionProgress>::create(1.5f, 0.0f, 1.0f, [] (float p) { }));
	a->setDuration(1.5f);
	a->getContainer();
	a->getSourceProgress();

	runAction(Rc<Sequence>::create(0.03f, [this] {
		_spriteLayer2->runAction(Rc<Hide>::create());
	}, 0.03f, [this] {
		_spriteLayer2->runAction(Rc<Show>::create());
	}, 0.03f, [this, a] {
		_spriteLayer2->runAction(Rc<ToggleVisibility>::create());
		_spriteLayer2->stopAction(a);
	}, 0.03f, [this] {
		_spriteLayer2->runAction(Rc<ToggleVisibility>::create());

		auto nodes = _actionManager->pauseAllRunningActions();
		_actionManager->resumeTargets(nodes);

		_actionManager->pauseTarget(_sprite);
		_actionManager->resumeTarget(_sprite);

	}, 0.03f, [this] {
		_spriteLayer2->stopActionByTag(234);
		_spriteLayer2->runAction(Rc<Place>::create(Vec2(50.0f, 50.0f)));
	}, 0.03f, [this] {
		_sprite->stopAllActionsByTag(123);

		_spriteLayer2->stopAllActions();

		_spriteLayer2->runAction(Rc<RemoveSelf>::create(true, false));
		_spriteLayer2 = nullptr;
	}, 0.03f, [this] {
		_sprite->stopAllActions();
	}, 0.03f, [] { }));
}

void TestGeneralUpdate::update(const UpdateTime &time) {
	TestLayout::update(time);

	auto t = time.app % (5_sec).toMicros();

	if (_background) {
		_background->setGradient(SimpleGradient(Color::Red_500, Color::Green_500,
				Vec2::forAngle(M_PI * 2.0 * (float(t) / (1_sec).toMicros()))));
	}

	updateAngle(-float(t) / (1_sec).toMicros());

	if (_component->isRunning()) {
		if (_component->isScheduled()) {
			_scheduler->resume(_component);
			if (!_scheduler->isPaused(_component)) {
				_component->unscheduleUpdate();
			}
		} else {
			_component->scheduleUpdate();
		}
	}
}

void TestGeneralUpdate::updateAngle(float val) {
	auto q = val * 360.0_to_rad - 180.0_to_rad;

	_angle = q;

	Vec2 center(0.5f, 0.5f);
	Vec2 angle = Vec2::forAngle(_angle);

	Vec2 start = center - angle * 0.5f;
	Vec2 end = center + angle * 0.5f;

	auto g = Rc<LinearGradient>::create(start, end, Vector<GradientStep>{
		GradientStep{0.0f, 1.0f, Color::Blue_500},
		GradientStep{0.45f, 0.0f, Color::Red_500},
		GradientStep{0.55f, 0.0f, Color::Blue_500},
		GradientStep{1.0f, 1.0f, Color::Red_500},
	});

	_sprite->setLinearGradient(move(g));
}

}
