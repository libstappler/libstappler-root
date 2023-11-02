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

namespace stappler::xenolith::app {

bool TestGeneralUpdate::init() {
	if (!TestLayout::init(LayoutName::GeneralUpdateTest, "Gradient should spin monotonically")) {
		return false;
	}

	_background = addChild(Rc<Layer>::create(Color::White));
	_background->setAnchorPoint(Anchor::Middle);

	scheduleUpdate();

	return true;
}

void TestGeneralUpdate::onContentSizeDirty() {
	TestLayout::onContentSizeDirty();

	_background->setContentSize(_contentSize);
	_background->setPosition(_contentSize / 2.0f);
}

void TestGeneralUpdate::onEnter(xenolith::Scene *scene) {
	TestLayout::onEnter(scene);
}

void TestGeneralUpdate::update(const UpdateTime &time) {
	TestLayout::update(time);

	auto t = time.app % 5_usec;

	if (_background) {
		_background->setGradient(SimpleGradient(Color::Red_500, Color::Green_500,
				Vec2::forAngle(M_PI * 2.0 * (float(t) / 5_usec))));
	}
}

}
