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

#include "TestCheckbox.h"

#include "XLInputListener.h"
#include "MaterialLabel.h"

namespace STAPPLER_VERSIONIZED stappler::xenolith::app {

bool TestCheckbox::init(bool value, Function<void(bool)> &&cb) {
	if (!Layer::init(Color::Grey_200)) {
		return false;
	}

	_value = value;
	_callback = sp::move(cb);

	setColor(_backgroundColor);
	setContentSize(Size2(32.0f, 32.0f));

	_input = addInputListener(Rc<InputListener>::create());
	_input->addTapRecognizer([this] (const GestureTap &data) {
		switch (data.event) {
		case GestureEvent::Activated:
			setValue(!_value);
			break;
		default:
			break;
		}
		return true;
	}, InputListener::makeButtonMask({InputMouseButton::Touch}), 1);

	updateValue();

	return true;
}

void TestCheckbox::setValue(bool value) {
	if (_value != value) {
		_value = value;
		updateValue();
		if (_callback) {
			_callback(_value);
		}
	}
}

bool TestCheckbox::getValue() const {
	return _value;
}

void TestCheckbox::setForegroundColor(const Color4F &color) {
	if (_foregroundColor != color) {
		_foregroundColor = color;
		updateValue();
	}
}

Color4F TestCheckbox::getForegroundColor() const {
	return _foregroundColor;
}

void TestCheckbox::setBackgroundColor(const Color4F &color) {
	if (_backgroundColor != color) {
		_backgroundColor = color;
		updateValue();
	}
}

Color4F TestCheckbox::getBackgroundColor() const {
	return _backgroundColor;
}

void TestCheckbox::updateValue() {
	if (_value) {
		setColor(_foregroundColor);
	} else {
		setColor(_backgroundColor);
	}
}

bool TestCheckboxWithLabel::init(StringView title, bool value, Function<void(bool)> &&cb) {
	if (!TestCheckbox::init(value, sp::move(cb))) {
		return false;
	}

	_label = addChild(Rc<material2d::TypescaleLabel>::create(material2d::TypescaleRole::HeadlineSmall));
	_label->setAnchorPoint(Anchor::MiddleLeft);
	_label->setString(title);

	return true;
}

void TestCheckboxWithLabel::onContentSizeDirty() {
	TestCheckbox::onContentSizeDirty();

	_label->setPosition(Vec2(_contentSize.width + 16.0f, _contentSize.height / 2.0f));
}

void TestCheckboxWithLabel::setLabelColor(const Color4F &color) {
	_label->setColor(color);
}

}
