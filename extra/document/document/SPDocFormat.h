/**
 Copyright (c) 2023-2024 Stappler LLC <admin@stappler.dev>

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

#ifndef EXTRA_DOCUMENT_DOCUMENT_SPDOCUMENTFORMAT_H_
#define EXTRA_DOCUMENT_DOCUMENT_SPDOCUMENTFORMAT_H_

#include "SPStringView.h"
#include "SPBytesView.h"
#include "SPRef.h"
#include "SPFilesystem.h"

namespace STAPPLER_VERSIONIZED stappler::document {

class Document;

using check_data_fn = bool (*) (memory::pool_t *, BytesView str, StringView ct);
using load_data_fn = Rc<Document> (*) (memory::pool_t *, BytesView , StringView ct);

using check_file_fn = bool (*) (memory::pool_t *, FilePath path, StringView ct);
using load_file_fn = Rc<Document> (*) (memory::pool_t *, FilePath path, StringView ct);

struct Format {
	check_data_fn check_data;
	check_file_fn check_file;

	load_data_fn load_data;
	load_file_fn load_file;

	size_t priority = 0;

	static bool canOpenDocumnt(memory::pool_t *, FilePath path, StringView ct = StringView());
	static bool canOpenDocumnt(memory::pool_t *, BytesView data, StringView ct = StringView());

	static Rc<Document> openDocument(memory::pool_t *, FilePath path, StringView ct = StringView());
	static Rc<Document> openDocument(memory::pool_t *, BytesView data, StringView ct = StringView());

	Format(check_file_fn, load_file_fn, check_data_fn, load_data_fn, size_t = 0);
	~Format();

	Format(const Format &) = delete;
	Format(Format &&) = delete;
	Format & operator=(const Format &) = delete;
	Format & operator=(Format &&) = delete;
};

}

#endif /* EXTRA_DOCUMENT_DOCUMENT_SPDOCUMENTFORMAT_H_ */
