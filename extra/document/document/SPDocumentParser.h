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

#ifndef EXTRA_DOCUMENT_DOCUMENT_SPDOCUMENTPARSER_H_
#define EXTRA_DOCUMENT_DOCUMENT_SPDOCUMENTPARSER_H_

#include "SPDocumentStyle.h"
#include "SPMemory.h"

namespace stappler::document::parser {

using namespace mem_pool;

using StringReader = StringViewUtf8;

String readHtmlTagName(StringReader &);
String readHtmlTagParamName(StringReader &);
String readHtmlTagParamValue(StringReader &);

bool readStyleMargin(const StringView &, Metric &top, Metric &right, Metric &bottom, Metric &left);
bool readStyleMargin(StringReader &, Metric &top, Metric &right, Metric &bottom, Metric &left);
bool readStyleMetric(const StringView &, Metric &value, bool resolutionMetric = false, bool allowEmptyMetric = false);
bool readStyleMetric(StringReader &, Metric &value, bool resolutionMetric = false, bool allowEmptyMetric = false);

struct StyleReaderInfo {
	Callback<void(StringId, const StringView &)> addString;
	Callback<MediaQueryId(MediaQuery &&)> addQuery;
	Callback<void(StringView, const StyleList::StyleVec &, MediaQueryId)> addStyle;
};

void readStyle(StyleReaderInfo &, StringReader &);

/*struct RefParser {
	using Callback = Function<void(const String &, const String &)>;

	RefParser(const String &content, const Callback &cb);
	void readRef(const char *ref, size_t offset);

	size_t idx = 0;
	const char *ptr = nullptr;
	Callback func;
};*/

}

#endif /* EXTRA_DOCUMENT_DOCUMENT_SPDOCUMENTPARSER_H_ */
