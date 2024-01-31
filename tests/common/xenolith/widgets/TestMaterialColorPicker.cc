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

#include "TestMaterialColorPicker.h"

#include "MaterialLabel.h"
#include "XLInputListener.h"
#include "XLTexture.h"

namespace STAPPLER_VERSIONIZED stappler::xenolith::app {

bool MaterialColorPicker::init(Type type, const material2d::ColorHCT &color, Function<void(float)> &&cb) {
	if (!Sprite::init()) {
		return false;
	}

	_type = type;
	_targetColor = color;
	_callback = move(cb);
	switch (_type) {
	case Type::Hue: _value = _targetColor.data.hue / 360.0f; break;
	case Type::Chroma: _value = _targetColor.data.chroma / 100.0f; break;
	case Type::Tone: _value = _targetColor.data.tone / 100.0f; break;
	}

	_label = addChild(Rc<material2d::TypescaleLabel>::create(material2d::TypescaleRole::TitleLarge), ZOrder(1));
	_label->setFontSize(20);
	_label->setAnchorPoint(Anchor::MiddleLeft);
	_label->setString(makeString());
	_label->setPersistentLayout(true);

	_indicator = addChild(Rc<Layer>::create(Color::Grey_500));
	_indicator->setAnchorPoint(Anchor::MiddleLeft);

	_input = addInputListener(Rc<InputListener>::create());
	_input->addTouchRecognizer([this] (const GestureData &data) {
		switch (data.event) {
		case GestureEvent::Began:
		case GestureEvent::Activated:
			setValue(math::clamp(convertToNodeSpace(data.input->currentLocation).x / _contentSize.width, 0.0f, 1.0f));
			break;
		default:
			break;
		}
		return true;
	}, InputListener::makeButtonMask({InputMouseButton::Touch}));

	return true;
}

void MaterialColorPicker::onContentSizeDirty() {
	Sprite::onContentSizeDirty();

	_label->setPosition(Vec2(_contentSize.width + 16.0f, _contentSize.height / 2.0f));
	_indicator->setContentSize(Size2(2.0f, _contentSize.height + 12.0f));
	_indicator->setPosition(Vec2(_contentSize.width * _value, _contentSize.height / 2.0f));
}

const material2d::ColorHCT &MaterialColorPicker::getTargetColor() const {
	return _targetColor;
}

void MaterialColorPicker::setTargetColor(const material2d::ColorHCT &color) {
	if (_targetColor != color) {
		_targetColor = color;
		_vertexesDirty = true;
		_label->setString(makeString());
	}
}

void MaterialColorPicker::setValue(float value) {
	if (_value != value) {
		_value = value;
		updateValue();
		if (_callback) {
			switch (_type) {
			case Type::Hue: _callback(_value * 360.0f); break;
			case Type::Chroma: _callback(_value * 100.0f); break;
			case Type::Tone: _callback(_value * 100.0f); break;
			}
		}
	}
}

float MaterialColorPicker::getValue() const {
	return _value;
}

void MaterialColorPicker::setLabelColor(const Color4F &color) {
	_label->setColor(color);
}

void MaterialColorPicker::updateVertexesColor() {
	_indicator->setPosition(Vec2(_contentSize.width * _value, _contentSize.height / 2.0f));
}

void MaterialColorPicker::initVertexes() {
	_vertexes.init(QuadsCount * 4, QuadsCount * 6);
	_vertexesDirty = true;
}

void MaterialColorPicker::updateVertexes() {
	_vertexes.clear();

	auto texExtent = _texture->getExtent();
	auto texSize = Size2(texExtent.width, texExtent.height);

	texSize = Size2(texSize.width * _textureRect.size.width, texSize.height * _textureRect.size.height);

	Size2 size(_contentSize.width / QuadsCount, _contentSize.height);
	Vec2 origin(0, 0);
	for (size_t i = 0; i < QuadsCount; ++ i) {
		material2d::ColorHCT color1;
		material2d::ColorHCT color2;

		switch (_type) {
		case Type::Hue:
			color1 = material2d::ColorHCT(i * (float(360) / float(QuadsCount)), _targetColor.data.chroma, _targetColor.data.tone, 1.0f);
			color2 = material2d::ColorHCT((i + 1) * (float(360) / float(QuadsCount)), _targetColor.data.chroma, _targetColor.data.tone, 1.0f);
			break;
		case Type::Chroma:
			color1 = material2d::ColorHCT(_targetColor.data.hue, i * (float(100) / float(QuadsCount)), _targetColor.data.tone, 1.0f);
			color2 = material2d::ColorHCT(_targetColor.data.hue, (i + 1) * (float(100) / float(QuadsCount)), _targetColor.data.tone, 1.0f);
			break;
		case Type::Tone:
			color1 = material2d::ColorHCT(_targetColor.data.hue, _targetColor.data.chroma, i * (float(100) / float(QuadsCount)), 1.0f);
			color2 = material2d::ColorHCT(_targetColor.data.hue, _targetColor.data.chroma, (i + 1) * (float(100) / float(QuadsCount)), 1.0f);
			break;
		}

		_vertexes.addQuad()
			.setGeometry(Vec4(origin, 0.0f, 1.0f), size)
			.setTextureRect(Rect(0.0f, 0.0f, 1.0f, 1.0f), 1.0f, 1.0f, _flippedX, _flippedY, _rotated)
			.setColor({color1.asColor4F(), color1.asColor4F(), color2.asColor4F(), color2.asColor4F()});

		origin += Vec2(size.width, 0.0f);
	}

	_vertexColorDirty = false;
}

String MaterialColorPicker::makeString() {
	switch (_type) {
	case Type::Hue:
		return toString("Hue: ", _targetColor.data.hue);
		break;
	case Type::Chroma:
		return toString("Chroma: ", _targetColor.data.chroma);
		break;
	case Type::Tone:
		return toString("Tone: ", _targetColor.data.tone);
		break;
	}
	return String();
}

void MaterialColorPicker::updateValue() {
	_indicator->setPosition(Vec2(_contentSize.width * _value, _contentSize.height / 2.0f));
}

}
