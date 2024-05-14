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

#include "TestDynamicFont.h"
#include "XL2dLabel.h"
#include "XL2dVectorSprite.h"
#include "XL2dCommandList.h"
#include "XLFrameInfo.h"

namespace STAPPLER_VERSIONIZED stappler::xenolith::app {

class VgSdfTestCircle : public VectorSprite {
public:
	virtual ~VgSdfTestCircle() { }

	virtual bool init(bool);

	virtual void pushShadowCommands(FrameInfo &, NodeFlags flags, const Mat4 &,
			SpanView<TransformVertexData>) override;

protected:
	bool _sdfShadow = false;
};

class VgSdfTestRect : public VectorSprite {
public:
	virtual ~VgSdfTestRect() { }

	virtual bool init(bool, float radius = 0.0f);

	virtual void pushShadowCommands(FrameInfo &, NodeFlags flags, const Mat4 &,
			SpanView<TransformVertexData>) override;

protected:
	bool _sdfShadow = false;
	float _radius = 0.0f;
};

class VgSdfTestTriangle : public VectorSprite {
public:
	virtual ~VgSdfTestTriangle() { }

	virtual bool init(bool);

	virtual void pushShadowCommands(FrameInfo &, NodeFlags flags, const Mat4 &,
			SpanView<TransformVertexData>) override;

protected:
	bool _sdfShadow = false;
};

class VgSdfTestPolygon : public VectorSprite {
public:
	virtual ~VgSdfTestPolygon() { }

	virtual bool init(bool);

