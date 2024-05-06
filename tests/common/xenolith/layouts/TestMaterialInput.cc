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

#include "TestMaterialInput.h"

namespace STAPPLER_VERSIONIZED stappler::xenolith::app {

bool TestMaterialInput::init() {
	if (!TestMaterial::init(LayoutName::MaterialInputTest, "")) {
		return false;
	}

	_field = addChild(Rc<material2d::InputField>::create());
	_field->setLabelText("Label text");
	_field->setSupportingText("Supporting text");
	_field->setLeadingIconName(IconName::Action_search_solid);
	_field->setTrailingIconName(IconName::Alert_error_solid);
	_field->setContentSize(Size2(300.0f, 56.0f));
	_field->setAnchorPoint(Anchor::Middle);

	return true;
}

static Vector<InputEventData> makeKeyInput(InputKeyCode code, char32_t ch) {
	auto ret = Vector<InputEventData> {
		InputEventData({
			toInt(code),
			InputEventName::KeyPressed,
			InputMouseButton::Touch,
			InputModifier::None,
			float(0),
			float(0)
		}),
		InputEventData({
			toInt(code),
			InputEventName::KeyReleased,
			InputMouseButton::Touch,
			InputModifier::None,
			float(0),
			float(0)
		})
	};

	ret[0].key.keycode = code;
	ret[0].key.compose = InputKeyComposeState::Nothing;
	ret[0].key.keysym = toInt(code);
	ret[0].key.keychar = ch;

	ret[1].key.keycode = code;
	ret[1].key.compose = InputKeyComposeState::Nothing;
	ret[1].key.keysym = toInt(code);
	ret[1].key.keychar = ch;

	return ret;
}

void TestMaterialInput::onContentSizeDirty() {
	TestMaterial::onContentSizeDirty();

	_field->setPosition(Vec2(_contentSize / 2.0f) - Vec2(0.0f, 100.0f));

	Vector<InputEventData> events {
		InputEventData({
			0,
			InputEventName::Begin,
			InputMouseButton::Touch,
			InputModifier::None,
			float(_field->getPosition().x),
			float(_field->getPosition().y)
		}),
		InputEventData({
			0,
			InputEventName::End,
			InputMouseButton::Touch,
			InputModifier::None,
			float(_field->getPosition().x),
			float(_field->getPosition().y)
		})
	};

	auto v = _director->getView();
	v->handleInputEvents(move(events));

	auto a = Rc<Sequence>::create(0.03f, [this] {
		_director->getView()->handleInputEvents(makeKeyInput(InputKeyCode::T, 'T'));
		_director->getView()->handleInputEvents(makeKeyInput(InputKeyCode::E, 'E'));
	}, 0.03f, [this] {
		_director->getView()->handleInputEvents(makeKeyInput(InputKeyCode::BACKSPACE, '\n'));
	}, 0.03f, [this] {
		_director->getView()->handleInputEvents(makeKeyInput(InputKeyCode::E, 'E'));
		_director->getView()->handleInputEvents(makeKeyInput(InputKeyCode::S, 'S'));
		_director->getView()->handleInputEvents(makeKeyInput(InputKeyCode::T, 'T'));
	}, 0.03f, [this] {
		_director->getView()->handleInputEvents(makeKeyInput(InputKeyCode::SPACE, ' '));
		_director->getView()->handleInputEvents(makeKeyInput(InputKeyCode::T, 'T'));
		_director->getView()->handleInputEvents(makeKeyInput(InputKeyCode::E, 'E'));
		_director->getView()->handleInputEvents(makeKeyInput(InputKeyCode::S, 'S'));
		_director->getView()->handleInputEvents(makeKeyInput(InputKeyCode::T, 'T'));
		_director->getView()->handleInputEvents(makeKeyInput(InputKeyCode::SPACE, ' '));
		_director->getView()->handleInputEvents(makeKeyInput(InputKeyCode::T, 'T'));
		_director->getView()->handleInputEvents(makeKeyInput(InputKeyCode::E, 'E'));
		_director->getView()->handleInputEvents(makeKeyInput(InputKeyCode::S, 'S'));
		_director->getView()->handleInputEvents(makeKeyInput(InputKeyCode::T, 'T'));
	}, 0.03f, [this] {
		_director->getView()->handleInputEvents(makeKeyInput(InputKeyCode::SPACE, ' '));
		_director->getView()->handleInputEvents(makeKeyInput(InputKeyCode::T, 'T'));
		_director->getView()->handleInputEvents(makeKeyInput(InputKeyCode::E, 'E'));
		_director->getView()->handleInputEvents(makeKeyInput(InputKeyCode::S, 'S'));
		_director->getView()->handleInputEvents(makeKeyInput(InputKeyCode::T, 'T'));
		_director->getView()->handleInputEvents(makeKeyInput(InputKeyCode::SPACE, ' '));
		_director->getView()->handleInputEvents(makeKeyInput(InputKeyCode::T, 'T'));
		_director->getView()->handleInputEvents(makeKeyInput(InputKeyCode::E, 'E'));
		_director->getView()->handleInputEvents(makeKeyInput(InputKeyCode::S, 'S'));
		_director->getView()->handleInputEvents(makeKeyInput(InputKeyCode::T, 'T'));
	}, 0.03f, [this] {
		if (_director->getTextInputManager()->hasText()) {
			_director->getTextInputManager()->textChanged(WideStringView(), TextCursor(), TextCursor());

			auto wstr = string::toUtf16<Interface>(toString(_director->getFps()));

			_director->getTextInputManager()->textChanged(wstr, TextCursor(0, wstr.size()), TextCursor());
		}
	}, 0.03f, [this] {
		_director->getView()->handleInputEvents(makeKeyInput(InputKeyCode::SPACE, ' '));
		_director->getView()->handleInputEvents(makeKeyInput(InputKeyCode::T, 'T'));
		_director->getView()->handleInputEvents(makeKeyInput(InputKeyCode::E, 'E'));
		_director->getView()->handleInputEvents(makeKeyInput(InputKeyCode::S, 'S'));
		_director->getView()->handleInputEvents(makeKeyInput(InputKeyCode::T, 'T'));

		_director->getView()->handleInputEvent(core::InputEventData::BoolEvent(core::InputEventName::Background, true));
		_director->getView()->handleInputEvent(core::InputEventData::BoolEvent(core::InputEventName::PointerEnter, true));
		_director->getView()->handleInputEvent(core::InputEventData::BoolEvent(core::InputEventName::FocusGain, false));

		Vector<InputEventData> events({
			core::InputEventData::BoolEvent(core::InputEventName::Background, false),
			core::InputEventData::BoolEvent(core::InputEventName::PointerEnter, false),
			core::InputEventData::BoolEvent(core::InputEventName::FocusGain, true)
		});
		_director->getView()->handleInputEvents(move(events));
	});

	runAction(a);
}

void TestMaterialInput::update(const UpdateTime &) {

}

}
