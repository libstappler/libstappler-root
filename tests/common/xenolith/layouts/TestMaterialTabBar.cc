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

#include "TestMaterialTabBar.h"

namespace STAPPLER_VERSIONIZED stappler::xenolith::app {

bool TestMaterialTabBar::init() {
	if (!TestMaterial::init(LayoutName::MaterialTabBar, "")) {
		return false;
	}

	auto source = Rc<material2d::MenuSource>::create();
	source->addButton("Test1", IconName::Action_bookmarks_solid, std::bind([] { }))->setSelected(true);
	source->addButton("Test2", IconName::Action_history_solid, std::bind([] { }));
	source->addButton("Test3", IconName::Action_list_solid, std::bind([] { }));

	_tabBar = _background->addChild(Rc<material2d::TabBar>::create(source,
			material2d::TabBar::ButtonStyle::TitleIcon, material2d::TabBar::BarStyle::Layout, material2d::TabBar::Alignment::Justify
	), ZOrder(1));
	_tabBar->setAnchorPoint(Anchor::MiddleTop);

	return true;
}

void TestMaterialTabBar::handleContentSizeDirty() {
	TestMaterial::handleContentSizeDirty();

	_tabBar->setContentSize(Size2(std::min(_contentSize.width - 32.0f, 400.0f), 72.0f));
	_tabBar->setPosition(Vec2(_contentSize.width / 2.0f, _contentSize.height - 96.0f));
}

void TestMaterialTabBar::handleEnter(Scene *scene) {
	TestMaterial::handleEnter(scene);

	runAction(Rc<Sequence>::create(0.1f, [this] {
		sendEventForNode(Vec2(_contentSize.width / 2.0f, _contentSize.height - 120.0f), 0);
	}, 0.1f, [this] {
		sendEventForNode(Vec2(_contentSize.width / 2.0f + 180.0f, _contentSize.height - 120.0f), 0);
	}, 0.1f, [this] {
		sendEventForNode(Vec2(_contentSize.width / 2.0f - 180.0f, _contentSize.height - 120.0f), 0);
	}));
}

void TestMaterialTabBar::update(const UpdateTime &t) {
	TestMaterial::update(t);
}

void TestMaterialTabBar::sendEventForNode(Vec2 pos, uint32_t id) {
	Vector<InputEventData> events {
		InputEventData({
			id,
			InputEventName::Begin,
			InputMouseButton::Touch,
			InputModifier::None,
			float(pos.x),
			float(pos.y)
		}),
		InputEventData({
			id,
			InputEventName::End,
			InputMouseButton::Touch,
			InputModifier::None,
			float(pos.x),
			float(pos.y)
		})
	};

	auto v = _director->getView();
	v->handleInputEvents(sp::move(events));
}

}
