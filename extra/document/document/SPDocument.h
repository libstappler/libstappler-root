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

#ifndef EXTRA_DOCUMENT_DOCUMENT_SPDOCUMENT_H_
#define EXTRA_DOCUMENT_DOCUMENT_SPDOCUMENT_H_

#include "SPRef.h"
#include "SPFilesystem.h"
#include "SPBytesView.h"
#include "SPMemory.h"

namespace STAPPLER_VERSIONIZED stappler::document {

class Document : public RefBase<memory::StandartInterface> {
public:
	static bool canOpen(FilePath path, StringView ct = StringView());
	static bool canOpen(BytesView data, StringView ct = StringView());
	static bool canOpen(memory::pool_t *, FilePath path, StringView ct = StringView());
	static bool canOpen(memory::pool_t *, BytesView data, StringView ct = StringView());

	static Rc<Document> open(FilePath path, StringView ct = StringView());
	static Rc<Document> open(BytesView data, StringView ct = StringView());
	static Rc<Document> open(memory::pool_t *, FilePath path, StringView ct = StringView());
	static Rc<Document> open(memory::pool_t *, BytesView data, StringView ct = StringView());

	virtual ~Document();

	virtual bool init();
	virtual bool init(memory::pool_t *);

protected:
	memory::pool_t *_pool = nullptr;
};

}

#endif /* EXTRA_DOCUMENT_DOCUMENT_SPDOCUMENT_H_ */