	virtual void pushShadowCommands(FrameInfo &, NodeFlags flags, const Mat4 &,
			SpanView<TransformVertexData>) override;

protected:
	bool _sdfShadow = false;
};

bool VgSdfTestCircle::init(bool value) {
	if (!VectorSprite::init(Size2(16, 16))) {
		return false;
	}

	_sdfShadow = value;
	_image->addPath()->openForWriting([] (vg::PathWriter &writer) { writer.addCircle(8, 8, 8); });

	setDepthIndex(4.0f);
	setColor(Color::Grey_100);
	setAnchorPoint(Anchor::Middle);

	return true;
}

void VgSdfTestCircle::pushShadowCommands(FrameInfo &frame, NodeFlags flags, const Mat4 &t, SpanView<TransformVertexData> data) {
	if (_sdfShadow) {
		auto ctx = static_cast<FrameContextHandle2d *>(frame.currentContext);
		ctx->shadows->pushSdfGroup(t, ctx->getCurrentState(), frame.depthStack.back(), [&] (CmdSdfGroup2D &cmd) {
			cmd.addCircle2D(_contentSize / 2.0f, min(_contentSize.width, _contentSize.height) / 2.0f);
		});
	} else {
		VectorSprite::pushShadowCommands(frame, flags, t, data);
	}
}

bool VgSdfTestRect::init(bool value, float radius) {
	if (!VectorSprite::init(Size2(16, 8))) {
		return false;
	}

	_sdfShadow = value;
	_radius = radius;

	if (_radius > 0.0f) {
		_image->addPath()->openForWriting([&] (vg::PathWriter &writer) { writer.addRect(Rect(0, 0, 16, 8), radius, radius); });
	} else {
		_image->addPath()->openForWriting([&] (vg::PathWriter &writer) { writer.addRect(Rect(0, 0, 16, 8)); });
	}

	setDepthIndex(4.0f);
	setColor(Color::Grey_100);
	setAnchorPoint(Anchor::Middle);

	return true;
}

void VgSdfTestRect::pushShadowCommands(FrameInfo &frame, NodeFlags flags, const Mat4 &t, SpanView<TransformVertexData> data) {
	if (_sdfShadow) {
		auto ctx = static_cast<FrameContextHandle2d *>(frame.currentContext);
		ctx->shadows->pushSdfGroup(t, ctx->getCurrentState(), frame.depthStack.back(), [&] (CmdSdfGroup2D &cmd) {
			if (_radius > 0.0f) {
				cmd.addRoundedRect2D(Rect(Vec2(0, 0), _contentSize), _radius * (_contentSize.width / 16.0f));
			} else {
				cmd.addRect2D(Rect(Vec2(0, 0), _contentSize));
			}
		});
	} else {
		VectorSprite::pushShadowCommands(frame, flags, t, data);
	}
}

bool VgSdfTestPolygon::init(bool value) {
	if (!VectorSprite::init(Size2(16, 20))) {
		return false;
	}

	_sdfShadow = value;
	_image->addPath()->openForWriting([&] (vg::PathWriter &writer) {
		writer.moveTo(0.0f, 0.0f).lineTo(16, 20).lineTo(0, 20).lineTo(16, 0).closePath();
	}).setAntialiased(false);

	setDepthIndex(4.0f);
	setColor(Color::Grey_100);
	setAnchorPoint(Anchor::Middle);

	return true;
}

void VgSdfTestPolygon::pushShadowCommands(FrameInfo &frame, NodeFlags flags, const Mat4 &t, SpanView<TransformVertexData> data) {
	if (_sdfShadow) {
		auto ctx = static_cast<FrameContextHandle2d *>(frame.currentContext);
		ctx->shadows->pushSdfGroup(t, ctx->getCurrentState(), frame.depthStack.back(), [&] (CmdSdfGroup2D &cmd) {
			Vec2 points[4] = {
				Vec2(0, 0),
				Vec2(_contentSize),
				Vec2(0, _contentSize.height),
				Vec2(_contentSize.width, 0),
			};
			cmd.addPolygon2D(points);
		});
	} else {
		VectorSprite::pushShadowCommands(frame, flags, t, data);
	}
}

bool VgSdfTestTriangle::init(bool value) {
	if (!VectorSprite::init(Size2(16, 16))) {
		return false;
	}

	_sdfShadow = value;
	_image->addPath()->openForWriting([&] (vg::PathWriter &writer) {
		writer.moveTo(0.0f, 0.0f).lineTo(8.0f, 16.0f).lineTo(16.0f, 0.0f).closePath();
	}).setAntialiased(false);

	setDepthIndex(4.0f);
	setColor(Color::Grey_100);
	setAnchorPoint(Anchor::Middle);

	return true;
}

void VgSdfTestTriangle::pushShadowCommands(FrameInfo &frame, NodeFlags flags, const Mat4 &t, SpanView<TransformVertexData> data) {
	if (_sdfShadow) {
		auto ctx = static_cast<FrameContextHandle2d *>(frame.currentContext);
		ctx->shadows->pushSdfGroup(t, ctx->getCurrentState(), frame.depthStack.back(), [&] (CmdSdfGroup2D &cmd) {
			cmd.addTriangle2D(Vec2(0, 0), Vec2(0, 0), Vec2(_contentSize.width / 2.0f, _contentSize.height), Vec2(_contentSize.width, 0));
		});
	} else {
		VectorSprite::pushShadowCommands(frame, flags, t, data);
	}
}

bool TestDynamicFont::init() {
	if (!TestLayout::init(LayoutName::MaterialDynamicFont, "")) {
		return false;
	}

	float initialFontSize = 28.0f;
	float initialFontStyle = 0.0f;

	_label = addChild(Rc<Label>::create(
			"ABCDEFGHIJKLMNOPQRSTUVWXYZ\n"
			"abcdefghijklmnopqrstuvwxyz\n"
			"1234567890!@#$%^&*(){}[],./\\"));
	_label->setFontSize(uint16_t(std::floor(initialFontSize)));
	_label->setFontFamily("sans");
	_label->setAnchorPoint(Anchor::Middle);
	_label->setAlignment(Label::TextAlign::Center);
	_label->setFontStyle(font::FontStyle::FromDegrees(initialFontStyle));


	bool testsVisible = true;

	_circleSprite = addChild(Rc<VgSdfTestCircle>::create(true), ZOrder(1));
	_circleSprite->setContentSize(Size2(64, 64));
	_circleSprite->setVisible(true);

	_circleTestSprite = addChild(Rc<VgSdfTestCircle>::create(false), ZOrder(1));
	_circleTestSprite->setContentSize(Size2(64, 64));
	_circleTestSprite->setVisible(testsVisible);

	_rectSprite = addChild(Rc<VgSdfTestRect>::create(true), ZOrder(1));
	_rectSprite->setContentSize(Size2(64, 32));
	_rectSprite->setVisible(true);

	_rectTestSprite = addChild(Rc<VgSdfTestRect>::create(false), ZOrder(1));
	_rectTestSprite->setContentSize(Size2(64, 32));
	_rectTestSprite->setVisible(testsVisible);

	_roundedRectSprite = addChild(Rc<VgSdfTestRect>::create(true, 2.0f), ZOrder(1));
	_roundedRectSprite->setContentSize(Size2(64, 32));
	_roundedRectSprite->setVisible(true);

	_roundedRectTestSprite = addChild(Rc<VgSdfTestRect>::create(false, 2.0f), ZOrder(1));
	_roundedRectTestSprite->setContentSize(Size2(64, 32));
	_roundedRectTestSprite->setVisible(testsVisible);

	_triangleSprite = addChild(Rc<VgSdfTestTriangle>::create(true), ZOrder(1));
	_triangleSprite->setContentSize(Size2(64, 64));
	_triangleSprite->setVisible(true);

	_triangleTestSprite = addChild(Rc<VgSdfTestTriangle>::create(false), ZOrder(1));
	_triangleTestSprite->setContentSize(Size2(64, 64));
	_triangleTestSprite->setVisible(testsVisible);

	_polygonSprite = addChild(Rc<VgSdfTestPolygon>::create(true), ZOrder(1));
	_polygonSprite->setContentSize(Size2(64, 80));
	_polygonSprite->setVisible(true);

	_polygonTestSprite = addChild(Rc<VgSdfTestPolygon>::create(false), ZOrder(1));
	_polygonTestSprite->setContentSize(Size2(64, 80));
	_polygonTestSprite->setVisible(testsVisible);

	return true;
}

void TestDynamicFont::onContentSizeDirty() {
	TestLayout::onContentSizeDirty();

	_label->setPosition(_contentSize / 2.0f);
	_label->setWidth(_contentSize.width);
}

void TestDynamicFont::onEnter(xenolith::Scene *scene) {
	TestLayout::onEnter(scene);

	_circleSprite->setPosition(Vec2(_contentSize / 2.0f) + Vec2(_contentSize.width / 3.0f, 100.0f));
	_circleTestSprite->setPosition(Vec2(_contentSize / 2.0f) + Vec2(-_contentSize.width / 3.0f, 100.0f));

	_rectSprite->setPosition(Vec2(_contentSize / 2.0f) + Vec2(_contentSize.width / 3.0f, 0.0f));
	_rectTestSprite->setPosition(Vec2(_contentSize / 2.0f) + Vec2(-_contentSize.width / 3.0f, 0.0f));

	_roundedRectSprite->setPosition(Vec2(_contentSize / 2.0f) + Vec2(_contentSize.width / 3.0f, -100.0f));
	_roundedRectTestSprite->setPosition(Vec2(_contentSize / 2.0f) + Vec2(-_contentSize.width / 3.0f, -100.0f));

	_triangleSprite->setPosition(Vec2(_contentSize / 2.0f) + Vec2(_contentSize.width / 6.0f, 100.0f));
	_triangleTestSprite->setPosition(Vec2(_contentSize / 2.0f) + Vec2(-_contentSize.width / 6.0f, 100.0f));

	_polygonSprite->setPosition(Vec2(_contentSize / 2.0f) + Vec2(_contentSize.width / 6.0f, -40.0f));
	_polygonTestSprite->setPosition(Vec2(_contentSize / 2.0f) + Vec2(-_contentSize.width / 6.0f, -40.0f));

	runAction(Rc<ActionProgress>::create(0.5f, [this] (float val) {
		float maxShadow = 20.0f;
		float minFontGrade = -200.0f;
		float maxFontGrade = 150.0f;

		_label->setFontSize(uint16_t(std::floor(val * 40.0f + 28.0f)));
		_label->setFontWeight(font::FontWeight(std::floor(val * 900.0f + 100.0f)));
		_label->setFontStretch(font::FontStretch(std::floor(val * (150.0f - 25.0f) + 25.0f) * 2.0f));
		_label->setFontStyle(font::FontStyle::FromDegrees(-val * 10.0f));
		_label->setFontGrade(font::FontGrade(val * (maxFontGrade - minFontGrade) + minFontGrade));

		_circleSprite->setScaleX(val * 2.9f + 0.1f);
		_circleTestSprite->setScaleX(val * 2.9f + 0.1f);
		_rectSprite->setScaleX(val * 2.9f + 0.1f);
		_rectTestSprite->setScaleX(val * 2.9f + 0.1f);
		_roundedRectSprite->setScaleX(val * 2.9f + 0.1f);
		_roundedRectTestSprite->setScaleX(val * 2.9f + 0.1f);
		_triangleSprite->setScaleX(val * 2.9f + 0.1f);
		_triangleTestSprite->setScaleX(val * 2.9f + 0.1f);
		_polygonSprite->setScaleX(val * 2.9f + 0.1f);
		_polygonTestSprite->setScaleX(val * 2.9f + 0.1f);

		_circleSprite->setScaleY(val * 2.9f + 0.1f);
		_circleTestSprite->setScaleY(val * 2.9f + 0.1f);
		_rectSprite->setScaleY(val * 2.9f + 0.1f);
		_rectTestSprite->setScaleY(val * 2.9f + 0.1f);
		_roundedRectSprite->setScaleY(val * 2.9f + 0.1f);
		_roundedRectTestSprite->setScaleY(val * 2.9f + 0.1f);
		_triangleSprite->setScaleY(val * 2.9f + 0.1f);
		_triangleTestSprite->setScaleY(val * 2.9f + 0.1f);
		_polygonSprite->setScaleY(val * 2.9f + 0.1f);
		_polygonTestSprite->setScaleY(val * 2.9f + 0.1f);

		_circleSprite->setRotation(val * numbers::pi * 2.0f);
		_circleTestSprite->setRotation(val * numbers::pi * 2.0f);
		_rectSprite->setRotation(val * numbers::pi * 2.0f);
		_rectTestSprite->setRotation(val * numbers::pi * 2.0f);
		_roundedRectSprite->setRotation(val * numbers::pi * 2.0f);
		_roundedRectTestSprite->setRotation(val * numbers::pi * 2.0f);
		_triangleSprite->setRotation(val * numbers::pi * 2.0f);
		_triangleTestSprite->setRotation(val * numbers::pi * 2.0f);
		_polygonSprite->setRotation(val * numbers::pi * 2.0f);
		_polygonTestSprite->setRotation(val * numbers::pi * 2.0f);

		_circleSprite->setDepthIndex(val * maxShadow);
		_circleTestSprite->setDepthIndex(val * maxShadow);
		_rectSprite->setDepthIndex(val * maxShadow);
		_rectTestSprite->setDepthIndex(val * maxShadow);
		_roundedRectSprite->setDepthIndex(val * maxShadow);
		_roundedRectTestSprite->setDepthIndex(val * maxShadow);
		_triangleSprite->setDepthIndex(val * maxShadow);
		_triangleTestSprite->setDepthIndex(val * maxShadow);
		_polygonSprite->setDepthIndex(val * maxShadow);
		_polygonTestSprite->setDepthIndex(val * maxShadow);
	}));
}

}
