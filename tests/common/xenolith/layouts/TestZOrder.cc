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

#include "TestZOrder.h"
#include "XL2dVectorSprite.h"
#include "XLInputListener.h"
#include "XLIcons.h"

namespace STAPPLER_VERSIONIZED stappler::xenolith::app {

class TestLabel : public Label {
public:
	virtual ~TestLabel() { }

	virtual bool init(StringView str) override {
		if (!Label::init(str)) {
			return false;
		}

		setTextIndent(10.0f);
		setTextTransform(font::TextTransform::Lowercase);
		setHyphens(font::Hyphens::Auto);
		setVerticalAlign(font::VerticalAlign::Middle);
		setLineHeightAbsolute(28.0f);
		setLineHeightRelative(1.5f);
		setMaxChars(1024);
		setOpticalAlignment(true);
		setFillerChar(' ');
		setLocaleEnabled(true);
		setTextDecoration(font::TextDecoration::LineThrough);

		append(u"str%RichTextCopy%test");

		setMarkedColor(Color::Amber_500);
		getMarkedColor();

		return true;
	}

protected:
	virtual void applyLayout(TextLayout *l) override {
		Label::applyLayout(l);

		if (l) {
			l->reserve(0, 0);
			auto it = l->begin();
			while (it != l->end()) {
				l->getLineRect(*it.line, 1.0f);
				++ it;
			}

			l->str(false);
			l->str(0, 20);
			l->getChar(100, 10, font::CharSelectMode::Best);
			l->getChar(100, 10, font::CharSelectMode::Center);
			l->getChar(100, 10, font::CharSelectMode::Prefix);
			l->getChar(100, 10, font::CharSelectMode::Suffix);
			l->getLineForChar(30);
			l->selectWord(30);
			l->getLineRect(1, 1.0f);

			_style.getConfigName(false);

			Style style;
			style.set(font::TextTransform::Lowercase);
			style.set(font::TextTransform::Uppercase);


			DescriptionStyle descStyle;
			descStyle.merge(l->getController(), Style(font::TextTransform::Lowercase));
			descStyle.merge(l->getController(), Style(font::TextDecoration::Underline));
			descStyle.merge(l->getController(), Style(font::Hyphens::Auto));
			descStyle.merge(l->getController(), Style(font::VerticalAlign::Baseline));
			descStyle.merge(l->getController(), Style(Color::Red_500));
			descStyle.merge(l->getController(), Style(OpacityValue(255)));
			descStyle.merge(l->getController(), Style(font::FontSize(28)));
			descStyle.merge(l->getController(), Style(font::FontStyle::Oblique));
			descStyle.merge(l->getController(), Style(font::FontWeight::SemiBold));
			descStyle.merge(l->getController(), Style(font::FontStretch::SemiCondensed));
			descStyle.merge(l->getController(), Style(font::FontGrade::Normal));
			descStyle.merge(l->getController(), Style(FontFamily("default"_tag)));

			if (descStyle == DescriptionStyle() && descStyle != DescriptionStyle()) { }

			ExternalFormatter fmt;
			fmt.init(nullptr, 400.0f, 1.0f);
			fmt.finalize();

			fmt.init(l->getController(), 400.0f, 1.0f);
			fmt.reserve(0, 0);
			fmt.setLineHeightAbsolute(34.0f);
			fmt.addString(descStyle, "Test string", true);
			fmt.setLineHeightRelative(1.5f);
			fmt.addString(descStyle, u"str%RichTextCopy%test", true);
			fmt.finalize();

			Label::getLocalizedString("Test string");
			Label::getLocalizedString("str%RichTextCopy%test");

			Label::getStringWidth(l->getController(), descStyle, "Test string", true);
			Label::getStringWidth(l->getController(), descStyle, "str%RichTextCopy%test", true);
			Label::getStringWidth(nullptr, descStyle, "Test string", true);

			Label::getLabelSize(l->getController(), descStyle, "Test string", 400.0f, true);
			Label::getLabelSize(l->getController(), descStyle, "str%RichTextCopy%test", 400.0f, true);
			Label::getLabelSize(nullptr, descStyle, "Test string", 400.0f, true);
			Label::getLabelSize(nullptr, descStyle, StringView(), 400.0f, true);

		}

		if (!_styleSet) {
			setStyles(getStyles());
			setStyles(move(_styles));
			_styleSet = true;
		}

		getAlignment();
		getWidth();
		getTextIndent();
		getTextTransform();
		getHyphens();
		getVerticalAlign();
		getFontStyle();
		getFontWeight();
		getFontStretch();
		getFontGrade();
		getFontFamily();
		getLineHeight();
		isLineHeightAbsolute();
		getMaxWidth();
		getMaxLines();
		isOpticallyAligned();
		getFillerChar();
		isLocaleEnabled();
		isPersistentLayout();
		getString();
		getString8();
		getStyles();
		getCompiledStyles();
		getTextDecoration();
		isLabelDirty();
		resolveLocaleTags(u"str%RichTextCopy%test\n");
		setString(_string16);
		getAdjustValue();
		isOverflow();
		getCharsCount();
		getLinesCount();
		getLine(0);
		getLine(100);
		getCursorOrigin();
		getCharIndex(Vec2(100.0f, 100.0f), font::CharSelectMode::Best);
		selectWord(23);
		getMaxLineX();
		getSelectionColor();
	}

