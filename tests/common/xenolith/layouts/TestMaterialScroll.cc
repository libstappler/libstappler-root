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

#include "TestMaterialScroll.h"
#include "MaterialDataScrollHandlerGrid.h"

namespace STAPPLER_VERSIONIZED stappler::xenolith::app {

bool TestMaterialScroll::init() {
	if (!TestMaterial::init(LayoutName::MaterialNodeTest, "")) {
		return false;
	}

	auto data = Rc<material2d::DataSource>::create(
			material2d::DataSource::DataSourceCallback([] (const material2d::DataSource::DataCallback &cb, material2d::DataSource::Id id) {
		Value val;
		val.setInteger(id.get(), "id");
		cb(move(val));
	}), material2d::DataSource::ChildsCount(20));

	_appBar = setFlexibleNode(Rc<material2d::AppBar>::create(material2d::AppBarLayout::Small, material2d::SurfaceStyle(
			material2d::NodeStyle::Filled, material2d::ColorRole::PrimaryContainer)));
	_appBar->setTitle("Test App Bar");
	_appBar->setNavButtonIcon(IconName::Navigation_arrow_back_solid);
	_appBar->setNavCallback([] {

	});
	_appBar->setMaxActionIcons(4);

	auto actionMenu = Rc<material2d::MenuSource>::create();
	actionMenu->addButton("", IconName::Editor_format_align_center_solid, [this] (material2d::Button *, material2d::MenuSourceButton *) {
		if (_appBar->getLayout() == material2d::AppBarLayout::CenterAligned) {
			_appBar->setLayout(material2d::AppBarLayout::Small);
		} else {
			_appBar->setLayout(material2d::AppBarLayout::CenterAligned);
		}
	});
	actionMenu->addButton("", IconName::Editor_vertical_align_top_solid, [] (material2d::Button *, material2d::MenuSourceButton *) {

	});
	actionMenu->addButton("", IconName::Notification_do_disturb_on_outline, [this] (material2d::Button *, material2d::MenuSourceButton *) {
		if (auto content = dynamic_cast<material2d::SceneContent *>(_scene->getContent())) {
			content->showSnackbar(move(material2d::SnackbarData("test shackbar").withButton("Button", IconName::Action_accessibility_solid, [content] () {
				content->showSnackbar(material2d::SnackbarData("updated shackbar", Color::Red_500, 1.0f));
			}, Color::Green_500, 1.0f)));
		}
	});
	actionMenu->addButton("", IconName::Content_file_copy_solid, [this] (material2d::Button *, material2d::MenuSourceButton *) {
		auto view = _director->getView();
		view->readFromClipboard([this] (BytesView view, StringView ct) {
			if (auto content = dynamic_cast<material2d::SceneContent *>(_scene->getContent())) {
				content->showSnackbar(material2d::SnackbarData(view.readString(view.size())));
			}
		}, this);
	});
	_appBar->setActionMenuSource(move(actionMenu));

	_scrollView = setBaseNode(Rc<material2d::DataScroll>::create(data, ScrollView::Vertical));

	_scrollView->setLoaderSize(100.0f);
	_scrollView->setMinLoadTime(TimeInterval::milliseconds(100));

	_scrollView->setLoaderCallback([] (material2d::DataScroll::Request, const Function<void()> &cb) -> Rc<material2d::DataScroll::Loader> {
		return Rc<material2d::DataScroll::Loader>::create(cb);
	});

	_scrollView->setHandlerCallback([] (material2d::DataScroll *scroll) -> Rc<material2d::DataScroll::Handler> {
		auto ret = Rc<material2d::DataScrollHandlerGrid>::create(scroll, Padding(10.0f));
		ret->setCellHeight(250.0f);
		ret->setCellAspectRatio(0.75f);
		ret->setCellMinWidth(250.0f);
		if (!ret->isAutoPaddings()) {
			ret->setAutoPaddings(true);
		}
		return ret;
	});

	_scrollView->setItemCallback([] (material2d::DataScroll::Item *item) -> Rc<material2d::Surface> {
		auto ret = Rc<material2d::Surface>::create(material2d::SurfaceStyle());
		ret->setPosition(item->getPosition());
		ret->setContentSize(item->getContentSize());

		auto l = Rc<Layer>::create(Color(Color::Tone(item->getId() % 12), Color::Level::a200));
		l->setContentSize(item->getContentSize());

		ret->addChild(l, ZOrder(1));

		return ret;
	});

	setFlexibleMinHeight(0.0f);
	setFlexibleMaxHeight(56.0f);

	return true;
}

static InputEventData makeEvent(InputEventName name, Size2 size, float offset) {
	return InputEventData({
		0,
		name,
		InputMouseButton::Touch,
		InputModifier::None,
		float(size.width / 2.0f),
		float(size.height / 2.0f + offset)
	});
}

static float TestMaterialScroll_pause = 0.02f;
static float TestMaterialScroll_step = 30.0f;

void TestMaterialScroll::handleContentSizeDirty() {
	TestMaterial::handleContentSizeDirty();

	auto a = Rc<Sequence>::create(0.15f, [this] {
		_director->getView()->handleInputEvent(makeEvent(InputEventName::Begin, _contentSize, TestMaterialScroll_step * 0));
	}, TestMaterialScroll_pause, [this] {
		_director->getView()->handleInputEvent(makeEvent(InputEventName::Move, _contentSize, TestMaterialScroll_step * 1));
	}, TestMaterialScroll_pause, [this] {
		_director->getView()->handleInputEvent(makeEvent(InputEventName::Move, _contentSize, TestMaterialScroll_step * 2));
	}, TestMaterialScroll_pause, [this] {
		_director->getView()->handleInputEvent(makeEvent(InputEventName::Move, _contentSize, TestMaterialScroll_step * 3));
	}, TestMaterialScroll_pause, [this] {
		_director->getView()->handleInputEvent(makeEvent(InputEventName::Move, _contentSize, TestMaterialScroll_step * 4));
	}, TestMaterialScroll_pause, [this] {
		_director->getView()->handleInputEvent(makeEvent(InputEventName::Move, _contentSize, TestMaterialScroll_step * 5));
	}, TestMaterialScroll_pause, [this] {
		_director->getView()->handleInputEvent(makeEvent(InputEventName::Move, _contentSize, TestMaterialScroll_step * 6));
	}, TestMaterialScroll_pause, [this] {
		_director->getView()->handleInputEvent(makeEvent(InputEventName::Move, _contentSize, TestMaterialScroll_step * 7));
	}, TestMaterialScroll_pause, [this] {
		_director->getView()->handleInputEvent(makeEvent(InputEventName::Move, _contentSize, TestMaterialScroll_step * 8));
	}, TestMaterialScroll_pause, [this] {
		_director->getView()->handleInputEvent(makeEvent(InputEventName::Move, _contentSize, TestMaterialScroll_step * 9));
	}, TestMaterialScroll_pause, [this] {
		_director->getView()->handleInputEvent(makeEvent(InputEventName::Move, _contentSize, TestMaterialScroll_step * 10));
	}, TestMaterialScroll_pause, [this] {
		_director->getView()->handleInputEvent(makeEvent(InputEventName::End, _contentSize, TestMaterialScroll_step * 10));
	});

	runAction(a);
}

void TestMaterialScroll::update(const UpdateTime &) {

}

}
