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

#include "TestMaterialColor.h"

#include "MaterialSurfaceInterior.h"
#include "MaterialStyleContainer.h"
#include "MaterialSurface.h"
#include "MaterialLabel.h"

namespace STAPPLER_VERSIONIZED stappler::xenolith::app {

bool TestMaterialColorSchemeNode::init(material2d::ColorRole name) {
	if (!Layer::init()) {
		return false;
	}

	_name = name;
	_labelName = addChild(Rc<Label>::create(), ZOrder(1));
	_labelName->setFontSize(14);
	_labelName->setAnchorPoint(Anchor::TopLeft);
	_labelName->setOpacity(1.0f);

	_labelDesc = addChild(Rc<Label>::create(), ZOrder(1));
	_labelDesc->setFontSize(14);
	_labelDesc->setAnchorPoint(Anchor::BottomRight);
	_labelDesc->setOpacity(1.0f);

	return true;
}

void TestMaterialColorSchemeNode::onContentSizeDirty() {
	Layer::onContentSizeDirty();
	_labelName->setPosition(Vec2(4.0f, _contentSize.height - 2.0f));
	_labelDesc->setPosition(Vec2(_contentSize.width - 4.0f, 4.0f));
}

void TestMaterialColorSchemeNode::setSchemeColor(material2d::ThemeType type, Color4F background, Color4F label) {
	setColor(background);
	_labelName->setColor(label);
	_labelDesc->setColor(label);
	_type = type;
	updateLabels();
}

void TestMaterialColorSchemeNode::updateLabels() {
	switch (_type) {
	case material2d::ThemeType::LightTheme:
	case material2d::ThemeType::Custom:
		switch (_name) {
		case material2d::ColorRole::Primary: _labelName->setString("Primary"); _labelDesc->setString("Primary40"); break;
		case material2d::ColorRole::OnPrimary: _labelName->setString("On Primary"); _labelDesc->setString("Primary100"); break;
		case material2d::ColorRole::PrimaryContainer: _labelName->setString("Primary Container"); _labelDesc->setString("Primary90"); break;
		case material2d::ColorRole::OnPrimaryContainer: _labelName->setString("On Primary Container"); _labelDesc->setString("Primary10"); break;
		case material2d::ColorRole::Secondary: _labelName->setString("Secondary"); _labelDesc->setString("Secondary40"); break;
		case material2d::ColorRole::OnSecondary: _labelName->setString("On Secondary"); _labelDesc->setString("Secondary100"); break;
		case material2d::ColorRole::SecondaryContainer: _labelName->setString("Secondary Container"); _labelDesc->setString("Secondary90"); break;
		case material2d::ColorRole::OnSecondaryContainer: _labelName->setString("On Secondary Container"); _labelDesc->setString("Secondary10"); break;
		case material2d::ColorRole::Tertiary: _labelName->setString("Tertiary"); _labelDesc->setString("Tertiary40"); break;
		case material2d::ColorRole::OnTertiary: _labelName->setString("On Tertiary"); _labelDesc->setString("Tertiary100"); break;
		case material2d::ColorRole::TertiaryContainer: _labelName->setString("Tertiary Container"); _labelDesc->setString("Tertiary90"); break;
		case material2d::ColorRole::OnTertiaryContainer: _labelName->setString("On Tertiary Container"); _labelDesc->setString("Tertiary10"); break;
		case material2d::ColorRole::Error: _labelName->setString("Error"); _labelDesc->setString("Error40"); break;
		case material2d::ColorRole::OnError: _labelName->setString("On Error"); _labelDesc->setString("Error100"); break;
		case material2d::ColorRole::ErrorContainer: _labelName->setString("Error Container"); _labelDesc->setString("Error90"); break;
		case material2d::ColorRole::OnErrorContainer: _labelName->setString("On Error Container"); _labelDesc->setString("Error10"); break;
		case material2d::ColorRole::Background: _labelName->setString("Background"); _labelDesc->setString("Neutral99"); break;
		case material2d::ColorRole::OnBackground: _labelName->setString("On Background"); _labelDesc->setString("Neutral10"); break;
		case material2d::ColorRole::Surface: _labelName->setString("Surface"); _labelDesc->setString("Neutral99"); break;
		case material2d::ColorRole::OnSurface: _labelName->setString("On Surface"); _labelDesc->setString("Neutral10"); break;
		case material2d::ColorRole::SurfaceVariant: _labelName->setString("Surface Variant"); _labelDesc->setString("Neutral-variant90"); break;
		case material2d::ColorRole::OnSurfaceVariant: _labelName->setString("On Surface Variant"); _labelDesc->setString("Neutral-variant30"); break;
		case material2d::ColorRole::Outline: _labelName->setString("Outline"); _labelDesc->setString("Neutral-variant50"); break;
		case material2d::ColorRole::OutlineVariant: _labelName->setString("Outline Variant"); _labelDesc->setString(""); break;
		case material2d::ColorRole::Shadow: _labelName->setString("Shadow"); _labelDesc->setString(""); break;
		case material2d::ColorRole::Scrim: _labelName->setString("Scrim"); _labelDesc->setString(""); break;
		case material2d::ColorRole::InverseSurface: _labelName->setString("Inverse Surface"); _labelDesc->setString(""); break;
		case material2d::ColorRole::InverseOnSurface: _labelName->setString("Inverse On Surface"); _labelDesc->setString(""); break;
		case material2d::ColorRole::InversePrimary: _labelName->setString("Inverse Primary"); _labelDesc->setString(""); break;
		case material2d::ColorRole::Undefined: break;
		case material2d::ColorRole::Max: break;
		}
		break;
	case material2d::ThemeType::DarkTheme:
		switch (_name) {
		case material2d::ColorRole::Primary: _labelName->setString("Primary"); _labelDesc->setString("Primary80"); break;
		case material2d::ColorRole::OnPrimary: _labelName->setString("On Primary"); _labelDesc->setString("Primary20"); break;
		case material2d::ColorRole::PrimaryContainer: _labelName->setString("Primary Container"); _labelDesc->setString("Primary30"); break;
		case material2d::ColorRole::OnPrimaryContainer: _labelName->setString("On Primary Container"); _labelDesc->setString("Primary90"); break;
		case material2d::ColorRole::Secondary: _labelName->setString("Secondary"); _labelDesc->setString("Secondary80"); break;
		case material2d::ColorRole::OnSecondary: _labelName->setString("On Secondary"); _labelDesc->setString("Secondary20"); break;
		case material2d::ColorRole::SecondaryContainer: _labelName->setString("Secondary Container"); _labelDesc->setString("Secondary30"); break;
		case material2d::ColorRole::OnSecondaryContainer: _labelName->setString("On Secondary Container"); _labelDesc->setString("Secondary90"); break;
		case material2d::ColorRole::Tertiary: _labelName->setString("Tertiary"); _labelDesc->setString("Tertiary80"); break;
		case material2d::ColorRole::OnTertiary: _labelName->setString("On Tertiary"); _labelDesc->setString("Tertiary20"); break;
		case material2d::ColorRole::TertiaryContainer: _labelName->setString("Tertiary Container"); _labelDesc->setString("Tertiary30"); break;
		case material2d::ColorRole::OnTertiaryContainer: _labelName->setString("On Tertiary Container"); _labelDesc->setString("Tertiary90"); break;
		case material2d::ColorRole::Error: _labelName->setString("Error"); _labelDesc->setString("Error80"); break;
		case material2d::ColorRole::OnError: _labelName->setString("On Error"); _labelDesc->setString("Error20"); break;
		case material2d::ColorRole::ErrorContainer: _labelName->setString("Error Container"); _labelDesc->setString("Error30"); break;
		case material2d::ColorRole::OnErrorContainer: _labelName->setString("On Error Container"); _labelDesc->setString("Error90"); break;
		case material2d::ColorRole::Background: _labelName->setString("Background"); _labelDesc->setString("Neutral10"); break;
		case material2d::ColorRole::OnBackground: _labelName->setString("On Background"); _labelDesc->setString("Neutral90"); break;
		case material2d::ColorRole::Surface: _labelName->setString("Surface"); _labelDesc->setString("Neutral10"); break;
		case material2d::ColorRole::OnSurface: _labelName->setString("On Surface"); _labelDesc->setString("Neutral90"); break;
		case material2d::ColorRole::SurfaceVariant: _labelName->setString("Surface Variant"); _labelDesc->setString("Neutral-variant30"); break;
		case material2d::ColorRole::OnSurfaceVariant: _labelName->setString("On Surface Variant"); _labelDesc->setString("Neutral-variant80"); break;
		case material2d::ColorRole::Outline: _labelName->setString("Outline"); _labelDesc->setString("Neutral-variant60"); break;
		case material2d::ColorRole::OutlineVariant: _labelName->setString("Outline Variant"); _labelDesc->setString(""); break;
		case material2d::ColorRole::Shadow: _labelName->setString("Shadow"); _labelDesc->setString(""); break;
		case material2d::ColorRole::Scrim: _labelName->setString("Scrim"); _labelDesc->setString(""); break;
		case material2d::ColorRole::InverseSurface: _labelName->setString("Inverse Surface"); _labelDesc->setString(""); break;
		case material2d::ColorRole::InverseOnSurface: _labelName->setString("Inverse On Surface"); _labelDesc->setString(""); break;
		case material2d::ColorRole::InversePrimary: _labelName->setString("Inverse Primary"); _labelDesc->setString(""); break;
		case material2d::ColorRole::Undefined: break;
		case material2d::ColorRole::Max: break;
		}
		break;
	}
}

bool TestMaterialColor::init() {
	if (!TestMaterial::init(LayoutName::MaterialColorPickerTest, "")) {
		return false;
	}

	_colorHct = ColorHCT(Color::Purple_500);

	_style = addComponent(Rc<material2d::StyleContainer>::create());
	_style->setPrimaryScheme(material2d::ThemeType::LightTheme, _colorHct, false);

	addComponent(Rc<material2d::SurfaceInterior>::create(material2d::SurfaceStyle{
		material2d::ColorRole::Primary
	}));

	_background = addChild(Rc<material2d::Surface>::create(material2d::SurfaceStyle::Background), ZOrder(-1));
	_background->setAnchorPoint(Anchor::Middle);

	_lightCheckbox = addChild(Rc<TestCheckboxWithLabel>::create("Dark theme", false, [this] (bool value) {
		if (value) {
			_themeType = material2d::ThemeType::DarkTheme;
		} else {
			_themeType = material2d::ThemeType::LightTheme;
		}
		updateColor(ColorHCT(_colorHct));
	}));
	_lightCheckbox->setAnchorPoint(Anchor::TopLeft);
	_lightCheckbox->setContentSize(Size2(24.0f, 24.0f));

	_contentCheckbox = addChild(Rc<TestCheckboxWithLabel>::create("Content theme", false, [this] (bool value) {
		_isContentColor = value;
		updateColor(ColorHCT(_colorHct));
	}));
	_contentCheckbox->setAnchorPoint(Anchor::TopLeft);
	_contentCheckbox->setContentSize(Size2(24.0f, 24.0f));

	_huePicker = addChild(Rc<MaterialColorPicker>::create(MaterialColorPicker::Hue,_colorHct, [this] (float val) {
		updateColor(ColorHCT(val, _colorHct.data.chroma, _colorHct.data.tone, 1.0f));
	}));
	_huePicker->setAnchorPoint(Anchor::TopLeft);
	_huePicker->setContentSize(Size2(240.0f, 24.0f));

	_chromaPicker = addChild(Rc<MaterialColorPicker>::create(MaterialColorPicker::Chroma,_colorHct, [this] (float val) {
		updateColor(ColorHCT(_colorHct.data.hue, val, _colorHct.data.tone, 1.0f));
	}));
	_chromaPicker->setAnchorPoint(Anchor::TopLeft);
	_chromaPicker->setContentSize(Size2(240.0f, 24.0f));

	_tonePicker = addChild(Rc<MaterialColorPicker>::create(MaterialColorPicker::Tone,_colorHct, [this] (float val) {
		updateColor(ColorHCT(_colorHct.data.hue, _colorHct.data.chroma, val, 1.0f));
	}));
	_tonePicker->setAnchorPoint(Anchor::TopLeft);
	_tonePicker->setContentSize(Size2(240.0f, 24.0f));

	_spriteLayer = addChild(Rc<LayerRounded>::create(_colorHct, 20.0f), ZOrder(-1));
	_spriteLayer->setContentSize(Size2(98, 98));
	_spriteLayer->setAnchorPoint(Anchor::TopLeft);

	for (auto i = toInt(material2d::ColorRole::Primary); i < toInt(material2d::ColorRole::Max); ++ i) {
		auto v = addChild(Rc<TestMaterialColorSchemeNode>::create(material2d::ColorRole(i)));
		v->setAnchorPoint(Anchor::TopLeft);
		_nodes[i] = v;
	}

	updateColor(ColorHCT(_colorHct));

	return true;
}

void TestMaterialColor::handleEnter(Scene *scene) {
	TestMaterial::handleEnter(scene);

	scheduleUpdate();
}

void TestMaterialColor::update(const UpdateTime &t) {
	TestMaterial::update(t);
	_initValue += t.dt;
	_huePicker->setValue(_initValue);
}

void TestMaterialColor::handleExit() {
	TestMaterial::handleExit();
}

void TestMaterialColor::onContentSizeDirty() {
	TestMaterial::onContentSizeDirty();

	_background->setContentSize(_contentSize);
	_background->setPosition(_contentSize / 2.0f);

	_lightCheckbox->setPosition(Vec2(16.0f, _contentSize.height - _decorationPadding.top - 128.0f));
	_contentCheckbox->setPosition(Vec2(240.0f, _contentSize.height - _decorationPadding.top - 128.0f));

	_huePicker->setContentSize(Size2(std::min(std::max(160.0f, _contentSize.width - 200.0f - 98.0f - 48.0f), 360.0f), 24.0f));
	_chromaPicker->setContentSize(Size2(std::min(std::max(160.0f, _contentSize.width - 200.0f - 98.0f - 48.0f), 360.0f), 24.0f));
	_tonePicker->setContentSize(Size2(std::min(std::max(160.0f, _contentSize.width - 200.0f - 98.0f - 48.0f), 360.0f), 24.0f));

	_huePicker->setPosition(Vec2(_spriteLayer->getContentSize().width + 32.0f, _contentSize.height - _decorationPadding.top - 16.0f));
	_chromaPicker->setPosition(Vec2(_spriteLayer->getContentSize().width + 32.0f, _contentSize.height - _decorationPadding.top - 16.0f - 36.0f));
	_tonePicker->setPosition(Vec2(_spriteLayer->getContentSize().width + 32.0f, _contentSize.height - _decorationPadding.top - 16.0f - 72.0f));
	_spriteLayer->setPosition(Vec2(16.0f, _contentSize.height - _decorationPadding.top - 16.0f));

	Vec2 origin(16.0f, _contentSize.height - _decorationPadding.top - 164.0f);
	Size2 size((_contentSize.width - 32.0f) / 4.0f, 48.0f);

	for (auto i = toInt(material2d::ColorRole::Primary); i < toInt(material2d::ColorRole::Max); ++ i) {
		auto row = i % 4;

		_nodes[i]->setContentSize(size);
		_nodes[i]->setPosition(origin + Vec2(row * size.width, 0.0f));

		if (row == 3) {
			origin.y -= size.height + 4.0f;
		}
	}
}

void TestMaterialColor::updateColor(ColorHCT &&color) {
	_colorHct = move(color);
	_spriteLayer->setColor(_colorHct);
	_huePicker->setTargetColor(_colorHct);
	_chromaPicker->setTargetColor(_colorHct);
	_tonePicker->setTargetColor(_colorHct);

	_colorScheme = material2d::ColorScheme(_themeType, _colorHct, _isContentColor);
	_style->setPrimaryScheme(_themeType, _colorHct, _isContentColor);

	for (auto i = toInt(material2d::ColorRole::Primary); i < toInt(material2d::ColorRole::Max); ++ i) {
		_nodes[i]->setSchemeColor(_themeType, _colorScheme.get(material2d::ColorRole(i)), _colorScheme.on(material2d::ColorRole(i)));
	}

	switch (_themeType) {
	case material2d::ThemeType::LightTheme:
	case material2d::ThemeType::Custom:
		_huePicker->setLabelColor(Color::Black);
		_chromaPicker->setLabelColor(Color::Black);
		_tonePicker->setLabelColor(Color::Black);
		_lightCheckbox->setLabelColor(Color::Black);
		_contentCheckbox->setLabelColor(Color::Black);
		break;
	case material2d::ThemeType::DarkTheme:
		_huePicker->setLabelColor(Color::White);
		_chromaPicker->setLabelColor(Color::White);
		_tonePicker->setLabelColor(Color::White);
		_lightCheckbox->setLabelColor(Color::White);
		_contentCheckbox->setLabelColor(Color::White);
		break;
	}
}

}
