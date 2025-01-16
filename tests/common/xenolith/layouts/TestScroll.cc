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

#include "TestScroll.h"
#include "XL2dScrollController.h"

namespace STAPPLER_VERSIONIZED stappler::xenolith::app {

bool TestScroll::init() {
	if (!TestLayout::init(LayoutName::Scroll, "")) {
		return false;
	}

	_scrollView = addChild(Rc<ScrollView>::create(ScrollView::Vertical));
	_scrollController = _scrollView->setController(Rc<ScrollController>::create());

	_scrollController->addPlaceholder(10.0f);
	for (uint32_t i = 0; i < 36; ++ i) {
		_scrollController->addItem([i] (const ScrollController::Item &item) {
			return Rc<Layer>::create(Color(Color::Tone(i % 12), Color::Level::a200));
		}, 64.0f, ZOrder(1), toString("Node", i));
	}
	_scrollController->addPlaceholder(10.0f);

	_scrollController->setRebuildCallback([] (ScrollController *c) {
		c->getRebuildCallback();

		c->addPlaceholder(10.0f);
		for (uint32_t i = 0; i < 36; ++ i) {
			c->addItem([i] (const ScrollController::Item &item) {
				return Rc<Layer>::create(Color(Color::Tone(i % 12), Color::Level::a200));
			}, 64.0f, ZOrder(1), toString("Node", i));
		}
		c->addPlaceholder(10.0f);

		return true;
	});

	_scrollView->getOverscrollColor();
	_scrollView->isOverscrollVisible();
	_scrollView->getIndicatorColor();
	_scrollView->setIndicatorVisible(true);
	_scrollView->isIndicatorVisible();

	_scrollView->setOverscrollFrontOffset(10.0f);
	_scrollView->getOverscrollFrontOffset();
	_scrollView->setOverscrollBackOffset(10.0f);
	_scrollView->getOverscrollBackOffset();
	_scrollView->setIndicatorIgnorePadding(true);
	_scrollView->isIndicatorIgnorePadding();
	_scrollView->setTapCallback([] (int count, const Vec2 &loc) {

	});
	_scrollView->getTapCallback();
	_scrollView->setAnimationCallback([] () { });
	_scrollView->getAnimationCallback();
	_scrollView->load(Value());

	return true;
}

void TestScroll::onContentSizeDirty() {
	TestLayout::onContentSizeDirty();

	_scrollView->setContentSize(_contentSize);
}

static float TestScroll_pause = 0.02f;
static float TestScroll_step = 60.0f;

static InputEventData makeScrollEvent(InputEventName name, Size2 size, float offset) {
	return InputEventData({
		0,
		name,
		InputMouseButton::Touch,
		InputModifier::None,
		float(size.width / 2.0f),
		float(size.height / 2.0f + offset)
	});
}

void TestScroll::handleEnter(xenolith::Scene *scene) {
	TestLayout::handleEnter(scene);

	runAction(Rc<Sequence>::create(0.05f, [this] {
		_director->getView()->handleInputEvent(makeScrollEvent(InputEventName::Begin, _contentSize, TestScroll_step * 0));
	}, TestScroll_pause, [this] {
		_director->getView()->handleInputEvent(makeScrollEvent(InputEventName::Move, _contentSize, TestScroll_step * 1));
	}, TestScroll_pause, [this] {
		_director->getView()->handleInputEvent(makeScrollEvent(InputEventName::Move, _contentSize, TestScroll_step * 2));
	}, TestScroll_pause, [this] {
		_director->getView()->handleInputEvent(makeScrollEvent(InputEventName::Move, _contentSize, TestScroll_step * 3));
	}, TestScroll_pause, [this] {
		_director->getView()->handleInputEvent(makeScrollEvent(InputEventName::Move, _contentSize, TestScroll_step * 4));
	}, TestScroll_pause, [this] {
		_director->getView()->handleInputEvent(makeScrollEvent(InputEventName::Move, _contentSize, TestScroll_step * 5));
	}, TestScroll_pause, [this] {
		_director->getView()->handleInputEvent(makeScrollEvent(InputEventName::Move, _contentSize, TestScroll_step * 6));
	}, TestScroll_pause, [this] {
		_director->getView()->handleInputEvent(makeScrollEvent(InputEventName::Move, _contentSize, TestScroll_step * 7));
	}, TestScroll_pause, [this] {
		auto front = _scrollController->getFrontNode();
		auto back = _scrollController->getBackNode();

		auto frontIdx = _scrollController->getItemIndex(front);
		auto backIdx = _scrollController->getItemIndex(front);
		auto frontItem = _scrollController->getItem(front);
		auto backItem = _scrollController->getItem(back);

		_scrollController->resizeItem(frontItem, 68.0f, false);
		_scrollController->resizeItem(backItem, 68.0f, true);
		_scrollController->setKeepNodes(true);
		_scrollController->isKeepNodes();
		_scrollController->getNextItemPosition();
		_scrollController->getRoot();
		_scrollController->getScroll();

		_scrollController->getItem(frontIdx);
		_scrollController->getItem(backIdx);
		_scrollController->getItem(toString("Node", frontIdx));
		_scrollController->getItem(toString("Node", backIdx));
		_scrollController->size();
		_scrollController->getItems();
		_scrollController->getNodes();
		_scrollController->getNodeByName(toString("Node", frontIdx));
		_scrollController->getNodeByName(toString("Node", backIdx));

		_scrollView->removeNode(front, 0.1f);
		_scrollView->resizeNode(back, 10.0f, 0.1f);

		_director->getView()->handleInputEvent(makeScrollEvent(InputEventName::Move, _contentSize, TestScroll_step * 8));
	}, TestScroll_pause, [this] {
		_director->getView()->handleInputEvent(makeScrollEvent(InputEventName::Move, _contentSize, TestScroll_step * 9));
	}, TestScroll_pause, [this] {
		_director->getView()->handleInputEvent(makeScrollEvent(InputEventName::Move, _contentSize, TestScroll_step * 10));
	}, TestScroll_pause, [this] {
		_director->getView()->handleInputEvent(makeScrollEvent(InputEventName::End, _contentSize, TestScroll_step * 10));
		_scrollController->setScrollRelativeValue(1.0f);
		_scrollView->save();
	}));

}

}
