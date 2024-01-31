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

#include "SPCommon.h"
#include "SPDocument.h"
#include "SPDocumentHtml.cc"
#include "SPDocumentNode.cc"
#include "SPDocumentStyle.cc"
#include "SPDocumentStyleCss.cc"
#include "SPDocumentStyleContainer.cc"
#include "SPDocumentPageContainer.cc"

#include "SPDocumentFormat.cc"
#include "SPDocumentParser.cc"

namespace STAPPLER_VERSIONIZED stappler::document {

bool Document::canOpen(FilePath path, StringView ct) {
	return canOpen(memory::pool::acquire(), path, ct);
}

bool Document::canOpen(BytesView data, StringView ct) {
	return canOpen(memory::pool::acquire(), data, ct);
}

bool Document::canOpen(memory::pool_t *p, FilePath path, StringView ct) {
	return Format::canOpenDocumnt(p, path, ct);
}

bool Document::canOpen(memory::pool_t *p, BytesView data, StringView ct) {
	return Format::canOpenDocumnt(p, data, ct);
}

Rc<Document> Document::open(FilePath path, StringView ct) {
	return Document::open(memory::pool::acquire(), path, ct);
}

Rc<Document> Document::open(BytesView data, StringView ct) {
	return Document::open(memory::pool::acquire(), data, ct);
}

Rc<Document> Document::open(memory::pool_t *p, FilePath path, StringView ct) {
	return Format::openDocument(p, path, ct);
}

Rc<Document> Document::open(memory::pool_t *p, BytesView data, StringView ct) {
	return Format::openDocument(p, data, ct);
}

Document::~Document() {
	if (_pool) {
		memory::pool::destroy(_pool);
	}
}

bool Document::init() {
	_pool = memory::pool::create(memory::pool::acquire());
	return true;
}

bool Document::init(memory::pool_t *pool) {
	_pool = memory::pool::create(pool);
	return true;
}

}
