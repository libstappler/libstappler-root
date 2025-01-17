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

#include "GuiSceneContent.h"
#include "XLInputListener.h"

namespace stappler::xenolith::test {

GuiSceneContent::~GuiSceneContent() { }

bool GuiSceneContent::init() {
	if (!SceneContent2d::init()) {
		return false;
	}

	_layer = addChild(Rc<basic2d::Layer>::create(Color::Red_500));
	_layer->setContentSize(Size2(48.0f, 48.0f));
	_layer->setAnchorPoint(Anchor::Middle);

	_layer2 = addChild(Rc<basic2d::Layer>::create(Color::Blue_500));
	_layer2->setContentSize(Size2(48.0f, 48.0f));
	_layer2->setAnchorPoint(Anchor::Middle);

	_layer3 = addChild(Rc<basic2d::Layer>::create(Color::Green_500));
	_layer3->setContentSize(Size2(48.0f, 48.0f));
	_layer3->setAnchorPoint(Anchor::Middle);

	_layer4 = addChild(Rc<basic2d::Layer>::create(Color::Grey_500));
	_layer4->setContentSize(Size2(48.0f, 48.0f));
	_layer4->setAnchorPoint(Anchor::Middle);

	auto l = addInputListener(Rc<InputListener>::create());

	GestureKeyRecognizer::KeyMask keys;
	keys.set();
	l->addKeyRecognizer([] (const GestureData &gesture) {
		log::verbose("Scene", gesture.event, ": ", core::getInputKeyCodeName(gesture.input->data.key.keycode),
				", ", gesture.input->data.key.keysym, ",", getInputModifiersNames(gesture.input->data.modifiers),
				", ", std::hex, uint32_t(gesture.input->data.key.keychar));
		return true;
	}, sp::move(keys));

	GestureKeyRecognizer::ButtonMask buttons;
	buttons.set();
	l->addTouchRecognizer([] (const GestureData &gesture) {
		log::verbose("Scene", gesture.event, ": ", core::getInputButtonName(gesture.input->data.button),
				",", getInputModifiersNames(gesture.input->data.modifiers), ", ", gesture.input->currentLocation);
		return true;
	}, sp::move(buttons));

	l->addScrollRecognizer([] (const GestureScroll &scroll) {
		log::verbose("Scene", scroll.amount, " ", scroll.pos);
		return true;
	});

	return true;
}

void GuiSceneContent::handleContentSizeDirty() {
	SceneContent2d::handleContentSizeDirty();

	_layer->setPosition(_contentSize / 2.0f + Vec2(48.0f, 48.0f));
	_layer2->setPosition(_contentSize / 2.0f + Vec2(-48.0f, 48.0f));
	_layer3->setPosition(_contentSize / 2.0f + Vec2(48.0f, -48.0f));
	_layer4->setPosition(_contentSize / 2.0f + Vec2(-48.0f, -48.0f));
}

}