	bool _styleSet = false;
};

bool TestZOrder::init() {
	if (!TestLayout::init(LayoutName::ZOrder, "")) {
		return false;
	}

	Color colors[5] = {
		Color::Red_500,
		Color::Green_500,
		Color::White,
		Color::Blue_500,
		Color::Teal_500
	};

	int16_t indexes[5] = {
		4, 3, 5, 2, 1
	};

	for (size_t i = 0; i < 5; ++ i) {
		_layers[i] = addChild(Rc<Layer>::create(), ZOrder(indexes[i]));
		_layers[i]->setContentSize(Size2(300, 300));
		_layers[i]->setColor(colors[i]);
		_layers[i]->setAnchorPoint(Anchor::Middle);
	}

	_label = addChild(Rc<TestLabel>::create("Test label str%RichTextCopy%test\n"), ZOrder(6));
	_label->setAnchorPoint(Anchor::Middle);
	_label->appendTextWithStyle("Test label\n", Label::Style(font::FontWeight::Thin));
	_label->appendTextWithStyle(u"Test label\n", Label::Style(font::FontStretch::UltraExpanded));
	_label->prependTextWithStyle("Test label\n", Label::Style(font::FontSize(28)));
	_label->prependTextWithStyle(u"Test label\n", Label::Style(font::TextDecoration::Underline));
	_label->erase16(0, 2);
	_label->erase8(0, 1);

	_label2 = addChild(Rc<Label>::create(), ZOrder(7));
	_label2->setString(u"str%RichTextCopy%test\n");
	_label2->setLocalizedString(0);

	return true;
}

void TestZOrder::onContentSizeDirty() {
	TestLayout::onContentSizeDirty();

	Vec2 center = _contentSize / 2.0f;

	Vec2 positions[5] = {
		center + Vec2(-100, -100),
		center + Vec2(100, -100),
		center,
		center + Vec2(-100, 100),
		center + Vec2(100, 100),
	};

	for (size_t i = 0; i < 5; ++ i) {
		if (_layers[i]) {
			_layers[i]->setPosition(positions[i]);
		}
	}

	_label->setPosition(_contentSize / 2.0f);
	_label2->setPosition(_contentSize / 2.0f + Vec2(0.0f, 200.0f));
}

void TestZOrder::onEnter(xenolith::Scene *scene) {
	TestLayout::onEnter(scene);
}

}
