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

#include "TestLayout.h"
#include "TestAppScene.h"
#include "TestGeneralUpdate.h"
#include "TestGeneralActions.h"
#include "TestMaterialColor.h"
#include "TestMaterialNodes.h"
#include "TestMaterialScroll.h"
#include "TestMaterialInput.h"
#include "TestMaterialMenu.h"
#include "TestMaterialTabBar.h"
#include "TestDynamicFont.h"
#include "TestIcons.h"
#include "TestAutofit.h"
#include "TestZOrder.h"
#include "TestScroll.h"

#include "XL2dVectorSprite.h"
#include "XLIcons.h"

namespace STAPPLER_VERSIONIZED stappler::xenolith::app {

struct MenuData {
	LayoutName layout;
	String id;
	String title;
	Function<Rc<basic2d::SceneLayout2d>(LayoutName)> constructor;
};

static Vector<MenuData> s_layouts {
	MenuData{LayoutName::None, "org.stappler.xenolith.test.None", "None",
		[] (LayoutName name) {
			return Rc<TestLayout>::create(name, "");
	}},
	MenuData{LayoutName::GeneralUpdateTest, "org.stappler.xenolith.test.GeneralUpdateTest", "GeneralUpdateTest",
		[] (LayoutName name) {
			return Rc<TestGeneralUpdate>::create();
	}},
	MenuData{LayoutName::GeneralActionTest, "org.stappler.xenolith.test.GeneralActionTest", "GeneralActionTest",
		[] (LayoutName name) {
			return Rc<TestGeneralAction>::create();
	}},
	MenuData{LayoutName::MaterialColorPickerTest, "org.stappler.xenolith.test.MaterialColorPickerTest", "MaterialColorPickerTest",
		[] (LayoutName name) {
			return Rc<TestMaterialColor>::create();
	}},
	MenuData{LayoutName::MaterialNodeTest, "org.stappler.xenolith.test.MaterialNodeTest", "MaterialNodeTest",
		[] (LayoutName name) {
			return Rc<TestMaterialNodes>::create();
	}},
	MenuData{LayoutName::MaterialScrollTest, "org.stappler.xenolith.test.MaterialScrollTest", "MaterialScrollTest",
		[] (LayoutName name) {
			return Rc<TestMaterialScroll>::create();
	}},
	MenuData{LayoutName::MaterialInputTest, "org.stappler.xenolith.test.MaterialInputTest", "MaterialInputTest",
		[] (LayoutName name) {
			return Rc<TestMaterialInput>::create();
	}},
	MenuData{LayoutName::MaterialMenuTest, "org.stappler.xenolith.test.MaterialMenuTest", "MaterialMenuTest",
		[] (LayoutName name) {
			return Rc<TestMaterialMenu>::create();
	}},
	MenuData{LayoutName::MaterialDynamicFont, "org.stappler.xenolith.test.MaterialDynamicFont", "MaterialDynamicFont",
		[] (LayoutName name) {
			return Rc<TestDynamicFont>::create();
	}},
	MenuData{LayoutName::MaterialTabBar, "org.stappler.xenolith.test.MaterialTabBar", "MaterialTabBar",
		[] (LayoutName name) {
			return Rc<TestMaterialTabBar>::create();
	}},
	MenuData{LayoutName::IconList, "org.stappler.xenolith.test.IconList", "IconList",
		[] (LayoutName name) {
			return Rc<TestIcons>::create();
	}},
	MenuData{LayoutName::Autofit, "org.stappler.xenolith.test.Autofit", "Autofit",
		[] (LayoutName name) {
			return Rc<TestAutofit>::create();
	}},
	MenuData{LayoutName::ZOrder, "org.stappler.xenolith.test.ZOrder", "ZOrder",
		[] (LayoutName name) {
			return Rc<TestZOrder>::create();
	}},
	MenuData{LayoutName::Scroll, "org.stappler.xenolith.test.Scroll", "Scroll",
		[] (LayoutName name) {
			return Rc<TestScroll>::create();
	}},
	MenuData{LayoutName::Empty, "org.stappler.xenolith.test.Empty", "Empty",
		[] (LayoutName name) {
			return Rc<TestEmpty>::create();
	}},
};

StringView getLayoutNameId(LayoutName name) {
	for (auto &it : s_layouts) {
		if (it.layout == name) {
			return it.id;
		}
	}
	return StringView();
}

StringView getLayoutNameTitle(LayoutName name) {
	for (auto &it : s_layouts) {
		if (it.layout == name) {
			return it.title;
		}
	}
	return StringView();
}

Rc<SceneLayout2d> makeLayoutNode(LayoutName name) {
	for (auto &it : s_layouts) {
		if (it.layout == name) {
			return it.constructor(name);
		}
	}
	return nullptr;
}

bool TestLayout::init(LayoutName layout, StringView text) {
	if (!Node::init()) {
		return false;
	}

	_layout = layout;

	_infoLabel = addChild(Rc<Label>::create(), ZOrderMax);
	_infoLabel->setString(text);
	_infoLabel->setAnchorPoint(Anchor::MiddleTop);
	_infoLabel->setAlignment(Label::TextAlign::Center);
	_infoLabel->setFontSize(24);
	_infoLabel->setAdjustValue(16);
	_infoLabel->setMaxLines(4);

	_linearProgress = addChild(Rc<LinearProgress>::create());
	_linearProgress->setLineColor(Color::Red_100);
	_linearProgress->setLineOpacity(1.0f);
	_linearProgress->setBarColor(Color::Amber_100);
	_linearProgress->setBarOpacity(1.0f);
	_linearProgress->setAnchorPoint(Anchor::BottomLeft);

	_roundedProgress = addChild(Rc<RoundedProgress>::create());
	_roundedProgress->setLineColor(Color::Blue_100);
	_roundedProgress->setLineOpacity(1.0f);
	_roundedProgress->setBarColor(Color::Teal_100);
	_roundedProgress->setBarOpacity(1.0f);
	_roundedProgress->setAnchorPoint(Anchor::BottomLeft);
	_roundedProgress->setBorderRadius(2.0f);

	setName(getLayoutNameId(_layout));

	return true;
}

bool TestEmpty::init() {
	if (!TestLayout::init(LayoutName::Empty, "")) {
		return false;
	}

	return true;
}

void TestLayout::onContentSizeDirty() {
	Node::onContentSizeDirty();

	_infoLabel->setPosition(Vec2(_contentSize.width / 2.0f, _contentSize.height - 16.0f));
	_infoLabel->setWidth(_contentSize.width * 3.0f / 4.0f);

	_linearProgress->setAnimated(true);
	_linearProgress->setPosition(Vec2(0.0f, 0.0f));
	_linearProgress->setContentSize(Size2(_contentSize.width, 8.0f));
	_roundedProgress->setPosition(Vec2(0.0f, 8.0f));
	_roundedProgress->setContentSize(Size2(_contentSize.width, 0.0f));
}

void TestLayout::onEnter(xenolith::Scene *scene) {
	Node::onEnter(scene);

	runAction(Rc<ActionProgress>::create(0.6f, [this] (float progress) {
		_roundedProgress->setBarScale(0.5f + 0.5f * progress);
		_roundedProgress->getLayout();
		_roundedProgress->isInverted();
		_linearProgress->isAnimated();
		_linearProgress->getProgress();
		_roundedProgress->getProgress();
		_roundedProgress->getBarScale();
		if (progress < 0.33f) {
			_roundedProgress->setLayout(RoundedProgress::Auto);
		} else if (progress < 0.66) {
			_roundedProgress->setLayout(RoundedProgress::Vertical);
		} else {
			_roundedProgress->setLayout(RoundedProgress::Horizontal);
		}

		if (progress < 0.5f) {
			_roundedProgress->setInverted(false);
			_roundedProgress->setProgress(progress, false);

		} else {
			_roundedProgress->setInverted(true);
			_roundedProgress->setProgress(progress, true);
			_linearProgress->setAnimated(false);
			_linearProgress->setProgress(progress);
		}
	}));
}

void TestLayout::update(const UpdateTime &t) {
	SceneLayout2d::update(t);
}

void TestLayout::setDataValue(Value &&val) {
	Node::setDataValue(move(val));
}

}
