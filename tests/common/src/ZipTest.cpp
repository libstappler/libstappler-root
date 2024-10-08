/**
Copyright (c) 2022 Roman Katuntsev <sbkarr@stappler.org>
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

#ifdef MODULE_STAPPLER_ZIP

#include "SPZip.h"
#include "Test.h"

namespace STAPPLER_VERSIONIZED stappler::app::test {

struct ZipTest : Test {
	ZipTest() : Test("ZipTest") { }

	virtual bool run() override {
		auto compressedText = StringView("Compressed text\n");
		auto uncompressedText = StringView("Uncompressed text\n");

		ZipArchive<Interface> archive;

		archive.addFile("mimetype", "application/vnd.oasis.opendocument.text", true);
		archive.addFile("uncompressed.txt", BytesView((const uint8_t *)uncompressedText.data(), uncompressedText.size()), true);
		archive.addFile("compressed.txt", BytesView((const uint8_t *)compressedText.data(), compressedText.size()));

		auto buf = archive.save();
		if (!buf.empty()) {
			return true;
		}

		return false;
	}
} _ZipTest;

}

#endif
