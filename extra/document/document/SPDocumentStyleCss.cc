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

#include "SPDocumentStyleContainer.h"
#include "SPStringView.h"

namespace STAPPLER_VERSIONIZED stappler::document {

static bool css_readListStyleType(const StringView &value, const StyleCallback &cb) {
	if (value.compare("none")) {
		return cb(StyleParameter::create<ParameterName::CssListStyleType>(ListStyleType::None));
	} else if (value.compare("circle")) {
		return cb(StyleParameter::create<ParameterName::CssListStyleType>(ListStyleType::Circle));
	} else if (value.compare("disc")) {
		return cb(StyleParameter::create<ParameterName::CssListStyleType>(ListStyleType::Disc));
	} else if (value.compare("square")) {
		return cb(StyleParameter::create<ParameterName::CssListStyleType>(ListStyleType::Square));
	} else if (value.compare("x-mdash")) {
		return cb(StyleParameter::create<ParameterName::CssListStyleType>(ListStyleType::XMdash));
	} else if (value.compare("decimal")) {
		return cb(StyleParameter::create<ParameterName::CssListStyleType>(ListStyleType::Decimal));
	} else if (value.compare("decimal-leading-zero")) {
		return cb(StyleParameter::create<ParameterName::CssListStyleType>(ListStyleType::DecimalLeadingZero));
	} else if (value.compare("lower-alpha") || value.compare("lower-latin")) {
		return cb(StyleParameter::create<ParameterName::CssListStyleType>(ListStyleType::LowerAlpha));
	} else if (value.compare("lower-greek")) {
		return cb(StyleParameter::create<ParameterName::CssListStyleType>(ListStyleType::LowerGreek));
	} else if (value.compare("lower-roman")) {
		return cb(StyleParameter::create<ParameterName::CssListStyleType>(ListStyleType::LowerRoman));
	} else if (value.compare("upper-alpha") || value.compare("upper-latin")) {
		return cb(StyleParameter::create<ParameterName::CssListStyleType>(ListStyleType::UpperAlpha));
	} else if (value.compare("upper-roman")) {
		return cb(StyleParameter::create<ParameterName::CssListStyleType>(ListStyleType::UpperRoman));
	}
	return false;
}

template <ParameterName Name>
static bool css_readBorderStyle(const StringView &value, const StyleCallback &cb) {
	if (value.compare("none")) {
		return cb(StyleParameter::create<Name>(BorderStyle::None));
	} else if (value.compare("solid")) {
		return cb(StyleParameter::create<Name>(BorderStyle::Solid));
	} else if (value.compare("dotted")) {
		return cb(StyleParameter::create<Name>(BorderStyle::Dotted));
	} else if (value.compare("dashed")) {
		return cb(StyleParameter::create<Name>(BorderStyle::Dashed));
	}
	return false;
}

template <ParameterName Name>
static bool css_readBorderColor(const StringView &value, const StyleCallback &cb) {
	if (value.compare("transparent")) {
		return cb(StyleParameter::create<Name>(Color4B(255, 255, 255, 0)));
	}

	Color4B color;
	if (geom::readColor(value, color)) {
		return cb(StyleParameter::create<Name>(color));
	}
	return false;
}

template <ParameterName Name>
static bool css_readBorderWidth(const StringView &value, const StyleCallback &cb) {
	if (value.compare("thin")) {
		return cb(StyleParameter::create<Name>(Metric(2.0f, Metric::Units::Px)));
	} else if (value.compare("medium")) {
		return cb(StyleParameter::create<Name>(Metric(4.0f, Metric::Units::Px)));
	} else if (value.compare("thick")) {
		return cb(StyleParameter::create<Name>(Metric(6.0f, Metric::Units::Px)));
	}

	Metric v;
	if (parser::readStyleMetric(value, v)) {
		return cb(StyleParameter::create<Name>(v));
	}
	return false;
}

template <ParameterName Style, ParameterName Color, ParameterName Width>
static bool css_readBorder(const StringView &value, const StyleCallback &cb) {
	bool ret = true;
	value.split<StringView::CharGroup<CharGroupId::WhiteSpace>>([&] (const StringView &str) {
		if (!css_readBorderStyle<Style>(str, cb)) {
			if (!css_readBorderColor<Color>(str, cb)) {
				if (!css_readBorderWidth<Width>(str, cb)) {
					ret = false;
				}
			}
		}
	});
	return ret;
}

template <typename T, typename Getter>
static void css_readQuadValue(const StringView &value, T &top, T &right, T &bottom, T &left, const Getter &g) {
	int count = 0;
	value.split<StringView::CharGroup<CharGroupId::WhiteSpace>>([&] (const StringView &r) {
		count ++;
		if (count == 1) {
			top = right = bottom = left = g(r);
		} else if (count == 2) {
			right = left = g(r);
		} else if (count == 3) {
			bottom = g(r);
		} else if (count == 4) {
			left = g(r);
		}
	});
}

bool css_readAspectRatioValue(StringView str, float &value) {
	float first, second;

	if (!str.readFloat().grab(first)) {
		return false;
	}

	if (str.empty()) {
		value = first;
		return true;
	}

	str.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
	if (str.is('/')) {
		++ str;
		str.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();

		if (!str.readFloat().grab(second)) {
			return false;
		} else {
			value = first / second;
			return true;
		}
	}

	return false;
}

static std::unordered_map<StringView, StyleFunctionPtr> s_cssParameters {
	pair("font-weight", [] (const StringView &value, const StyleCallback &cb, const StringCallback &) {
		if (value.compare("bold")) {
			return cb(StyleParameter::create<ParameterName::CssFontWeight>(FontWeight::Bold));
		} else if (value.compare("normal")) {
			return cb(StyleParameter::create<ParameterName::CssFontWeight>(FontWeight::Normal));
		} else {
			StringView tmp(value);
			if (auto val = tmp.readInteger(10).get(0)) {
				if (val > 0 && val <= 1000) {
					return cb(StyleParameter::create<ParameterName::CssFontWeight>(FontWeight(val)));
				}
			}
		}
		return false;
	}),
	pair("font-stretch", [] (const StringView &value, const StyleCallback &cb, const StringCallback &) {
		if (value.compare("normal")) {
			return cb(StyleParameter::create<ParameterName::CssFontStretch>(FontStretch::Normal));
		} else if (value.compare("ultra-condensed")) {
			return cb(StyleParameter::create<ParameterName::CssFontStretch>(FontStretch::UltraCondensed));
		} else if (value.compare("extra-condensed")) {
			return cb(StyleParameter::create<ParameterName::CssFontStretch>(FontStretch::ExtraCondensed));
		} else if (value.compare("condensed")) {
			return cb(StyleParameter::create<ParameterName::CssFontStretch>(FontStretch::Condensed));
		} else if (value.compare("semi-condensed")) {
			return cb(StyleParameter::create<ParameterName::CssFontStretch>(FontStretch::SemiCondensed));
		} else if (value.compare("semi-expanded")) {
			return cb(StyleParameter::create<ParameterName::CssFontStretch>(FontStretch::SemiExpanded));
		} else if (value.compare("expanded")) {
			return cb(StyleParameter::create<ParameterName::CssFontStretch>(FontStretch::Expanded));
		} else if (value.compare("extra-expanded")) {
			return cb(StyleParameter::create<ParameterName::CssFontStretch>(FontStretch::ExtraExpanded));
		} else if (value.compare("ultra-expanded")) {
			return cb(StyleParameter::create<ParameterName::CssFontStretch>(FontStretch::UltraExpanded));
		} else {
			StringView tmp(value);
			if (auto val = tmp.readFloat().get(0)) {
				tmp.skipChars<StringView::WhiteSpace>();
				if (tmp.is('%') && val >= 50.0f && val <= 200.0f) {
					return cb(StyleParameter::create<ParameterName::CssFontStretch>(FontStretch(uint16_t(val * 2.0f))));
				}
			}
		}
		return false;
	}),
	pair("font-style", [] (const StringView &value, const StyleCallback &cb, const StringCallback &) {
		if (value.compare("italic")) {
			return cb(StyleParameter::create<ParameterName::CssFontStyle>(FontStyle::Italic));
		} else if (value.compare("normal")) {
			return cb(StyleParameter::create<ParameterName::CssFontStyle>(FontStyle::Normal));
		} else if (value.compare("oblique")) {
			return cb(StyleParameter::create<ParameterName::CssFontStyle>(FontStyle::Oblique));
		} else {
			StringView tmp(value);
			if (value.starts_with("oblique")) {
				tmp += "oblique"_len;
			}
			tmp.skipChars<StringView::WhiteSpace>();
			auto val = tmp.readFloat().get(nan());
			if (!std::isnan(val)) {
				tmp.skipChars<StringView::WhiteSpace>();
				if (tmp.is("deg") && val >= -90.0 && val <= 90.0) {
					return cb(StyleParameter::create<ParameterName::CssFontStyle>(FontStyle(uint16_t(val * (1 << 6)))));
				}
			}
		}
		return false;
	}),
	pair("font-size", [] (const StringView &value, const StyleCallback &cb, const StringCallback &) {
		if (value.compare("xx-small")) {
			return cb(StyleParameter::create<ParameterName::CssFontSize>(FontSize::XXSmall));
		} else if (value.compare("x-small")) {
			return cb(StyleParameter::create<ParameterName::CssFontSize>(FontSize::XSmall));
		} else if (value.compare("small")) {
			return cb(StyleParameter::create<ParameterName::CssFontSize>(FontSize::Small));
		} else if (value.compare("medium")) {
			return cb(StyleParameter::create<ParameterName::CssFontSize>(FontSize::Medium));
		} else if (value.compare("large")) {
			return cb(StyleParameter::create<ParameterName::CssFontSize>(FontSize::Large));
		} else if (value.compare("x-large")) {
			return cb(StyleParameter::create<ParameterName::CssFontSize>(FontSize::XLarge));
		} else if (value.compare("xx-large")) {
			return cb(StyleParameter::create<ParameterName::CssFontSize>(FontSize::XXLarge));
		} else if (value.compare("larger")) {
			return cb(StyleParameter::create<ParameterName::CssFontSizeIncrement>(Metric(1.15f, Metric::Em)));
		} else if (value.compare("x-larger")) {
			return cb(StyleParameter::create<ParameterName::CssFontSizeIncrement>(Metric(1.3f, Metric::Em)));
		} else if (value.compare("smaller")) {
			return cb(StyleParameter::create<ParameterName::CssFontSizeIncrement>(Metric(0.85f, Metric::Em)));
		} else if (value.compare("x-smaller")) {
			return cb(StyleParameter::create<ParameterName::CssFontSizeIncrement>(Metric(0.7f, Metric::Em)));
		} else {
			Metric fontSize;
			if (parser::readStyleMetric(value, fontSize)) {
				if (fontSize.metric == Metric::Units::Px) {
					return cb(StyleParameter::create<ParameterName::CssFontSize>(FontSize(fontSize.value)));
				} else if (fontSize.metric == Metric::Units::Em) {
					return cb(StyleParameter::create<ParameterName::CssFontSize>(FontSize(FontSize::Medium.get() * fontSize.value)));
				}
			}
		}
		return false;
	}),
	pair("font-variant", [] (const StringView &value, const StyleCallback &cb, const StringCallback &) {
		if (value.compare("small-caps")) {
			return cb(StyleParameter::create<ParameterName::CssFontVariant>(FontVariant::SmallCaps));
		} else if (value.compare("normal")) {
			return cb(StyleParameter::create<ParameterName::CssFontVariant>(FontVariant::Normal));
		}
		return false;
	}),
	pair("text-decoration", [] (const StringView &value, const StyleCallback &cb, const StringCallback &) {
		if (value.compare("underline")) {
			return cb(StyleParameter::create<ParameterName::CssTextDecoration>(TextDecoration::Underline));
		} else if (value.compare("line-through")) {
			return cb(StyleParameter::create<ParameterName::CssTextDecoration>(TextDecoration::LineThrough));
		} else if (value.compare("overline")) {
			return cb(StyleParameter::create<ParameterName::CssTextDecoration>(TextDecoration::Overline));
		} else if (value.compare("none")) {
			return cb(StyleParameter::create<ParameterName::CssTextDecoration>(TextDecoration::None));
		}
		return false;
	}),
	pair("text-transform", [] (const StringView &value, const StyleCallback &cb, const StringCallback &) {
		if (value.compare("uppercase")) {
			return cb(StyleParameter::create<ParameterName::CssTextTransform>(TextTransform::Uppercase));
		} else if (value.compare("lowercase")) {
			return cb(StyleParameter::create<ParameterName::CssTextTransform>(TextTransform::Lowercase));
		} else if (value.compare("none")) {
			return cb(StyleParameter::create<ParameterName::CssTextTransform>(TextTransform::None));
		}
		return false;
	}),
	pair("text-align", [] (const StringView &value, const StyleCallback &cb, const StringCallback &) {
		if (value.compare("left")) {
			return cb(StyleParameter::create<ParameterName::CssTextAlign>(TextAlign::Left));
		} else if (value.compare("right")) {
			return cb(StyleParameter::create<ParameterName::CssTextAlign>(TextAlign::Right));
		} else if (value.compare("center")) {
			return cb(StyleParameter::create<ParameterName::CssTextAlign>(TextAlign::Center));
		} else if (value.compare("justify")) {
			return cb(StyleParameter::create<ParameterName::CssTextAlign>(TextAlign::Justify));
		}
		return false;
	}),
	pair("white-space", [] (const StringView &value, const StyleCallback &cb, const StringCallback &) {
		if (value.compare("normal")) {
			return cb(StyleParameter::create<ParameterName::CssWhiteSpace>(WhiteSpace::Normal));
		} else if (value.compare("nowrap")) {
			return cb(StyleParameter::create<ParameterName::CssWhiteSpace>(WhiteSpace::Nowrap));
		} else if (value.compare("pre")) {
			return cb(StyleParameter::create<ParameterName::CssWhiteSpace>(WhiteSpace::Pre));
		} else if (value.compare("pre-line")) {
			return cb(StyleParameter::create<ParameterName::CssWhiteSpace>(WhiteSpace::PreLine));
		} else if (value.compare("pre-wrap")) {
			return cb(StyleParameter::create<ParameterName::CssWhiteSpace>(WhiteSpace::PreWrap));
		}
		return false;
	}),
	pair("hyphens", [] (const StringView &value, const StyleCallback &cb, const StringCallback &) {
		if (value.compare("none")) {
			return cb(StyleParameter::create<ParameterName::CssHyphens>(Hyphens::None));
		} else if (value.compare("manual")) {
			return cb(StyleParameter::create<ParameterName::CssHyphens>(Hyphens::Manual));
		} else if (value.compare("auto")) {
			return cb(StyleParameter::create<ParameterName::CssHyphens>(Hyphens::Auto));
		}
		return false;
	}),
	pair("-epub-hyphens", [] (const StringView &value, const StyleCallback &cb, const StringCallback &) {
		if (value.compare("none")) {
			return cb(StyleParameter::create<ParameterName::CssHyphens>(Hyphens::None));
		} else if (value.compare("manual")) {
			return cb(StyleParameter::create<ParameterName::CssHyphens>(Hyphens::Manual));
		} else if (value.compare("auto")) {
			return cb(StyleParameter::create<ParameterName::CssHyphens>(Hyphens::Auto));
		}
		return false;
	}),
	pair("display", [] (const StringView &value, const StyleCallback &cb, const StringCallback &) {
		if (value.compare("none")) {
			return cb(StyleParameter::create<ParameterName::CssDisplay>(Display::None));
		} else if (value.compare("run-in")) {
			return cb(StyleParameter::create<ParameterName::CssDisplay>(Display::RunIn));
		} else if (value.compare("list-item")) {
			return cb(StyleParameter::create<ParameterName::CssDisplay>(Display::ListItem));
		} else if (value.compare("inline")) {
			return cb(StyleParameter::create<ParameterName::CssDisplay>(Display::Inline));
		} else if (value.compare("inline-block")) {
			return cb(StyleParameter::create<ParameterName::CssDisplay>(Display::InlineBlock));
		} else if (value.compare("block")) {
			return cb(StyleParameter::create<ParameterName::CssDisplay>(Display::Block));
		}
		return false;
	}),
	pair("list-style-type", [] (const StringView &value, const StyleCallback &cb, const StringCallback &) {
		return css_readListStyleType(value, cb);
	}),
	pair("list-style-position", [] (const StringView &value, const StyleCallback &cb, const StringCallback &) {
		if (value.compare("inside")) {
			return cb(StyleParameter::create<ParameterName::CssListStylePosition>(ListStylePosition::Inside));
		} else if (value.compare("outside")) {
			return cb(StyleParameter::create<ParameterName::CssListStylePosition>(ListStylePosition::Outside));
		}
		return false;
	}),
	pair("list-style", [] (const StringView &value, const StyleCallback &cb, const StringCallback &) {
		bool ret = true;
		value.split<StringView::CharGroup<CharGroupId::WhiteSpace>>([&] (const StringView &r) {
			if (!css_readListStyleType(r, cb)) {
				if (r.compare("inside")) {
					cb(StyleParameter::create<ParameterName::CssListStylePosition>(ListStylePosition::Inside));
				} else if (r.compare("outside")) {
					cb(StyleParameter::create<ParameterName::CssListStylePosition>(ListStylePosition::Outside));
				}
				ret = false;
			}
		});
		return ret;
	}),
	pair("x-list-style-offset", [] (const StringView &value, const StyleCallback &cb, const StringCallback &) {
		Metric data;
		if (parser::readStyleMetric(value, data)) {
			return cb(StyleParameter::create<ParameterName::CssXListStyleOffset>(data));
		}
		return false;
	}),
	pair("float", [] (const StringView &value, const StyleCallback &cb, const StringCallback &) {
		if (value.compare("none")) {
			return cb(StyleParameter::create<ParameterName::CssFloat>(Float::None));
		} else if (value.compare("left")) {
			return cb(StyleParameter::create<ParameterName::CssFloat>(Float::Left));
		} else if (value.compare("right")) {
			return cb(StyleParameter::create<ParameterName::CssFloat>(Float::Right));
		}
		return false;
	}),
	pair("clear", [] (const StringView &value, const StyleCallback &cb, const StringCallback &) {
		if (value.compare("none")) {
			return cb(StyleParameter::create<ParameterName::CssClear>(Clear::None));
		} else if (value.compare("left")) {
			return cb(StyleParameter::create<ParameterName::CssClear>(Clear::Left));
		} else if (value.compare("right")) {
			return cb(StyleParameter::create<ParameterName::CssClear>(Clear::Right));
		} else if (value.compare("both")) {
			return cb(StyleParameter::create<ParameterName::CssClear>(Clear::Both));
		}
		return false;
	}),
	pair("opacity", [] (const StringView &value, const StyleCallback &cb, const StringCallback &) {
		bool ret = false;
		StringView(value).readFloat().unwrap([&] (float data) {
			ret = cb(StyleParameter::create<ParameterName::CssOpacity>((uint8_t)(math::clamp(data, 0.0f, 1.0f) * 255.0f)));
		});
		return ret;
	}),
	pair("color", [] (const StringView &value, const StyleCallback &cb, const StringCallback &) {
		Color4B color;
		if (readColor(value, color)) {
			cb(StyleParameter::create<ParameterName::CssColor>(Color3B(color.r, color.g, color.b)));
			if (color.a != 255) {
				cb(StyleParameter::create<ParameterName::CssOpacity>(color.a));
			}
			return true;
		}
		return false;
	}),
	pair("text-indent", [] (const StringView &value, const StyleCallback &cb, const StringCallback &) {
		Metric data;
		if (parser::readStyleMetric(value, data)) {
			return cb(StyleParameter::create<ParameterName::CssTextIndent>(data));
		}
		return false;
	}),
	pair("line-height", [] (const StringView &value, const StyleCallback &cb, const StringCallback &) {
		Metric data;
		if (parser::readStyleMetric(value, data, false, true)) {
			return cb(StyleParameter::create<ParameterName::CssLineHeight>(data));
		}
		return false;
	}),
	pair("margin", [] (const StringView &value, const StyleCallback &cb, const StringCallback &) {
		Metric top, right, bottom, left;
		if (parser::readStyleMargin(value, top, right, bottom, left)) {
			cb(StyleParameter::create<ParameterName::CssMarginTop>(top));
			cb(StyleParameter::create<ParameterName::CssMarginRight>(right));
			cb(StyleParameter::create<ParameterName::CssMarginBottom>(bottom));
			cb(StyleParameter::create<ParameterName::CssMarginLeft>(left));
			return true;
		}
		return false;
	}),
	pair("margin-top", [] (const StringView &value, const StyleCallback &cb, const StringCallback &) {
		Metric data;
		if (parser::readStyleMetric(value, data)) {
			return cb(StyleParameter::create<ParameterName::CssMarginTop>(data));
		}
		return false;
	}),
	pair("margin-right", [] (const StringView &value, const StyleCallback &cb, const StringCallback &) {
		Metric data;
		if (parser::readStyleMetric(value, data)) {
			return cb(StyleParameter::create<ParameterName::CssMarginRight>(data));
		}
		return false;
	}),
	pair("margin-bottom", [] (const StringView &value, const StyleCallback &cb, const StringCallback &) {
		Metric data;
		if (parser::readStyleMetric(value, data)) {
			return cb(StyleParameter::create<ParameterName::CssMarginBottom>(data));
		}
		return false;
	}),
	pair("margin-left", [] (const StringView &value, const StyleCallback &cb, const StringCallback &) {
		Metric data;
		if (parser::readStyleMetric(value, data)) {
			return cb(StyleParameter::create<ParameterName::CssMarginLeft>(data));
		}
		return false;
	}),
	pair("width", [] (const StringView &value, const StyleCallback &cb, const StringCallback &) {
		Metric data;
		if (parser::readStyleMetric(value, data)) {
			return cb(StyleParameter::create<ParameterName::CssWidth>(data));
		}
		return false;
	}),
	pair("height", [] (const StringView &value, const StyleCallback &cb, const StringCallback &) {
		Metric data;
		if (parser::readStyleMetric(value, data)) {
			return cb(StyleParameter::create<ParameterName::CssHeight>(data));
		}
		return false;
	}),
	pair("min-width", [] (const StringView &value, const StyleCallback &cb, const StringCallback &) {
		Metric data;
		if (parser::readStyleMetric(value, data)) {
			return cb(StyleParameter::create<ParameterName::CssMinWidth>(data));
		}
		return false;
	}),
	pair("min-height", [] (const StringView &value, const StyleCallback &cb, const StringCallback &) {
		Metric data;
		if (parser::readStyleMetric(value, data)) {
			return cb(StyleParameter::create<ParameterName::CssMinHeight>(data));
		}
		return false;
	}),
	pair("max-width", [] (const StringView &value, const StyleCallback &cb, const StringCallback &) {
		Metric data;
		if (parser::readStyleMetric(value, data)) {
			return cb(StyleParameter::create<ParameterName::CssMaxWidth>(data));
		}
		return false;
	}),
	pair("max-height", [] (const StringView &value, const StyleCallback &cb, const StringCallback &) {
		Metric data;
		if (parser::readStyleMetric(value, data)) {
			return cb(StyleParameter::create<ParameterName::CssMaxHeight>(data));
		}
		return false;
	}),
	pair("padding", [] (const StringView &value, const StyleCallback &cb, const StringCallback &) {
		Metric top, right, bottom, left;
		if (parser::readStyleMargin(value, top, right, bottom, left)) {
			cb(StyleParameter::create<ParameterName::CssPaddingTop>(top));
			cb(StyleParameter::create<ParameterName::CssPaddingRight>(right));
			cb(StyleParameter::create<ParameterName::CssPaddingBottom>(bottom));
			cb(StyleParameter::create<ParameterName::CssPaddingLeft>(left));
			return true;
		}
		return false;
	}),
	pair("padding-top", [] (const StringView &value, const StyleCallback &cb, const StringCallback &) {
		Metric data;
		if (parser::readStyleMetric(value, data)) {
			return cb(StyleParameter::create<ParameterName::CssPaddingTop>(data));
		}
		return false;
	}),
	pair("padding-right", [] (const StringView &value, const StyleCallback &cb, const StringCallback &) {
		Metric data;
		if (parser::readStyleMetric(value, data)) {
			return cb(StyleParameter::create<ParameterName::CssPaddingRight>(data));
		}
		return false;
	}),
	pair("padding-bottom", [] (const StringView &value, const StyleCallback &cb, const StringCallback &) {
		Metric data;
		if (parser::readStyleMetric(value, data)) {
			return cb(StyleParameter::create<ParameterName::CssPaddingBottom>(data));
		}
		return false;
	}),
	pair("padding-left", [] (const StringView &value, const StyleCallback &cb, const StringCallback &) {
		Metric data;
		if (parser::readStyleMetric(value, data)) {
			return cb(StyleParameter::create<ParameterName::CssPaddingLeft>(data));
		}
		return false;
	}),
	pair("font-family", [] (const StringView &value, const StyleCallback &cb, const StringCallback &strCb) {
		if (value.compare("default") || value.compare("serif")) {
			return cb(StyleParameter::create<ParameterName::CssFontFamily>(StringIdNone));
		} else {
			auto str = StyleContainer::resolveCssString(value);
			if (!str.empty()) {
				return cb(StyleParameter::create<ParameterName::CssFontFamily>(strCb(str)));
			}
		}
		return false;
	}),
	pair("background-color", [] (const StringView &value, const StyleCallback &cb, const StringCallback &strCb) {
		if (value.compare("transparent")) {
			return cb(StyleParameter::create<ParameterName::CssBackgroundColor>(Color4B(0, 0, 0, 0)));
		} else {
			Color4B color;
			if (readColor(value, color)) {
				return cb(StyleParameter::create<ParameterName::CssBackgroundColor>(color));
			}
		}
		return false;
	}),
	pair("background-image", [] (const StringView &value, const StyleCallback &cb, const StringCallback &strCb) {
		if (value.compare("none")) {
			return cb(StyleParameter::create<ParameterName::CssBackgroundImage>(StringIdNone));
		} else {
			StringView tmp(value);
			tmp.trimChars<StringView::WhiteSpace>();
			if (tmp.starts_with("url")) {
				tmp += "url"_len;
			}
			tmp = StyleContainer::resolveCssString(tmp);

			if (!tmp.empty()) {
				return cb(StyleParameter::create<ParameterName::CssBackgroundImage>(strCb(tmp)));
			}
		}
		return false;
	}),
	pair("background-position", [] (const StringView &value, const StyleCallback &cb, const StringCallback &strCb) {
		StringView first, second;
		Metric x, y;
		bool validX = false, validY = false, swapValues = false;

		value.split<StringView::WhiteSpace>([&] (const StringView r) {
			if (first.empty()) {
				first = r;
			} else {
				second = r;
			}
		});

		if (!first.empty() && !second.empty()) {
			bool parseError = false;
			bool firstWasCenter = false;
			if (first.compare("center")) {
				x.value = 0.5f; x.metric = Metric::Units::Percent; validX = true; firstWasCenter = true;
			} else if (first.compare("left")) {
				x.value = 0.0f; x.metric = Metric::Units::Percent; validX = true;
			} else if (first.compare("right")) {
				x.value = 1.0f; x.metric = Metric::Units::Percent; validX = true;
			} else if (first.compare("top")) {
				x.value = 0.0f; x.metric = Metric::Units::Percent; validX = true; swapValues = true;
			} else if (first.compare("bottom")) {
				x.value = 1.0f; x.metric = Metric::Units::Percent; validX = true; swapValues = true;
			}

			if (second.compare("center")) {
				y.value = 0.5f; y.metric = Metric::Units::Percent; validY = true;
			} else if (second.compare("left")) {
				if (swapValues || firstWasCenter) {
					y.value = 0.0f; y.metric = Metric::Units::Percent; validY = true; swapValues = true;
				} else {
					parseError = true;
				}
			} else if (second.compare("right")) {
				if (swapValues || firstWasCenter) {
					y.value = 1.0f; y.metric = Metric::Units::Percent; validY = true; swapValues = true;
				} else {
					parseError = true;
				}
			} else if (second.compare("top")) {
				if (!swapValues) {
					y.value = 0.0f; y.metric = Metric::Units::Percent; validY = true; swapValues = true;
				} else {
					parseError = true;
				}
			} else if (second.compare("bottom")) {
				if (!swapValues) {
					y.value = 1.0f; y.metric = Metric::Units::Percent; validY = true; swapValues = true;
				} else {
					parseError = true;
				}
			}

			if (!parseError && !validX) {
				if (parser::readStyleMetric(first, x)) {
					validX = true;
				}
			}

			if (!parseError && !validY) {
				if (parser::readStyleMetric(second, y)) {
					validY = true;
				}
			}
		} else {
			if (value.compare("center")) {
				x.value = 0.5f; x.metric = Metric::Units::Percent; validX = true;
				y.value = 0.5f; y.metric = Metric::Units::Percent; validY = true;
			} else if (value.compare("top")) {
				x.value = 0.5f; x.metric = Metric::Units::Percent; validX = true;
				y.value = 0.0f; y.metric = Metric::Units::Percent; validY = true;
			} else if (value.compare("right")) {
				x.value = 1.0f; x.metric = Metric::Units::Percent; validX = true;
				y.value = 0.5f; y.metric = Metric::Units::Percent; validY = true;
			} else if (value.compare("bottom")) {
				x.value = 0.5f; x.metric = Metric::Units::Percent; validX = true;
				y.value = 1.0f; y.metric = Metric::Units::Percent; validY = true;
			} else if (value.compare("left")) {
				x.value = 0.0f; x.metric = Metric::Units::Percent; validX = true;
				y.value = 0.5f; y.metric = Metric::Units::Percent; validY = true;
			}
		}

		if (validX && validY) {
			if (!swapValues) {
				return cb(StyleParameter::create<ParameterName::CssBackgroundPositionX>(x))
						&& cb(StyleParameter::create<ParameterName::CssBackgroundPositionY>(y));
			} else {
				return cb(StyleParameter::create<ParameterName::CssBackgroundPositionX>(y))
						&& cb(StyleParameter::create<ParameterName::CssBackgroundPositionY>(x));
			}
		}
		return false;
	}),
	pair("background-repeat", [] (const StringView &value, const StyleCallback &cb, const StringCallback &strCb) {
		if (value.compare("no-repeat")) {
			return cb(StyleParameter::create<ParameterName::CssBackgroundRepeat>(BackgroundRepeat::NoRepeat));
		} else if (value.compare("repeat")) {
			return cb(StyleParameter::create<ParameterName::CssBackgroundRepeat>(BackgroundRepeat::Repeat));
		} else if (value.compare("repeat-x")) {
			return cb(StyleParameter::create<ParameterName::CssBackgroundRepeat>(BackgroundRepeat::RepeatX));
		} else if (value.compare("repeat-y")) {
			return cb(StyleParameter::create<ParameterName::CssBackgroundRepeat>(BackgroundRepeat::RepeatY));
		}
		return false;
	}),
	pair("background-size", [] (const StringView &value, const StyleCallback &cb, const StringCallback &strCb) {
		StringView first, second;
		Metric width, height;
		bool validWidth = false, validHeight = false;

		value.split<StringView::WhiteSpace>([&] (const StringView r) {
			if (first.empty()) {
				first = r;
			} else {
				second = r;
			}
		});

		if (value.compare("contain")) {
			width.metric = Metric::Units::Contain; validWidth = true;
			height.metric = Metric::Units::Contain; validHeight = true;
		} else if (value.compare("cover")) {
			width.metric = Metric::Units::Cover; validWidth = true;
			height.metric = Metric::Units::Cover; validHeight = true;
		} else if (!first.empty() && !second.empty()) {
			if (first.compare("contain")) {
				width.metric = Metric::Units::Contain; validWidth = true;
			} else if (first.compare("cover")) {
				width.metric = Metric::Units::Cover; validWidth = true;
			} else if (parser::readStyleMetric(first, width)) {
				validWidth = true;
			}

			if (second.compare("contain")) {
				height.metric = Metric::Units::Contain; validWidth = true;
			} else if (second.compare("cover")) {
				height.metric = Metric::Units::Cover; validWidth = true;
			} else if (parser::readStyleMetric(second, height)) {
				validHeight = true;
			}
		} else if (parser::readStyleMetric(value, width)) {
			height.metric = Metric::Units::Auto; validWidth = true; validHeight = true;
		}

		if (validWidth && validHeight) {
			return cb(StyleParameter::create<ParameterName::CssBackgroundSizeWidth>(width))
					&& cb(StyleParameter::create<ParameterName::CssBackgroundSizeHeight>(height));
		}
		return false;
	}),
	pair("vertical-align", [] (const StringView &value, const StyleCallback &cb, const StringCallback &strCb) {
		if (value.compare("baseline")) {
			return cb(StyleParameter::create<ParameterName::CssVerticalAlign>(VerticalAlign::Baseline));
		} else if (value.compare("sub")) {
			return cb(StyleParameter::create<ParameterName::CssVerticalAlign>(VerticalAlign::Sub));
		} else if (value.compare("super")) {
			return cb(StyleParameter::create<ParameterName::CssVerticalAlign>(VerticalAlign::Super));
		} else if (value.compare("middle")) {
			return cb(StyleParameter::create<ParameterName::CssVerticalAlign>(VerticalAlign::Middle));
		} else if (value.compare("top")) {
			return cb(StyleParameter::create<ParameterName::CssVerticalAlign>(VerticalAlign::Top));
		} else if (value.compare("bottom")) {
			return cb(StyleParameter::create<ParameterName::CssVerticalAlign>(VerticalAlign::Bottom));
		}
		return false;
	}),
	pair("outline", [] (const StringView &value, const StyleCallback &cb, const StringCallback &strCb) {
		return css_readBorder<ParameterName::CssOutlineStyle, ParameterName::CssOutlineColor, ParameterName::CssOutlineWidth>(value, cb);
	}),
	pair("outline-style", [] (const StringView &value, const StyleCallback &cb, const StringCallback &strCb) {
		return css_readBorderStyle<ParameterName::CssOutlineStyle>(value, cb);
	}),
	pair("outline-color", [] (const StringView &value, const StyleCallback &cb, const StringCallback &strCb) {
		return css_readBorderColor<ParameterName::CssOutlineColor>(value, cb);
	}),
	pair("outline-width", [] (const StringView &value, const StyleCallback &cb, const StringCallback &strCb) {
		return css_readBorderWidth<ParameterName::CssOutlineWidth>(value, cb);
	}),
	pair("border-top", [] (const StringView &value, const StyleCallback &cb, const StringCallback &strCb) {
		return css_readBorder<ParameterName::CssBorderTopStyle, ParameterName::CssBorderTopColor, ParameterName::CssBorderTopWidth>(value, cb);
	}),
	pair("border-top-style", [] (const StringView &value, const StyleCallback &cb, const StringCallback &strCb) {
		return css_readBorderStyle<ParameterName::CssBorderTopStyle>(value, cb);
	}),
	pair("border-top-color", [] (const StringView &value, const StyleCallback &cb, const StringCallback &strCb) {
		return css_readBorderColor<ParameterName::CssBorderTopColor>(value, cb);
	}),
	pair("border-top-width", [] (const StringView &value, const StyleCallback &cb, const StringCallback &strCb) {
		return css_readBorderWidth<ParameterName::CssBorderTopWidth>(value, cb);
	}),
	pair("border-right", [] (const StringView &value, const StyleCallback &cb, const StringCallback &strCb) {
		return css_readBorder<ParameterName::CssBorderRightStyle, ParameterName::CssBorderRightColor, ParameterName::CssBorderRightWidth>(value, cb);
	}),
	pair("border-right-style", [] (const StringView &value, const StyleCallback &cb, const StringCallback &strCb) {
		return css_readBorderStyle<ParameterName::CssBorderRightStyle>(value, cb);
	}),
	pair("border-right-color", [] (const StringView &value, const StyleCallback &cb, const StringCallback &strCb) {
		return css_readBorderColor<ParameterName::CssBorderRightColor>(value, cb);
	}),
	pair("border-right-width", [] (const StringView &value, const StyleCallback &cb, const StringCallback &strCb) {
		return css_readBorderWidth<ParameterName::CssBorderRightWidth>(value, cb);
	}),
	pair("border-bottom", [] (const StringView &value, const StyleCallback &cb, const StringCallback &strCb) {
		return css_readBorder<ParameterName::CssBorderBottomStyle, ParameterName::CssBorderBottomColor, ParameterName::CssBorderBottomWidth>(value, cb);
	}),
	pair("border-bottom-style", [] (const StringView &value, const StyleCallback &cb, const StringCallback &strCb) {
		return css_readBorderStyle<ParameterName::CssBorderBottomStyle>(value, cb);
	}),
	pair("border-bottom-color", [] (const StringView &value, const StyleCallback &cb, const StringCallback &strCb) {
		return css_readBorderColor<ParameterName::CssBorderBottomColor>(value, cb);
	}),
	pair("border-bottom-width", [] (const StringView &value, const StyleCallback &cb, const StringCallback &strCb) {
		return css_readBorderWidth<ParameterName::CssBorderBottomWidth>(value, cb);
	}),
	pair("border-left", [] (const StringView &value, const StyleCallback &cb, const StringCallback &strCb) {
		return css_readBorder<ParameterName::CssBorderLeftStyle, ParameterName::CssBorderLeftColor, ParameterName::CssBorderLeftWidth>(value, cb);
	}),
	pair("border-left-style", [] (const StringView &value, const StyleCallback &cb, const StringCallback &strCb) {
		return css_readBorderStyle<ParameterName::CssBorderLeftStyle>(value, cb);
	}),
	pair("border-left-color", [] (const StringView &value, const StyleCallback &cb, const StringCallback &strCb) {
		return css_readBorderColor<ParameterName::CssBorderLeftColor>(value, cb);
	}),
	pair("border-left-width", [] (const StringView &value, const StyleCallback &cb, const StringCallback &strCb) {
		return css_readBorderWidth<ParameterName::CssBorderLeftWidth>(value, cb);
	}),
	pair("border-style", [] (const StringView &value, const StyleCallback &cb, const StringCallback &strCb) {
		if (value.empty()) {
			return false;
		}
		BorderStyle top, right, bottom, left;
		css_readQuadValue(value, top, right, bottom, left, [&] (const StringView &v) -> BorderStyle {
			if (v.compare("solid")) {
				return BorderStyle::Solid;
			} else if (v.compare("dotted")) {
				return BorderStyle::Dotted;
			} else if (v.compare("dashed")) {
				return BorderStyle::Dashed;
			}
			return BorderStyle::None;
		});
		return cb(StyleParameter::create<ParameterName::CssBorderTopStyle>(top))
			&& cb(StyleParameter::create<ParameterName::CssBorderRightStyle>(right))
			&& cb(StyleParameter::create<ParameterName::CssBorderBottomStyle>(bottom))
			&& cb(StyleParameter::create<ParameterName::CssBorderLeftStyle>(left));
	}),
	pair("border-color", [] (const StringView &value, const StyleCallback &cb, const StringCallback &strCb) {
		if (value.empty()) {
			return false;
		}
		Color4B top, right, bottom, left;
		css_readQuadValue(value, top, right, bottom, left, [&] (const StringView &v) -> Color4B {
			if (v.compare("transparent")) {
				return Color4B(0, 0, 0, 0);
			} else {
				Color4B color(0, 0, 0, 0);
				readColor(v, color);
				return color;
			}
		});
		return cb(StyleParameter::create<ParameterName::CssBorderTopColor>(top))
			&& cb(StyleParameter::create<ParameterName::CssBorderRightColor>(right))
			&& cb(StyleParameter::create<ParameterName::CssBorderBottomColor>(bottom))
			&& cb(StyleParameter::create<ParameterName::CssBorderLeftColor>(left));
	}),
	pair("border-color", [] (const StringView &value, const StyleCallback &cb, const StringCallback &strCb) {
		if (value.empty()) {
			return false;
		}
		Metric top, right, bottom, left;
		css_readQuadValue(value, top, right, bottom, left, [&] (const StringView &v) -> Metric {
			if (v.compare("thin")) {
				return Metric(2.0f, Metric::Units::Px);
			} else if (v.compare("medium")) {
				return Metric(4.0f, Metric::Units::Px);
			} else if (v.compare("thick")) {
				return Metric(6.0f, Metric::Units::Px);
			}

			Metric m(0.0f, Metric::Units::Px);
			parser::readStyleMetric(v, m);
			return m;
		});
		return cb(StyleParameter::create<ParameterName::CssBorderTopWidth>(top))
			&& cb(StyleParameter::create<ParameterName::CssBorderRightWidth>(right))
			&& cb(StyleParameter::create<ParameterName::CssBorderBottomWidth>(bottom))
			&& cb(StyleParameter::create<ParameterName::CssBorderLeftWidth>(left));
	}),
	pair("border", [] (const StringView &value, const StyleCallback &cb, const StringCallback &strCb) {
		if (value.empty()) {
			return false;
		}
		BorderStyle style = BorderStyle::None;
		Metric width(0.0f, Metric::Units::Px);
		Color4B color(0, 0, 0, 0);
		value.split<StringView::CharGroup<CharGroupId::WhiteSpace>>([&] (const StringView &r) {
			if (r.compare("solid")) {
				style = BorderStyle::Solid;
			} else if (r.compare("dotted")) {
				style = BorderStyle::Dotted;
			} else if (r.compare("dashed")) {
				style = BorderStyle::Dashed;
			} else if (r.compare("none")) {
				style = BorderStyle::None;
			} else if (r.compare("transparent")) {
				color = Color4B(0, 0, 0, 0);
			} else if (r.compare("thin")) {
				width = Metric(2.0f, Metric::Units::Px);
			} else if (r.compare("medium")) {
				width = Metric(4.0f, Metric::Units::Px);
			} else if (r.compare("thick")) {
				width = Metric(6.0f, Metric::Units::Px);
			} else if (!readColor(r, color)) {
				parser::readStyleMetric(r, width);
			}
		});
		return cb(StyleParameter::create<ParameterName::CssBorderTopStyle>(style))
			&& cb(StyleParameter::create<ParameterName::CssBorderRightStyle>(style))
			&& cb(StyleParameter::create<ParameterName::CssBorderBottomStyle>(style))
			&& cb(StyleParameter::create<ParameterName::CssBorderLeftStyle>(style))
			&& cb(StyleParameter::create<ParameterName::CssBorderTopColor>(color))
			&& cb(StyleParameter::create<ParameterName::CssBorderRightColor>(color))
			&& cb(StyleParameter::create<ParameterName::CssBorderBottomColor>(color))
			&& cb(StyleParameter::create<ParameterName::CssBorderLeftColor>(color))
			&& cb(StyleParameter::create<ParameterName::CssBorderTopWidth>(width))
			&& cb(StyleParameter::create<ParameterName::CssBorderRightWidth>(width))
			&& cb(StyleParameter::create<ParameterName::CssBorderBottomWidth>(width))
			&& cb(StyleParameter::create<ParameterName::CssBorderLeftWidth>(width));
	}),
	pair("border-collapse", [] (const StringView &value, const StyleCallback &cb, const StringCallback &strCb) {
		if (value.compare("collapse")) {
			return cb(StyleParameter::create<ParameterName::CssBorderCollapse>(BorderCollapse::Collapse));
		} else if (value.compare("separate")) {
			return cb(StyleParameter::create<ParameterName::CssBorderCollapse>(BorderCollapse::Separate));
		}
		return false;
	}),
	pair("caption-side", [] (const StringView &value, const StyleCallback &cb, const StringCallback &strCb) {
		if (value.compare("top")) {
			return cb(StyleParameter::create<ParameterName::CssCaptionSide>(CaptionSide::Top));
		} else if (value.compare("bottom")) {
			return cb(StyleParameter::create<ParameterName::CssCaptionSide>(CaptionSide::Bottom));
		}
		return false;
	}),
	pair("page-break-after", [] (const StringView &value, const StyleCallback &cb, const StringCallback &strCb) {
		if (value.compare("always")) {
			return cb(StyleParameter::create<ParameterName::CssPageBreakAfter>(PageBreak::Always));
		} else if (value.compare("auto")) {
			return cb(StyleParameter::create<ParameterName::CssPageBreakAfter>(PageBreak::Auto));
		} else if (value.compare("avoid")) {
			return cb(StyleParameter::create<ParameterName::CssPageBreakAfter>(PageBreak::Avoid));
		} else if (value.compare("left")) {
			return cb(StyleParameter::create<ParameterName::CssPageBreakAfter>(PageBreak::Left));
		} else if (value.compare("right")) {
			return cb(StyleParameter::create<ParameterName::CssPageBreakAfter>(PageBreak::Right));
		}
		return false;
	}),
	pair("page-break-before", [] (const StringView &value, const StyleCallback &cb, const StringCallback &strCb) {
		if (value.compare("always")) {
			return cb(StyleParameter::create<ParameterName::CssPageBreakBefore>(PageBreak::Always));
		} else if (value.compare("auto")) {
			return cb(StyleParameter::create<ParameterName::CssPageBreakBefore>(PageBreak::Auto));
		} else if (value.compare("avoid")) {
			return cb(StyleParameter::create<ParameterName::CssPageBreakBefore>(PageBreak::Avoid));
		} else if (value.compare("left")) {
			return cb(StyleParameter::create<ParameterName::CssPageBreakBefore>(PageBreak::Left));
		} else if (value.compare("right")) {
			return cb(StyleParameter::create<ParameterName::CssPageBreakBefore>(PageBreak::Right));
		}
		return false;
	}),
	pair("page-break-inside", [] (const StringView &value, const StyleCallback &cb, const StringCallback &strCb) {
		if (value.compare("auto")) {
			return cb(StyleParameter::create<ParameterName::CssPageBreakInside>(PageBreak::Auto));
		} else if (value.compare("avoid")) {
			return cb(StyleParameter::create<ParameterName::CssPageBreakInside>(PageBreak::Avoid));
		}
		return false;
	}),
	pair("orientation", [] (const StringView &value, const StyleCallback &cb, const StringCallback &strCb) {
		if (value.compare("landscape")) {
			return cb(StyleParameter::create<ParameterName::CssMediaOrientation>(Orientation::Landscape));
		} else if (value.compare("portrait")) {
			return cb(StyleParameter::create<ParameterName::CssMediaOrientation>(Orientation::Portrait));
		}
		return false;
	}),
	pair("pointer", [] (const StringView &value, const StyleCallback &cb, const StringCallback &strCb) {
		if (value.compare("none")) {
			return cb(StyleParameter::create<ParameterName::CssMediaPointer>(Pointer::None));
		} else if (value.compare("fine")) {
			return cb(StyleParameter::create<ParameterName::CssMediaPointer>(Pointer::Fine));
		} else if (value.compare("coarse")) {
			return cb(StyleParameter::create<ParameterName::CssMediaPointer>(Pointer::Coarse));
		}
		return false;
	}),
	pair("hover", [] (const StringView &value, const StyleCallback &cb, const StringCallback &strCb) {
		if (value.compare("none")) {
			return cb(StyleParameter::create<ParameterName::CssMediaHover>(Hover::None));
		} else if (value.compare("hover")) {
			return cb(StyleParameter::create<ParameterName::CssMediaHover>(Hover::Hover));
		} else if (value.compare("on-demand")) {
			return cb(StyleParameter::create<ParameterName::CssMediaHover>(Hover::OnDemand));
		}
		return false;
	}),
	pair("light-level", [] (const StringView &value, const StyleCallback &cb, const StringCallback &strCb) {
		if (value.compare("dim")) {
			return cb(StyleParameter::create<ParameterName::CssMediaLightLevel>(LightLevel::Dim));
		} else if (value.compare("normal")) {
			return cb(StyleParameter::create<ParameterName::CssMediaLightLevel>(LightLevel::Normal));
		} else if (value.compare("washed")) {
			return cb(StyleParameter::create<ParameterName::CssMediaLightLevel>(LightLevel::Washed));
		}
		return false;
	}),
	pair("scripting", [] (const StringView &value, const StyleCallback &cb, const StringCallback &strCb) {
		if (value.compare("none")) {
			return cb(StyleParameter::create<ParameterName::CssMediaScripting>(Scripting::None));
		} else if (value.compare("initial-only")) {
			return cb(StyleParameter::create<ParameterName::CssMediaScripting>(Scripting::InitialOnly));
		} else if (value.compare("enabled")) {
			return cb(StyleParameter::create<ParameterName::CssMediaScripting>(Scripting::Enabled));
		}
		return false;
	}),
	pair("aspect-ratio", [] (const StringView &value, const StyleCallback &cb, const StringCallback &strCb) {
		float ratio = 0.0f;
		if (css_readAspectRatioValue(value, ratio)) {
			return cb(StyleParameter::create<ParameterName::CssMediaAspectRatio>(ratio));
		}
		return false;
	}),
	pair("min-aspect-ratio", [] (const StringView &value, const StyleCallback &cb, const StringCallback &strCb) {
		float ratio = 0.0f;
		if (css_readAspectRatioValue(value, ratio)) {
			return cb(StyleParameter::create<ParameterName::CssMediaMinAspectRatio>(ratio));
		}
		return false;
	}),
	pair("max-aspect-ratio", [] (const StringView &value, const StyleCallback &cb, const StringCallback &strCb) {
		float ratio = 0.0f;
		if (css_readAspectRatioValue(value, ratio)) {
			return cb(StyleParameter::create<ParameterName::CssMediaMaxAspectRatio>(ratio));
		}
		return false;
	}),
	pair("resolution", [] (const StringView &value, const StyleCallback &cb, const StringCallback &strCb) {
		Metric size;
		if (parser::readStyleMetric(value, size, true)) {
			return cb(StyleParameter::create<ParameterName::CssMediaResolution>(size));
		}
		return false;
	}),
	pair("min-resolution", [] (const StringView &value, const StyleCallback &cb, const StringCallback &strCb) {
		Metric size;
		if (parser::readStyleMetric(value, size, true)) {
			return cb(StyleParameter::create<ParameterName::CssMediaMinResolution>(size));
		}
		return false;
	}),
	pair("max-resolution", [] (const StringView &value, const StyleCallback &cb, const StringCallback &strCb) {
		Metric size;
		if (parser::readStyleMetric(value, size, true)) {
			return cb(StyleParameter::create<ParameterName::CssMediaMaxResolution>(size));
		}
		return false;
	}),
	pair("x-option", [] (const StringView &value, const StyleCallback &cb, const StringCallback &strCb) {
		auto str = StyleContainer::resolveCssString(value);
		if (!str.empty()) {
			return cb(StyleParameter::create<ParameterName::CssMediaOption>(strCb(str)));
		}
		return false;
	})
};

void StyleContainer::readCssParameter(const StringView &name, const StringView &value, const StyleCallback &cb, const StringCallback &strCb) {
	auto it = s_cssParameters.find(name);
	if (it != s_cssParameters.end()) {
		it->second(value, cb, strCb);
	} else {
		if (!name.is('-')) {
			log::warn("document::StyleContainer", "Unknown CSS parameter: ", name);
		}
	}
}

}