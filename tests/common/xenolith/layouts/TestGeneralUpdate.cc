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
#include "SPTime.h"

namespace STAPPLER_VERSIONIZED stappler::xenolith::app {

bool TestGeneralUpdate::init() {
	if (!TestLayout::init(LayoutName::GeneralUpdateTest, "Gradient should spin monotonically")) {
		return false;
	}

	_sprite = addChild(Rc<Sprite>::create("xenolith-1-480.png"), ZOrder(2));
	//_sprite->setAutofit(Sprite::Autofit::Contain);
	_sprite->setSamplerIndex(Sprite::SamplerIndexDefaultFilterLinear);
	_sprite->setAnchorPoint(Anchor::Middle);

	_sprite2 = addChild(Rc<Sprite>::create(), ZOrder(1));
	_sprite2->setAutofit(Sprite::Autofit::Contain);
	_sprite2->setAnchorPoint(Anchor::Middle);

	updateAngle(0.5f);

	_background = addChild(Rc<Layer>::create(Color::White));
	_background->setAnchorPoint(Anchor::Middle);

	scheduleUpdate();

	return true;
}

void TestGeneralUpdate::onContentSizeDirty() {
	TestLayout::onContentSizeDirty();

	_background->setContentSize(_contentSize);
	_background->setPosition(_contentSize / 2.0f);

	_sprite->setPosition(_contentSize / 2.0f);
	_sprite->setContentSize(Size2(750.0f, 600.0f));

	_sprite2->setPosition(_contentSize / 2.0f);
	_sprite2->setContentSize(Size2(750.0f, 600.0f));

	runAction(Rc<RenderContinuously>::create());
}

void TestGeneralUpdate::onEnter(xenolith::Scene *scene) {
	TestLayout::onEnter(scene);

	auto cache = _director->getResourceCache();

	StringView name("external://resources/xenolith-2-480.png");

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
		if (_sprite2) {
			_sprite2->removeFromParent(true);
			_sprite2 = nullptr;
		}
	}));
}

void TestGeneralUpdate::update(const UpdateTime &time) {
	TestLayout::update(time);

	auto t = time.app % (5_sec).toMicros();

	if (_background) {
		_background->setGradient(SimpleGradient(Color::Red_500, Color::Green_500,
				Vec2::forAngle(M_PI * 2.0 * (float(t) / (1_sec).toMicros()))));
	}

	updateAngle(-float(t) / (1_sec).toMicros());
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
