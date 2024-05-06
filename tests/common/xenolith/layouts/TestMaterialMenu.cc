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

#include "TestMaterialMenu.h"

namespace STAPPLER_VERSIONIZED stappler::xenolith::app {

bool TestMaterialMenu::init() {
	if (!TestMaterial::init(LayoutName::MaterialMenuTest, "")) {
		return false;
	}

	auto source = Rc<material2d::MenuSource>::create();
	auto btn1 = source->addButton("Long long long long long long long menu item name", [] (material2d::Button *b, material2d::MenuSourceButton *) {

	});
	btn1->setNameIcon(IconName::Action_commute_outline);
	btn1->setValueIcon(IconName::Editor_add_comment_outline);
	btn1->setValue("value1");

	auto btn2 = source->addButton("Test2", [] (material2d::Button *b, material2d::MenuSourceButton *) {

	});
	btn2->setValueIcon(IconName::Editor_add_comment_outline);
	btn2->setValue("value2");

	source->addSeparator();
	auto btn3 = source->addButton("Test3");
	btn3->setValue("value3");

	auto subMenuSource = Rc<material2d::MenuSource>::create();
	subMenuSource->addButton("Sub button 1", [] (material2d::Button *b, material2d::MenuSourceButton *) { });
	subMenuSource->addButton("Sub button 2", [] (material2d::Button *b, material2d::MenuSourceButton *) { });
	subMenuSource->addButton("Sub button 3", [] (material2d::Button *b, material2d::MenuSourceButton *) { });

	source->addButton("Sub menu", IconName::Action_calendar_today_outline, move(subMenuSource));

	_menu = _background->addChild(Rc<material2d::Menu>::create(source), ZOrder(1));
	_menu->setAnchorPoint(Anchor::MiddleTop);

	return true;
}

void TestMaterialMenu::onContentSizeDirty() {
	TestMaterial::onContentSizeDirty();

	_menu->setContentSize(Size2(std::min(_contentSize.width - 32.0f, 400.0f), _contentSize.height - 120.0f));
	_menu->setPosition(Vec2(_contentSize.width / 2.0f + 50.0f, _contentSize.height - 96.0f));

	runAction(Rc<Sequence>::create(0.03f, [this] {
		Vector<InputEventData> events {
			InputEventData({
				0,
				InputEventName::Begin,
				InputMouseButton::Touch,
				InputModifier::None,
				float(_contentSize.width / 2.0f),
				float(_contentSize.height / 2.0f + 80.0f)
			}),
			InputEventData({
				0,
				InputEventName::End,
				InputMouseButton::Touch,
				InputModifier::None,
				float(_contentSize.width / 2.0f),
				float(_contentSize.height / 2.0f + 80.0f)
			})
		};

		auto v = _director->getView();
		v->handleInputEvents(move(events));
	}));
}

void TestMaterialMenu::update(const UpdateTime &) {

}

}
