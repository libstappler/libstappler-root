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

#include "TestMaterialNodes.h"

#include "MaterialLabel.h"
#include "XLInputListener.h"
#include "XLView.h"

namespace STAPPLER_VERSIONIZED stappler::xenolith::app {

static constexpr float NODE_UPDATE_TIME = 0.1f;

class MaterialNodeWithLabel : public material2d::Surface {
public:
	virtual ~MaterialNodeWithLabel() { }

	virtual bool init(const material2d::SurfaceStyle &, StringView);

	virtual void handleContentSizeDirty() override;

protected:
	bool initialize(StringView);

	Label *_label = nullptr;
};

bool MaterialNodeWithLabel::init(const material2d::SurfaceStyle &style, StringView str) {
	if (!Surface::init(style)) {
		return false;
	}

	return initialize(str);
}

bool MaterialNodeWithLabel::initialize(StringView str) {
	_label = addChild(Rc<material2d::TypescaleLabel>::create(material2d::TypescaleRole::TitleLarge, str), ZOrder(1));
	_label->setAnchorPoint(Anchor::Middle);

	return true;
}

void MaterialNodeWithLabel::handleContentSizeDirty() {
	Surface::handleContentSizeDirty();

	_label->setPosition(_contentSize / 2.0f);
}

bool TestMaterialNodes::init() {
	if (!TestMaterial::init(LayoutName::MaterialNodeTest, "")) {
		return false;
	}

	_nodeElevation = addChild(Rc<MaterialNodeWithLabel>::create(material2d::SurfaceStyle{
		material2d::ColorRole::Primary, material2d::Elevation::Level1
	}, "Elevation"), ZOrder(1));
	_nodeElevation->setContentSize(Size2(160.0f, 100.0f));
	_nodeElevation->setAnchorPoint(Anchor::Middle);

	auto el = _nodeElevation->addInputListener(Rc<InputListener>::create());
	el->addTapRecognizer([this] (GestureTap tap) {
		auto style = _nodeElevation->getStyleTarget();
		style.elevation = material2d::Elevation((toInt(style.elevation) + 1) % (toInt(material2d::Elevation::Level5) + 1));
		_nodeElevation->setStyle(move(style), NODE_UPDATE_TIME);

		sendEventForNode(_nodeElevation, 1, NODE_UPDATE_TIME);

		return true;
	}, InputListener::makeButtonMask({InputMouseButton::Touch}), 1);


	_nodeShadow = addChild(Rc<MaterialNodeWithLabel>::create(material2d::SurfaceStyle{
		material2d::ColorRole::Primary, material2d::Elevation::Level3, material2d::NodeStyle::SurfaceTonalElevated
	}, "Shadow"), ZOrder(1));
	_nodeShadow->setContentSize(Size2(160.0f, 100.0f));
	_nodeShadow->setAnchorPoint(Anchor::Middle);

	el = _nodeShadow->addInputListener(Rc<InputListener>::create());
	el->addTapRecognizer([this] (GestureTap tap) {
		auto style = _nodeShadow->getStyleTarget();
		style.elevation = material2d::Elevation((toInt(style.elevation) + 1) % (toInt(material2d::Elevation::Level5) + 1));
		_nodeShadow->setStyle(move(style), NODE_UPDATE_TIME);

		sendEventForNode(_nodeShadow, 2, NODE_UPDATE_TIME);

		return true;
	}, InputListener::makeButtonMask({InputMouseButton::Touch}), 1);


	_nodeCornerRounded = addChild(Rc<MaterialNodeWithLabel>::create(material2d::SurfaceStyle{
		material2d::Elevation::Level3, material2d::ShapeFamily::RoundedCorners, material2d::ShapeStyle::ExtraSmall,
		material2d::NodeStyle::SurfaceTonalElevated
	}, "Rounded"), ZOrder(1));
	_nodeCornerRounded->setContentSize(Size2(160.0f, 100.0f));
	_nodeCornerRounded->setAnchorPoint(Anchor::Middle);

	el = _nodeCornerRounded->addInputListener(Rc<InputListener>::create());
	el->addTapRecognizer([this] (GestureTap tap) {
		auto style = _nodeCornerRounded->getStyleTarget();
		style.shapeStyle = material2d::ShapeStyle((toInt(style.shapeStyle) + 1) % (toInt(material2d::ShapeStyle::Full) + 1));
		_nodeCornerRounded->setStyle(move(style), NODE_UPDATE_TIME);

		sendEventForNode(_nodeCornerRounded, 3, NODE_UPDATE_TIME);

		return true;
	}, InputListener::makeButtonMask({InputMouseButton::Touch}), 1);


	_nodeCornerCut = addChild(Rc<MaterialNodeWithLabel>::create(material2d::SurfaceStyle{
		material2d::Elevation::Level3, material2d::ShapeFamily::CutCorners, material2d::ShapeStyle::ExtraSmall,
		material2d::NodeStyle::SurfaceTonalElevated
	}, "Cut"), ZOrder(1));
	_nodeCornerCut->setContentSize(Size2(160.0f, 100.0f));
	_nodeCornerCut->setAnchorPoint(Anchor::Middle);

	el = _nodeCornerCut->addInputListener(Rc<InputListener>::create());
	el->addTapRecognizer([this] (GestureTap tap) {
		auto style = _nodeCornerCut->getStyleTarget();
		style.shapeStyle = material2d::ShapeStyle((toInt(style.shapeStyle) + 1) % (toInt(material2d::ShapeStyle::Full) + 1));
		_nodeCornerCut->setStyle(move(style), NODE_UPDATE_TIME);

		sendEventForNode(_nodeCornerCut, 4, NODE_UPDATE_TIME);

		return true;
	}, InputListener::makeButtonMask({InputMouseButton::Touch}), 1);


	_nodeStyle = addChild(Rc<MaterialNodeWithLabel>::create(material2d::SurfaceStyle{
		material2d::Elevation::Level5, material2d::NodeStyle::SurfaceTonalElevated, material2d::ShapeStyle::Full, material2d::ActivityState::Enabled
	}, "Style"), ZOrder(1));
	_nodeStyle->setContentSize(Size2(160.0f, 100.0f));
	_nodeStyle->setAnchorPoint(Anchor::Middle);

	el = _nodeStyle->addInputListener(Rc<InputListener>::create());
	el->addTapRecognizer([this] (GestureTap tap) {
		if (tap.input->data.button == InputMouseButton::MouseLeft) {
			auto style = _nodeStyle->getStyleTarget();
			 style.nodeStyle = material2d::NodeStyle((toInt(style.nodeStyle) + 1) % (toInt(material2d::NodeStyle::Text) + 1));
			_nodeStyle->setStyle(move(style), NODE_UPDATE_TIME);
		} else {
			auto style = _nodeStyle->getStyleTarget();
			style.activityState = material2d::ActivityState((toInt(style.activityState) + 1) % (toInt(material2d::ActivityState::Pressed) + 1));
			_nodeStyle->setStyle(move(style), NODE_UPDATE_TIME);
		}

		sendEventForNode(_nodeStyle, 5, NODE_UPDATE_TIME);

		return true;
	}, InputListener::makeButtonMask({InputMouseButton::MouseLeft, InputMouseButton::MouseRight}), 1);

	scheduleUpdate();

	return true;
}

void TestMaterialNodes::handleContentSizeDirty() {
	TestMaterial::handleContentSizeDirty();

	_nodeElevation->setPosition(Vec2(_contentSize / 2.0f) - Vec2(100.0f, 20.0f));
	_nodeShadow->setPosition(Vec2(_contentSize / 2.0f) - Vec2(-100.0f, 20.0f));
	_nodeCornerRounded->setPosition(Vec2(_contentSize / 2.0f) - Vec2(100.0f, -100.0f));
	_nodeCornerCut->setPosition(Vec2(_contentSize / 2.0f) - Vec2(-100.0f, -100.0f));
	_nodeStyle->setPosition(Vec2(_contentSize / 2.0f) - Vec2(100.0f, 140.0f));

	sendEventForNode(_nodeElevation, 1, 0.0f);
	sendEventForNode(_nodeShadow, 2, 0.0f);
	sendEventForNode(_nodeCornerRounded, 3, 0.0f);
	sendEventForNode(_nodeCornerCut, 4, 0.0f);
	sendEventForNode(_nodeStyle, 5, 0.0f);
}

void TestMaterialNodes::update(const UpdateTime &time) {

}

void TestMaterialNodes::sendEventForNode(material2d::Surface *surface, uint32_t id, float d) {
	Vector<InputEventData> events {
		InputEventData({
			id,
			InputEventName::Begin,
			InputMouseButton::Touch,
			InputModifier::None,
			float(surface->getPosition().x),
			float(surface->getPosition().y)
		}),
		InputEventData({
			id,
			InputEventName::End,
			InputMouseButton::Touch,
			InputModifier::None,
			float(surface->getPosition().x),
			float(surface->getPosition().y)
		})
	};

	if (d == 0.0f) {
		auto v = _director->getView();
		v->handleInputEvents(sp::move(events));
	} else {
		runAction(Rc<Sequence>::create(d, [this, events = sp::move(events)] () mutable {
			auto v = _director->getView();
			v->handleInputEvents(sp::move(events));
		}));
	}
}

}
