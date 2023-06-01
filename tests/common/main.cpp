/**
Copyright (c) 2017 Roman Katuntsev <sbkarr@stappler.org>
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
#include "SPTime.h"
#include "Test.h"

#if MODULE_STAPPLER_DATA
#include "SPData.h"
#endif

static constexpr auto HELP_STRING(
R"HelpString(sptest <options> <test-name|all>
Options are one of:
    -v (--verbose)
    -h (--help))HelpString");


namespace stappler::app::test {

struct StringToNumberTest : Test {
	StringToNumberTest() : Test("StringToNumberTest") { }

	template <typename T>
	bool runTest(StringStream &stream, const T &t) {
		auto n = StringToNumber<T>(toString(t));
		stream << "\t" << t << " -> " << toString(t) << " -> " << n << " -> " << (n == t) << "\n";
		return n == t;
	}

	template <typename T>
	bool runFloatTest(StringStream &stream, const T &t) {
		auto n = StringToNumber<T>(toString(t));
		stream << "\t" << t << " -> " << toString(t) << " -> " << n << " -> " << (toString(n) == toString(t)) << "\n";
		return toString(n) == toString(t);
	}

	virtual bool run() {
		StringStream stream;
		size_t pass = 0;

		stream << "\n";

		if (runTest(stream, rand_int64_t())) { ++ pass; }
		if (runTest(stream, rand_uint64_t())) { ++ pass; }
		if (runTest(stream, rand_int32_t())) { ++ pass; }
		if (runTest(stream, rand_uint32_t())) { ++ pass; }
		if (runFloatTest(stream, rand_float())) { ++ pass; }
		if (runFloatTest(stream, rand_double())) { ++ pass; }

		_desc = stream.str();

		if (pass == 6) {
			return true;
		}

		return false;
	}
} _StringToNumberTest;

struct UnicodeTest : Test {
	UnicodeTest() : Test("UnicodeTest") { }

	virtual bool run() {
		String str("Идейные соображения высшего порядка, а также начало повседневной работы по формированию позиции");
		WideString wstr(u"Идейные соображения высшего порядка, а также начало повседневной работы по формированию позиции");
		String strHtml("Идейные&nbsp;&lt;соображения&gt;&amp;работы&#x410;&#x0411;&#1042;&#1043;");

		StringStream stream;
		size_t pass = 0;

		stream << "\n\tUtf8 -> Utf16 -> Utf8 test";
		if (str == string::toUtf8<Interface>(string::toUtf16<Interface>(str))) {
			stream << " passed\n";
			++ pass;
		} else {
			stream << " failed\n";
		}

		stream << "\tUtf16 -> Utf8 -> Utf16 test";
		if (wstr == string::toUtf16<Interface>(string::toUtf8<Interface>(wstr))) {
			stream << " passed\n";
			++ pass;
		} else {
			stream << " failed\n";
		}

		stream << "\tUtf16Html \"" << string::toUtf8<Interface>(string::toUtf16Html<Interface>(strHtml)) << "\"";
		if (u"Идейные <соображения>&работыАБВГ" == string::toUtf16Html<Interface>(strHtml)) {
			stream << " passed\n";
			++ pass;
		} else {
			stream << " failed\n";
		}
		_desc = stream.str();
		return pass == 3;
	}
} _UnicodeTest;

struct TimeTest : Test {
	TimeTest() : Test("TimeTest") { }

	virtual bool run() {
		StringStream stream;

		auto now = Time::now();

		bool success = true;

		for (int i = 0; i <= 10; ++ i) {
			auto t = now + TimeInterval::milliseconds( (i == 0) ? 0 : rand());

			auto ctime = t.toCTime<Interface>();
			auto http = t.toHttp<Interface>();

			auto t1 = Time::fromHttp(http);
			auto t2 = Time::fromHttp(ctime);

			stream << "\n\t" << t.toSeconds() << " | Rfc822: " << http << " | " << t1.toSeconds() << " " << (t1.toSeconds() == t.toSeconds());
			stream << " | CTime: " << ctime << " | " << t2.toSeconds() << " " << (t2.toSeconds() == t.toSeconds());

			if (!(t1.toSeconds() == t.toSeconds() && t2.toSeconds() == t.toSeconds())) {
				success = false;
			}
		}

		for (int i = 0; i <= 10; ++ i) {
			auto t = now + TimeInterval::milliseconds( (i == 0) ? 0 : rand());

			auto xml = t.toIso8601<Interface>(6);

			auto t1 = Time::fromHttp(xml);

			stream << "\n\t" << t.toMicros() << " | Iso8601: " << xml << " | " << t1.toMicros() << " "
					<< t1.toIso8601<Interface>(3) << " " << (t1.toMicros() == t.toMicros());

			if (!(t1.toMicros() == t.toMicros())) {
				success = false;
			}
		}

		_desc = stream.str();
		return success;
	}
} _TimeTest;

#if MODULE_STAPPLER_DATA
int parseOptionSwitch(Value &ret, char c, const char *str) {
	if (c == 'h') {
		ret.setBool(true, "help");
	} else if (c == 'v') {
		ret.setBool(true, "verbose");
	}
	return 1;
}

int parseOptionString(Value &ret, const StringView &str, int argc, const char * argv[]) {
	if (str == "help") {
		ret.setBool(true, "help");
	} else if (str == "verbose") {
		ret.setBool(true, "verbose");
	} else if (str == "gencbor") {
		ret.setBool(true, "gencbor");
	}
	return 1;
}
#endif

SP_EXTERN_C int _spMain(argc, argv) {
#if MODULE_STAPPLER_DATA
	Value opts = data::parseCommandLineOptions<Interface>(argc, argv,
			&parseOptionSwitch, &parseOptionString);
	if (opts.getBool("help")) {
		std::cout << HELP_STRING << "\n";
		return 0;
	}

	if (opts.getBool("verbose")) {
#if MODULE_STAPPLER_FILESYSTEM
		std::cout << " Current work dir: " << stappler::filesystem::currentDir<Interface>() << "\n";
		std::cout << " Documents dir: " << stappler::filesystem::documentsPathReadOnly<Interface>() << "\n";
		std::cout << " Cache dir: " << stappler::filesystem::cachesPathReadOnly<Interface>() << "\n";
		std::cout << " Writable dir: " << stappler::filesystem::writablePathReadOnly<Interface>() << "\n";
#endif
		std::cout << " Options: " << stappler::data::EncodeFormat::Pretty << opts << "\n";
	}
#endif

	auto mempool = memory::pool::create();
	memory::pool::push(mempool);

	if (argc > 1) {
		StringView testName(argv[1]);
		if (testName == "all") {
			Test::RunAll();
		} else {
			for (int i = 1; i < argc; ++ i) {
				Test::Run(StringView(argv[i]));
			}
		}
	} else {
		Test::RunAll();
	}

	memory::pool::pop();

	return 0;
}

}
