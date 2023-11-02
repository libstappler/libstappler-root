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

#include "TestMaterial.h"
#include "TestAppScene.h"
#include "XL2dLabel.h"

namespace stappler::xenolith::app {

bool TestMaterial::init(LayoutName layout, StringView text) {
	if (!FlexibleLayout::init()) {
		return false;
	}

	_layout = layout;

	setViewDecorationTracked(true);

	_infoLabel = addChild(Rc<Label>::create(), ZOrderMax);
	_infoLabel->setString(text);
	_infoLabel->setAnchorPoint(Anchor::MiddleTop);
	_infoLabel->setAlignment(Label::TextAlign::Center);
	_infoLabel->setFontSize(24);
	_infoLabel->setAdjustValue(16);
	_infoLabel->setMaxLines(4);

	setName(getLayoutNameId(_layout));

	return true;
}

void TestMaterial::onContentSizeDirty() {
	FlexibleLayout::onContentSizeDirty();

	_infoLabel->setPosition(Vec2(_contentSize.width / 2.0f, _contentSize.height - _decorationPadding.top - 16.0f));
	_infoLabel->setWidth(_contentSize.width * 3.0f / 4.0f);
}

void TestMaterial::onEnter(xenolith::Scene *scene) {
	FlexibleLayout::onEnter(scene);
}

void TestMaterial::setDataValue(Value &&val) {
	FlexibleLayout::setDataValue(move(val));
}

}
