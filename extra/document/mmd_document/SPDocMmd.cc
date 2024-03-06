// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2017 Roman Katuntsev <sbkarr@stappler.org>

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

#include "SPDocMmd.h"
#include "SPDocFormat.h"
#include "SPDocPageContainer.h"
#include "MMDEngine.h"
#include "SPDocMmdProcessor.h"

namespace STAPPLER_VERSIONIZED stappler::document {

static Format s_mmdFormat([] (memory::pool_t *, FilePath str, StringView ct) -> bool {
	return DocumentMmd::isMmd(str);
}, [] (memory::pool_t *p, FilePath str, StringView ct) -> Rc<Document> {
	return Rc<DocumentMmd>::create(p, str, ct);
}, [] (memory::pool_t *, BytesView str, StringView ct) -> bool {
	return DocumentMmd::isMmd(str);
}, [] (memory::pool_t *p, BytesView str, StringView ct) -> Rc<Document> {
	return Rc<DocumentMmd>::create(p, str, ct);
}, 0);

bool DocumentMmd::isMmd(BytesView data) {
	StringView str((const char *)data.data(), data.size());
	str.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();

	if (str.is("#") || str.is("{{TOC}}") || str.is("Title:")) {
		return true;
	}

	str.skipUntilString("\n#", true);
	if (str.is("\n#")) {
		return true;
	}

	return false;
}

bool DocumentMmd::isMmd(FilePath path) {
	auto ext = filepath::lastExtension(path.get());
	if (ext == "md" || ext == "markdown") {
		return true;
	}

	if (ext == "text" || ext == "txt" || ext.empty()) {
		if (auto file = filesystem::openForReading(path.get())) {
			StackBuffer<512> data;
			if (io::Producer(file).seekAndRead(0, data, 512) > 0) {
				return isMmd(BytesView(data.data(), data.size()));
			}
		}
	}

	return false;
}

bool DocumentMmd::init(FilePath path, StringView ct) {
	if (!Document::init()) {
		return false;
	}

	auto data = filesystem::readIntoMemory<memory::PoolInterface>(path.get());
	return read(data, ct);
}

bool DocumentMmd::init(BytesView data, StringView ct) {
	if (!Document::init()) {
		return false;
	}

	return read(data, ct);
}

bool DocumentMmd::init(memory::pool_t *pool, FilePath path, StringView ct) {
	if (!Document::init(pool)) {
		return false;
	}

	memory::pool::context ctx(_data->pool);

	auto data = filesystem::readIntoMemory<memory::PoolInterface>(path.get());
	return read(data, ct);
}

bool DocumentMmd::init(memory::pool_t *pool, BytesView data, StringView ct) {
	if (!Document::init(pool)) {
		return false;
	}

	memory::pool::context ctx(_data->pool);
	return read(data, ct);
}


bool DocumentMmd::read(BytesView data, StringView ct) {
	if (data.empty()) {
		return false;
	}

	_monospaceId = _data->addString("monospace");

	mmd::Engine e; e.init(StringView((const char *)data.data(), data.size()), mmd::StapplerExtensions);
	e.process([&] (const mmd::Content &c, const StringView &s, const mmd::Token &t) {
		mmd::DocumentProcessor p; p.init(this, _data);
		p.process(c, s, t);
	});

	return !_data->pages.empty();
}

PageContainer *DocumentMmd::acquireRootPage() {
	memory::pool::context ctx(_pool);
	if (_data->pages.empty()) {
		_data->pages.emplace(StringView(), new (_pool) PageContainer(_data));
	}

	auto page = _data->pages.begin()->second;

	PageContainer::StyleBuffers buffers;

	auto textQuery = PageContainer::StringReader("all and (max-width:500px)");
	_minWidthQuery = _data->addQuery(MediaQuery{ page->readMediaQueryList(buffers, textQuery) });

	textQuery = PageContainer::StringReader("all and (min-width:500px) and (max-width:750px)");
	_mediumWidthQuery = _data->addQuery(MediaQuery{ page->readMediaQueryList(buffers, textQuery) });

	textQuery = PageContainer::StringReader("all and (min-width:750px)");
	_maxWidthQuery = _data->addQuery(MediaQuery{ page->readMediaQueryList(buffers, textQuery) });

	textQuery = PageContainer::StringReader("all and (x-option:image-view)");
	_imageViewQuery = _data->addQuery(MediaQuery{ page->readMediaQueryList(buffers, textQuery) });

	return page;
}

void DocumentMmd::onTag(StyleList &style, StringView tag, StringView parent, const MediaParameters &media) const {
	if (tag == "div") {
		style.set(StyleParameter::create<ParameterName::CssDisplay>(Display::Block), true);
	}

	if (tag == "p" || tag == "h1" || tag == "h2" || tag == "h3" || tag == "h4" || tag == "h5" || tag == "h6") {
		if (parent != "li" && parent != "blockquote") {
			style.set(StyleParameter::create<ParameterName::CssMarginTop>(Metric(0.5f, Metric::Units::Em)), true);
			style.set(StyleParameter::create<ParameterName::CssMarginBottom>(Metric(0.5f, Metric::Units::Em)), true);
			if (parent != "dd" && parent != "figcaption") {
				style.set(StyleParameter::create<ParameterName::CssTextIndent>(Metric(1.5f, Metric::Units::Rem)), true);
			}
		}
		style.set(StyleParameter::create<ParameterName::CssLineHeight>(Metric(1.2f, Metric::Units::Em)), true);
		style.set(StyleParameter::create<ParameterName::CssDisplay>(Display::Block), true);
	}

	if (tag == "span" || tag == "strong" || tag == "em" || tag == "nobr"
			|| tag == "sub" || tag == "sup" || tag == "inf" || tag == "b"
			|| tag == "i" || tag == "u" || tag == "code") {
		style.set(StyleParameter::create<ParameterName::CssDisplay>(Display::Inline), true);
	}

	if (tag == "h1") {
		style.set(StyleParameter::create<ParameterName::CssMarginTop>(Metric(0.8f, Metric::Units::Em)), true);
		style.set(StyleParameter::create<ParameterName::CssFontSize>(FontSize(32)), true);
		style.set(StyleParameter::create<ParameterName::CssFontWeight>(FontWeight(200)), true);
		style.set(StyleParameter::create<ParameterName::CssOpacity>(uint8_t(222)), true);

	} else if (tag == "h2") {
		style.set(StyleParameter::create<ParameterName::CssMarginTop>(Metric(0.8f, Metric::Units::Em)), true);
		style.set(StyleParameter::create<ParameterName::CssFontSize>(FontSize(28)), true);
		style.set(StyleParameter::create<ParameterName::CssFontWeight>(FontWeight(400)), true);
		style.set(StyleParameter::create<ParameterName::CssOpacity>(uint8_t(222)), true);

	} else if (tag == "h3") {
		style.set(StyleParameter::create<ParameterName::CssMarginTop>(Metric(0.8f, Metric::Units::Em)), true);
		style.set(StyleParameter::create<ParameterName::CssFontSize>(FontSize::XXLarge), true);
		style.set(StyleParameter::create<ParameterName::CssFontWeight>(FontWeight(400)), true);
		style.set(StyleParameter::create<ParameterName::CssOpacity>(uint8_t(200)), true);

	} else if (tag == "h4") {
		style.set(StyleParameter::create<ParameterName::CssMarginTop>(Metric(0.8f, Metric::Units::Em)), true);
		style.set(StyleParameter::create<ParameterName::CssFontSize>(FontSize::XLarge), true);
		style.set(StyleParameter::create<ParameterName::CssFontWeight>(FontWeight(500)), true);
		style.set(StyleParameter::create<ParameterName::CssOpacity>(uint8_t(188)), true);

	} else if (tag == "h5") {
		style.set(StyleParameter::create<ParameterName::CssMarginTop>(Metric(0.8f, Metric::Units::Em)), true);
		style.set(StyleParameter::create<ParameterName::CssFontSize>(FontSize(18)), true);
		style.set(StyleParameter::create<ParameterName::CssFontWeight>(FontWeight(400)), true);
		style.set(StyleParameter::create<ParameterName::CssOpacity>(uint8_t(222)), true);

	} else if (tag == "h6") {
		style.set(StyleParameter::create<ParameterName::CssMarginTop>(Metric(0.8f, Metric::Units::Em)), true);
		style.set(StyleParameter::create<ParameterName::CssFontSize>(FontSize::Large), true);
		style.set(StyleParameter::create<ParameterName::CssFontWeight>(FontWeight(500)), true);
		style.set(StyleParameter::create<ParameterName::CssOpacity>(uint8_t(216)), true);

	} else if (tag == "p") {
		style.set(StyleParameter::create<ParameterName::CssTextAlign>(TextAlign::Justify), true);
		style.set(StyleParameter::create<ParameterName::CssHyphens>(Hyphens::Auto), true);

	} else if (tag == "hr") {
		style.set(StyleParameter::create<ParameterName::CssMarginTop>(Metric(0.5f, Metric::Units::Em)), true);
		style.set(StyleParameter::create<ParameterName::CssMarginRight>(Metric(0.5f, Metric::Units::Em)), true);
		style.set(StyleParameter::create<ParameterName::CssMarginBottom>(Metric(0.5f, Metric::Units::Em)), true);
		style.set(StyleParameter::create<ParameterName::CssMarginLeft>(Metric(0.5f, Metric::Units::Em)), true);
		style.set(StyleParameter::create<ParameterName::CssHeight>(Metric(2, Metric::Units::Px)), true);
		style.set(StyleParameter::create<ParameterName::CssBackgroundColor>(Color4B(0, 0, 0, 127)), true);

	} else if (tag == "a") {
		style.set(StyleParameter::create<ParameterName::CssTextDecoration>(TextDecoration::Underline), true);
		style.set(StyleParameter::create<ParameterName::CssColor>(Color3B(0x0d, 0x47, 0xa1)), true);

	} else if (tag == "b" || tag == "strong") {
		style.set(StyleParameter::create<ParameterName::CssFontWeight>(FontWeight::Bold), true);

	} else if (tag == "i" || tag == "em") {
		style.set(StyleParameter::create<ParameterName::CssFontStyle>(FontStyle::Italic), true);

	} else if (tag == "u") {
		style.set(StyleParameter::create<ParameterName::CssTextDecoration>(TextDecoration::Underline), true);

	} else if (tag == "nobr") {
		style.set(StyleParameter::create<ParameterName::CssWhiteSpace>(WhiteSpace::Nowrap), true);

	} else if (tag == "pre") {
		style.set(StyleParameter::create<ParameterName::CssMarginTop>(Metric(0.5f, Metric::Units::Em)), true);
		style.set(StyleParameter::create<ParameterName::CssMarginBottom>(Metric(0.5f, Metric::Units::Em)), true);
		style.set(StyleParameter::create<ParameterName::CssWhiteSpace>(WhiteSpace::Pre), true);
		style.set(StyleParameter::create<ParameterName::CssDisplay>(Display::Block), true);
		style.set(StyleParameter::create<ParameterName::CssBackgroundColor>(Color4B(228, 228, 228, 255)), true);

		style.set(StyleParameter::create<ParameterName::CssPaddingTop>(Metric(0.5f, Metric::Units::Em)), true);
		style.set(StyleParameter::create<ParameterName::CssPaddingLeft>(Metric(0.5f, Metric::Units::Em)), true);
		style.set(StyleParameter::create<ParameterName::CssPaddingBottom>(Metric(0.5f, Metric::Units::Em)), true);
		style.set(StyleParameter::create<ParameterName::CssPaddingRight>(Metric(0.5f, Metric::Units::Em)), true);

	} else if (tag == "code") {
		style.set(StyleParameter::create<ParameterName::CssFontFamily>(_monospaceId), true);
		style.set(StyleParameter::create<ParameterName::CssBackgroundColor>(Color4B(228, 228, 228, 255)), true);

	} else if (tag == "sub" || tag == "inf") {
		style.set(StyleParameter::create<ParameterName::CssVerticalAlign>(VerticalAlign::Sub), true);
		style.data.push_back(StyleParameter::create<ParameterName::CssFontSizeIncrement>(Metric(0.7f, Metric::Em)));

	} else if (tag == "sup") {
		style.set(StyleParameter::create<ParameterName::CssVerticalAlign>(VerticalAlign::Super), true);
		style.data.push_back(StyleParameter::create<ParameterName::CssFontSizeIncrement>(Metric(0.7f, Metric::Em)));

	} else if (tag == "body") {
		style.set(StyleParameter::create<ParameterName::CssMarginLeft>(Metric(0.8f, Metric::Units::Rem), MediaQuery::IsScreenLayout), true);
		style.set(StyleParameter::create<ParameterName::CssMarginRight>(Metric(0.8f, Metric::Units::Rem), MediaQuery::IsScreenLayout), true);

	} else if (tag == "br") {
		style.set(StyleParameter::create<ParameterName::CssWhiteSpace>(WhiteSpace::PreLine), true);
		style.set(StyleParameter::create<ParameterName::CssDisplay>(Display::Inline), true);

	} else if (tag == "li") {
		style.set(StyleParameter::create<ParameterName::CssDisplay>(Display::ListItem), true);
		style.set(StyleParameter::create<ParameterName::CssLineHeight>(Metric(1.2f, Metric::Units::Em)), true);
		style.set(StyleParameter::create<ParameterName::CssMarginTop>(Metric(0.25f, Metric::Units::Rem)), true);
		style.set(StyleParameter::create<ParameterName::CssMarginBottom>(Metric(0.25f, Metric::Units::Rem)), true);

	} else if (tag == "ol") {
		style.set(StyleParameter::create<ParameterName::CssDisplay>(Display::Block), true);
		style.set(StyleParameter::create<ParameterName::CssListStyleType>(ListStyleType::Decimal), true);
		style.set(StyleParameter::create<ParameterName::CssPaddingLeft>(Metric(1.5f, Metric::Units::Rem)), true);
		style.set(StyleParameter::create<ParameterName::CssMarginTop>(Metric(0.4f, Metric::Units::Rem)), true);
		style.set(StyleParameter::create<ParameterName::CssMarginBottom>(Metric(0.4f, Metric::Units::Rem)), true);
		style.set(StyleParameter::create<ParameterName::CssXListStyleOffset>(Metric(0.7f, Metric::Units::Rem)), true);

	} else if (tag == "ul") {
		style.set(StyleParameter::create<ParameterName::CssDisplay>(Display::Block), true);
		if (parent == "li") {
			style.set(StyleParameter::create<ParameterName::CssListStyleType>(ListStyleType::Circle), true);
		} else {
			style.set(StyleParameter::create<ParameterName::CssListStyleType>(ListStyleType::Disc), true);
		}
		style.set(StyleParameter::create<ParameterName::CssPaddingLeft>(Metric(1.5f, Metric::Units::Rem)), true);
		style.set(StyleParameter::create<ParameterName::CssMarginTop>(Metric(0.4f, Metric::Units::Rem)), true);
		style.set(StyleParameter::create<ParameterName::CssMarginBottom>(Metric(0.4f, Metric::Units::Rem)), true);
		style.set(StyleParameter::create<ParameterName::CssXListStyleOffset>(Metric(0.7f, Metric::Units::Rem)), true);

	} else if (tag == "img") {
		if (parent == "figure") {
			style.set(StyleParameter::create<ParameterName::CssDisplay>(Display::Block), true);
		} else {
			style.set(StyleParameter::create<ParameterName::CssDisplay>(Display::InlineBlock), true);
		}

		style.set(StyleParameter::create<ParameterName::CssBackgroundSizeWidth>(Metric(1.0, Metric::Units::Contain)), true);
		style.set(StyleParameter::create<ParameterName::CssBackgroundSizeHeight>(Metric(1.0, Metric::Units::Contain)), true);
		style.set(StyleParameter::create<ParameterName::CssPageBreakInside>(PageBreak::Avoid), true);

		style.set(StyleParameter::create<ParameterName::CssMarginRight>(Metric(0.0f, Metric::Units::Auto)), true);
		style.set(StyleParameter::create<ParameterName::CssMarginLeft>(Metric(0.0f, Metric::Units::Auto)), true);

		style.set(StyleParameter::create<ParameterName::CssMaxWidth>(Metric(70.0f, Metric::Units::Vw)), true);
		style.set(StyleParameter::create<ParameterName::CssMaxHeight>(Metric(70.0f, Metric::Units::Vh)), true);
		style.set(StyleParameter::create<ParameterName::CssMinWidth>(Metric(100.8f, Metric::Units::Px)), true);
		style.set(StyleParameter::create<ParameterName::CssMinHeight>(Metric(88.8f, Metric::Units::Px)), true);

	} else if (tag == "table") {
		style.data.reserve(16);
		style.data.push_back(StyleParameter::create<ParameterName::CssMarginTop>(Metric(0.5f, Metric::Units::Rem)));
		style.data.push_back(StyleParameter::create<ParameterName::CssMarginBottom>(Metric(0.5f, Metric::Units::Rem)));

		style.data.push_back(StyleParameter::create<ParameterName::CssBorderTopStyle>(BorderStyle::Solid));
		style.data.push_back(StyleParameter::create<ParameterName::CssBorderTopWidth>(Metric(1.0f, Metric::Units::Px)));
		style.data.push_back(StyleParameter::create<ParameterName::CssBorderTopColor>(Color4B(168, 168, 168,255)));
		style.data.push_back(StyleParameter::create<ParameterName::CssBorderRightStyle>(BorderStyle::Solid));
		style.data.push_back(StyleParameter::create<ParameterName::CssBorderRightWidth>(Metric(1.0f, Metric::Units::Px)));
		style.data.push_back(StyleParameter::create<ParameterName::CssBorderRightColor>(Color4B(168, 168, 168,255)));
		style.data.push_back(StyleParameter::create<ParameterName::CssBorderBottomStyle>(BorderStyle::Solid));
		style.data.push_back(StyleParameter::create<ParameterName::CssBorderBottomWidth>(Metric(1.0f, Metric::Units::Px)));
		style.data.push_back(StyleParameter::create<ParameterName::CssBorderBottomColor>(Color4B(168, 168, 168,255)));
		style.data.push_back(StyleParameter::create<ParameterName::CssBorderLeftStyle>(BorderStyle::Solid));
		style.data.push_back(StyleParameter::create<ParameterName::CssBorderLeftWidth>(Metric(1.0f, Metric::Units::Px)));
		style.data.push_back(StyleParameter::create<ParameterName::CssBorderLeftColor>(Color4B(168, 168, 168,255)));

		style.set(StyleParameter::create<ParameterName::CssDisplay>(Display::Table), true);

	} else if (tag == "td" || tag == "th") {
		style.data.reserve(18);
		style.data.push_back(StyleParameter::create<ParameterName::CssPaddingTop>(Metric(0.3f, Metric::Units::Rem)));
		style.data.push_back(StyleParameter::create<ParameterName::CssPaddingLeft>(Metric(0.3f, Metric::Units::Rem)));
		style.data.push_back(StyleParameter::create<ParameterName::CssPaddingBottom>(Metric(0.3f, Metric::Units::Rem)));
		style.data.push_back(StyleParameter::create<ParameterName::CssPaddingRight>(Metric(0.3f, Metric::Units::Rem)));

		style.data.push_back(StyleParameter::create<ParameterName::CssBorderTopStyle>(BorderStyle::Solid));
		style.data.push_back(StyleParameter::create<ParameterName::CssBorderTopWidth>(Metric(1.0f, Metric::Units::Px)));
		style.data.push_back(StyleParameter::create<ParameterName::CssBorderTopColor>(Color4B(168, 168, 168,255)));
		style.data.push_back(StyleParameter::create<ParameterName::CssBorderRightStyle>(BorderStyle::Solid));
		style.data.push_back(StyleParameter::create<ParameterName::CssBorderRightWidth>(Metric(1.0f, Metric::Units::Px)));
		style.data.push_back(StyleParameter::create<ParameterName::CssBorderRightColor>(Color4B(168, 168, 168,255)));
		style.data.push_back(StyleParameter::create<ParameterName::CssBorderBottomStyle>(BorderStyle::Solid));
		style.data.push_back(StyleParameter::create<ParameterName::CssBorderBottomWidth>(Metric(1.0f, Metric::Units::Px)));
		style.data.push_back(StyleParameter::create<ParameterName::CssBorderBottomColor>(Color4B(168, 168, 168,255)));
		style.data.push_back(StyleParameter::create<ParameterName::CssBorderLeftStyle>(BorderStyle::Solid));
		style.data.push_back(StyleParameter::create<ParameterName::CssBorderLeftWidth>(Metric(1.0f, Metric::Units::Px)));
		style.data.push_back(StyleParameter::create<ParameterName::CssBorderLeftColor>(Color4B(168, 168, 168,255)));

		if (tag == "th") {
			style.set(StyleParameter::create<ParameterName::CssFontWeight>(FontWeight::Bold), true);
		}

	} else if (tag == "caption") {
		style.data.push_back(StyleParameter::create<ParameterName::CssTextAlign>(TextAlign::Center));

		style.data.push_back(StyleParameter::create<ParameterName::CssPaddingTop>(Metric(0.4f, Metric::Units::Rem)));
		style.data.push_back(StyleParameter::create<ParameterName::CssPaddingBottom>(Metric(0.4f, Metric::Units::Rem)));
		style.data.push_back(StyleParameter::create<ParameterName::CssPaddingLeft>(Metric(0.4f, Metric::Units::Rem)));
		style.data.push_back(StyleParameter::create<ParameterName::CssPaddingRight>(Metric(0.4f, Metric::Units::Rem)));

	} else if (tag == "blockquote") {
		style.set(StyleParameter::create<ParameterName::CssDisplay>(Display::Block), true);
		style.set(StyleParameter::create<ParameterName::CssMarginTop>(Metric(0.5f, Metric::Units::Rem)), true);
		style.set(StyleParameter::create<ParameterName::CssMarginBottom>(Metric(0.5f, Metric::Units::Rem)), true);

		if (parent == "blockquote") {
			style.set(StyleParameter::create<ParameterName::CssPaddingLeft>(Metric(0.8f, Metric::Units::Rem)), true);
		} else {
			style.set(StyleParameter::create<ParameterName::CssMarginLeft>(Metric(1.5f, Metric::Units::Rem)), true);
			style.set(StyleParameter::create<ParameterName::CssMarginRight>(Metric(1.5f, Metric::Units::Rem)), true);
			style.set(StyleParameter::create<ParameterName::CssPaddingLeft>(Metric(1.0f, Metric::Units::Rem)), true);
		}

		style.set(StyleParameter::create<ParameterName::CssPaddingTop>(Metric(0.1f, Metric::Units::Rem)), true);
		style.set(StyleParameter::create<ParameterName::CssPaddingBottom>(Metric(0.3f, Metric::Units::Rem)), true);

		style.set(StyleParameter::create<ParameterName::CssBorderLeftColor>(Color4B(0, 0, 0, 64)), true);
		style.set(StyleParameter::create<ParameterName::CssBorderLeftWidth>(Metric(3, Metric::Units::Px)), true);
		style.set(StyleParameter::create<ParameterName::CssBorderLeftStyle>(BorderStyle::Solid), true);

	} else if (tag == "dl") {
		style.set(StyleParameter::create<ParameterName::CssDisplay>(Display::Block), true);
		style.set(StyleParameter::create<ParameterName::CssMarginTop>(Metric(1.0f, Metric::Units::Em)), true);
		style.set(StyleParameter::create<ParameterName::CssMarginBottom>(Metric(1.0f, Metric::Units::Em)), true);
		style.set(StyleParameter::create<ParameterName::CssMarginLeft>(Metric(1.5f, Metric::Units::Rem)), true);

	} else if (tag == "dt") {
		style.set(StyleParameter::create<ParameterName::CssDisplay>(Display::Block), true);
		style.set(StyleParameter::create<ParameterName::CssFontWeight>(FontWeight(700)), true);

	} else if (tag == "dd") {
		style.set(StyleParameter::create<ParameterName::CssDisplay>(Display::Block), true);
		style.set(StyleParameter::create<ParameterName::CssPaddingLeft>(Metric(1.0f, Metric::Units::Rem)), true);
		style.set(StyleParameter::create<ParameterName::CssMarginTop>(Metric(0.5f, Metric::Units::Em)), true);
		style.set(StyleParameter::create<ParameterName::CssMarginBottom>(Metric(0.5f, Metric::Units::Em)), true);

		style.set(StyleParameter::create<ParameterName::CssBorderLeftColor>(Color4B(0, 0, 0, 64)), true);
		style.set(StyleParameter::create<ParameterName::CssBorderLeftWidth>(Metric(2, Metric::Units::Px)), true);
		style.set(StyleParameter::create<ParameterName::CssBorderLeftStyle>(BorderStyle::Solid), true);

	} else if (tag == "figure") {
		style.set(StyleParameter::create<ParameterName::CssDisplay>(Display::Block), true);
		style.set(StyleParameter::create<ParameterName::CssMarginTop>(Metric(1.0f, Metric::Units::Em)), true);
		style.set(StyleParameter::create<ParameterName::CssMarginBottom>(Metric(0.5f, Metric::Units::Em)), true);

	} else if (tag == "figcaption") {
		style.set(StyleParameter::create<ParameterName::CssDisplay>(Display::Block), true);
		style.set(StyleParameter::create<ParameterName::CssFontSize>(FontSize::Small), true);
		style.set(StyleParameter::create<ParameterName::CssFontWeight>(FontWeight(500)), true);
		if (media.hasOption("image-view")) {
			style.set(StyleParameter::create<ParameterName::CssMarginTop>(Metric(0.5f, Metric::Units::Em)), true);
			style.set(StyleParameter::create<ParameterName::CssTextAlign>(TextAlign::Justify), true);
		} else {
			style.set(StyleParameter::create<ParameterName::CssMarginTop>(Metric(0.5f, Metric::Units::Em)), true);
			style.set(StyleParameter::create<ParameterName::CssMarginBottom>(Metric(0.5f, Metric::Units::Em)), true);
			style.set(StyleParameter::create<ParameterName::CssTextAlign>(TextAlign::Center), true);
		}
	}
}

static void LayoutDocument_onClass(StyleList &style, const StringView &name, const StringView &classStr, const MediaParameters &media) {
	if (name == "div" && classStr == "footnotes") {
		style.set(StyleParameter::create<ParameterName::CssMarginTop>(Metric(16.0f, Metric::Units::Px)), true);
		style.set(StyleParameter::create<ParameterName::CssMarginBottom>(Metric(16.0f, Metric::Units::Px)), true);
	}

	if (classStr == "math") {
		style.set(StyleParameter::create<ParameterName::CssDisplay>(Display::None), true);
	}

	if (name == "figure") {
		/*if (classStr == "middle") {
			style.set(StyleParameter::create<ParameterName::CssFloat>(Float::Right), true);
			style.set(StyleParameter::create<ParameterName::CssWidth>(Metric(1.0f, Metric::Units::Percent)), true);
		}*/

	} else if (name == "img") {
		if (classStr == "middle") {
			style.set(StyleParameter::create<ParameterName::CssDisplay>(Display::Block), true);
			style.set(StyleParameter::create<ParameterName::CssMarginRight>(Metric(0.0f, Metric::Units::Auto)), true);
			style.set(StyleParameter::create<ParameterName::CssMarginLeft>(Metric(0.0f, Metric::Units::Auto)), true);
		}
	}

	if (classStr == "reversefootnote" && media.hasOption("tooltip")) {
		style.set(StyleParameter::create<ParameterName::CssDisplay>(Display::None), true);
	}
}

// Default style, that can be redefined with css
void DocumentMmd::beginStyle(StyleList &style, const Node &node, SpanView<const Node *> stack, const MediaParameters &media) const {
	const Node *parent = nullptr;
	if (stack.size() > 1) {
		parent = stack.at(stack.size() - 2);
	}

	onTag(style, node.getHtmlName(), parent ? StringView(parent->getHtmlName()) : StringView(), media);

	if (parent && node.getHtmlName() == "tr") {
		if (parent->getHtmlName() == "tbody") {
			if (parent->getChildIndex(node) % 2 == 1) {
				style.set(StyleParameter::create<ParameterName::CssBackgroundColor>(Color4B::WHITE), true);
			} else {
				style.set(StyleParameter::create<ParameterName::CssBackgroundColor>(Color4B(232, 232, 232, 255)), true);
			}
		} else {
			style.set(StyleParameter::create<ParameterName::CssBackgroundColor>(Color4B::WHITE), true);
		}
	}

	auto &attr = node.getAttributes();
	for (auto &it : attr) {
		if (it.first == "class") {
			StringView r(it.second);
			r.split<StringView::CharGroup<CharGroupId::WhiteSpace>>([&] (const StringView &classStr) {
				LayoutDocument_onClass(style, node.getHtmlName(), classStr, media);
			});
		} else if ((node.getHtmlName() == "img" || node.getHtmlName() == "figcaption") && it.first == "type") {
			LayoutDocument_onClass(style, node.getHtmlName(), it.second, media);
		} else {
			onStyleAttribute(style, node.getHtmlName(), it.first, it.second, media);
		}
	}
}

// Default style, that can NOT be redefined with css
void DocumentMmd::endStyle(StyleList &style, const Node &node, SpanView<const Node *> stack, const MediaParameters &media) const {
	return Document::endStyle(style, node, stack, media);
}

}
