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
#include "Test.h"

#if MODULE_STAPPLER_NETWORK

#include "SPNetworkHandle.h"

namespace stappler::app::test {

struct NetworkTest : Test {
	NetworkTest() : Test("NetworkTest") { }

	virtual bool run() override {
		StringStream stream;
		size_t count = 0;
		size_t passed = 0;
		stream << "\n";

		runTest(stream, "get", count, passed, [&] {
			network::Handle<Interface> h;
			h.init(network::Method::Get, "https://geobase.stappler.org/proxy/getHeaders?pretty");
			h.setVerifyTls(false);
			h.setReceiveCallback([&] (char *data, size_t size) -> size_t {
				stream << StringView(data, size);
				return size;
			});

			auto ret = h.perform();
			stream << "\n";

			return ret;
		});

		_desc = stream.str();
		return count == passed;
	}
} _NetworkTest;

}

#endif
