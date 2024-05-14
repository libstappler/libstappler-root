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
#include "SPString.h"
#include "Test.h"
#include "SPIO.h"
#include "SPBuffer.h"
#include "SPValid.h"
#include "SPCharGroup.h"
#include "SPDso.h"
#include "SPHalfFloat.h"
#include "SPHtmlParser.h"
#include "SPSubscription.h"

namespace STAPPLER_VERSIONIZED stappler::app::test {

struct HtmlTag : html::Tag<StringView> {
	HtmlTag(StringView name)
	: Tag(name) {
		if (name == "nocontent") {
			nestedTagsAllowed = false;
		}
	}

	explicit operator bool() const { return !name.empty(); }
};

struct HtmlReader {
	using Parser = html::Parser<HtmlReader, StringView, HtmlTag>;

	using Interface = memory::PoolInterface;

	template <typename T>
	using Vector = Interface::VectorType<T>;

	using String = Interface::StringType;

	void onBeginTag(Parser &p, HtmlTag &tag) { }
	void onEndTag(Parser &p, HtmlTag &tag, bool isClosable) { }
	void onTagAttribute(Parser &p, HtmlTag &tag, StringView &name, StringView &value) { }
	void onTagAttributeList(Parser &p, HtmlTag &tag, StringView &data) {}
	void onPushTag(Parser &p, HtmlTag &tag) { }
	void onPopTag(Parser &p, HtmlTag &tag) { }
	void onInlineTag(Parser &p, HtmlTag &tag) { }
	void onTagContent(Parser &p, HtmlTag &tag, StringView &s) { }
	void onSchemeTag(Parser &p, StringView &name, StringView &value) { }
	void onCommentTag(Parser &p, StringView &data) { }

	bool shouldParseTag(Parser &p, HtmlTag &tag) {
		return tag.name != "ignored";
	}

	HtmlReader() { }
};

struct CoreTest : Test {
	CoreTest() : Test("CoreTest") { }

	virtual bool run() override {
		StringStream stream;
		size_t count = 0;
		size_t passed = 0;
		stream << "\n";

		runTest(stream, "Literals", count, passed, [&] {
			return "hashbase"_hash == "hashbase"_tag && "hashbase"_hash64 == "hashbase"_tag64
					&& "hashbase"_len == "hashbase"_length && u"hashbase"_len == u"hashbase"_length
					&& 32_c16 == char16_t(32) && 32_c8 == char(32)
					&& 1_GiB == 1024LL * 1024LL * 1024LL
					&& 1_MiB == 1024LL * 1024LL
					&& 1_KiB == 1024LL
					&& 90_to_rad == 90.0_to_rad && std::abs(42.0_to_rad - 42.0 * numbers::pi / 180.0) < epsilon();
		});

		runTest(stream, "Common math", count, passed, [&] {
			return isnan(nan())
				&& epsilon() == std::numeric_limits<float>::epsilon()
				&& maxOf<int64_t>() == std::numeric_limits<int64_t>::max()
				&& minOf<int64_t>() == std::numeric_limits<int64_t>::min()
				&& progress(1.0, 3.0, 0.5) == 2.0
				&& math::clamp(0.5, 0.0, 1.0) == 0.5
				&& math::clamp(0.5, 1.0, 0.0) == 0.5
				&& math::clamp(-0.5, 0.0, 1.0) == 0.0
				&& math::clamp(1.5, 0.0, 1.0) == 1.0
				&& math::clamp_distance(0.5, 0.0, 1.0) == 0.0
				&& math::clamp_distance(-0.5, 0.0, 1.0) == 0.5
				&& math::clamp_distance(1.5, 0.0, 1.0) == 0.5
				&& math::clamp_distance(0.5, 0.0, 1.0, -0.5) == -0.5
				&& math::clamp_distance(0.5, 0.0, 1.0, std::less<>()) == 0.0
				&& math::npot(uint32_t(63)) == 64 && math::npot(uint32_t(64)) == 64
				&& math::npot(uint64_t(63)) == 64 && math::npot(uint64_t(64)) == 64
				&& std::abs(math::to_rad(42.0) - 42.0 * numbers::pi / 180.0) < epsilon()
				&& std::abs(math::to_deg(1.0) - 180.0 / numbers::pi) < epsilon();
		});

		runTest(stream, "StringToNumber", count, passed, [&] {
			return StringToNumber<unsigned long>("FF", nullptr, 16) == 0xFF
					&& StringToNumber<unsigned long>(nullptr, nullptr, 10) == 0
					&& StringToNumber<unsigned long long>("FF", nullptr, 16) == 0xFF
					&& StringToNumber<unsigned long long>(nullptr, nullptr, 16) == 0
					&& StringToNumber<int>("FF", nullptr, 16) == 0xFF
					&& StringToNumber<int>(nullptr, nullptr, 16) == 0
					&& StringToNumber<long>("FF", nullptr, 16) == 0xFF
					&& StringToNumber<long>(nullptr, nullptr, 16) == 0
					&& StringToNumber<long long>("FF", nullptr, 16) == 0xFF
					&& StringToNumber<long long>(nullptr, nullptr, 16) == 0
					&& StringToNumber<float>("1.0", nullptr, 16) == 1.0f
					&& StringToNumber<float>(nullptr, nullptr, 16) == 0
					&& StringToNumber<double>("1.0", nullptr, 16) == 1.0f
					&& StringToNumber<double>(nullptr, nullptr, 16) == 0
					&& StringToNumber<unsigned int>("1.0", nullptr, 16) == 1.0f
					&& StringToNumber<unsigned int>(nullptr, nullptr, 16) == 0;
		});

		// Disabled for LCC - compiler bug
#ifndef __LCC__
		runTest(stream, "ValueWrapper", count, passed, [&] {
			using FloatValueWrapper = ValueWrapper<float, class FloatValueWrapperTag>;
			using IntValueWrapper = ValueWrapper<uint32_t, class IntValueWrapperTag>;

			float oneFloat = 1.0f;

			FloatValueWrapper zero(0.0f);
			FloatValueWrapper one(oneFloat);
			FloatValueWrapper two; two.set(2.0f);

			IntValueWrapper four(4);
			IntValueWrapper six(2);
			six = IntValueWrapper(four);
			six |= IntValueWrapper(2);
			six &= four;
			six ^= IntValueWrapper(2);
			six -= IntValueWrapper(4);
			six += IntValueWrapper(1);
			six *= IntValueWrapper(6);
			six /= IntValueWrapper(3);
			++ six; -- six;
			six ++; six --;

			uint32_t tenInt = 10;
			IntValueWrapper ten(0);
			ten.set(tenInt);
			ten = six;
			ten += IntValueWrapper(4);

			return FloatValueWrapper::max().get() == maxOf<FloatValueWrapper::Type>()
				&& FloatValueWrapper::min().get() == minOf<FloatValueWrapper::Type>()
				&& FloatValueWrapper::zero().get() == 0.0f
				&& FloatValueWrapper::epsilon().get() == epsilon<FloatValueWrapper::Type>()
				&& one == progress(zero, two, 0.5f)
				&& zero.empty()
				&& one < two && one <= two
				&& FloatValueWrapper().get() != nan()
				&& six == IntValueWrapper(6) && ten.get() == tenInt
				&& FloatValueWrapper(one) != two
				&& two > one && two >= FloatValueWrapper(move(one))
				&& -FloatValueWrapper(1.0f) == FloatValueWrapper(-1.0f)
				&& (IntValueWrapper(2) | IntValueWrapper(4)) == IntValueWrapper(6)
				&& (IntValueWrapper(6) & IntValueWrapper(4)) == IntValueWrapper(4)
				&& (IntValueWrapper(2) ^ IntValueWrapper(4)) == IntValueWrapper(6)
				&& (IntValueWrapper(4) - IntValueWrapper(2)) == IntValueWrapper(2)
				&& (IntValueWrapper(4) * IntValueWrapper(2)) == IntValueWrapper(8)
				&& (IntValueWrapper(4) / IntValueWrapper(2)) == IntValueWrapper(2)
				&& std::hash<IntValueWrapper>()(ten)
				;
		});
#endif

		runTest(stream, "Result", count, passed, [&] {
			String data("12345");
			StringView r(data);

			String invalidStr("asdfgh");
			StringView r2(invalidStr);

			Result<int64_t> defaultResult;
			int64_t intVal = 0;
			auto res = r.readInteger(10);
			auto invalid = r2.readInteger(10);
			return res.valid() && res.unwrap([&] (auto val) {
				intVal = val;
			})
			&& res.grab(intVal)
			&& !invalid.unwrap([&] (auto val) {
				intVal = val;
			})
			&& !invalid.grab(intVal)
			&& !defaultResult.valid();
		});

		runTest(stream, "itoa/dtoa", count, passed, [&] {
			WideString wout;
			String out;

			auto cb = [&] (StringView iout) {
				out = iout.str<memory::StandartInterface>();
			};

			auto wcb = [&] (WideStringView iout) {
				wout = iout.str<memory::StandartInterface>();
			};

			Callback<void(StringView)> callback(cb);
			Callback<void(WideStringView)> wcallback(wcb);

			callback << std::numeric_limits<double>::quiet_NaN();
			callback << std::numeric_limits<double>::infinity();
			callback << -std::numeric_limits<double>::infinity();

			callback << double(12.34);
			wcallback << double(12.34);
			auto retd1 = out == "12.34" && wout == u"12.34";

			callback << int64_t(1234);
			wcallback << int64_t(1234);
			auto ret1 = out == "1234" && wout == u"1234";

			callback << int64_t(-1234);
			wcallback << int64_t(-1234);
			auto ret2 = out == "-1234" && wout == u"-1234";

			callback << int64_t(-1234567890123456);
			wcallback << int64_t(-1234567890123456);
			auto ret3 = out == "-1234567890123456" && wout == u"-1234567890123456";

			callback << int64_t(1234567890123456);
			wcallback << int64_t(1234567890123456);
			auto ret4 = out == "1234567890123456" && wout == u"1234567890123456";

			callback << int64_t(12345);
			wcallback << int64_t(12345);
			auto ret5 = out == "12345" && wout == u"12345";

			callback << size_t(12345);
			wcallback << size_t(12345);

			callback << int64_t(-12345);
			wcallback << int64_t(-12345);
			auto ret6 = out == "-12345" && wout == u"-12345";

			callback << char32_t('a');
			wcallback << char32_t('a');

			callback << char16_t('a');
			wcallback << char16_t('a');

			callback << char('a');
			wcallback << char('a');

			return ret1 && ret2 && ret3 && ret4 && ret5 && ret6 && retd1;
		});

		runTest(stream, "toString", count, passed, [&] {
			auto str1 = string::toString<Interface>(double(12.34));
			auto str2 = string::toString<Interface>(-1234567890123456);
			auto str3 = string::toString<Interface>(size_t(1234567890123456));
			auto str4 = string::toString<Interface>("test string");
			auto str5 = string::toString<Interface>(StringView("test string1"), ' ', 1234, " & ", 12.34, ' ', String("test string 2"));

			StringView v(str1);
			v.is<'t'>();
			v.begin();
			v.end();

			StringViewUtf8 vUtf8;
			StringViewUtf8 vUtf8_2("ТЕСТ");
			StringViewUtf8 vUtf8_3(vUtf8_2, 2);
			vUtf8_3.is("Т");
			vUtf8_3.offset(1);
			vUtf8_3 ++;
			vUtf8_2 += 2;

			StringViewUtf8 vUtf8_4("ТЕСТ");
			vUtf8_4.foreach([] (char32_t) {

			});

			string::_strncasecmp(u"ТЕСТ", u"ТЕСТ", 4);

			string::toUtf8<Interface>(char16_t('A'));
			string::toUtf8<Interface>(char32_t('A'));
			string::toKoi8r<Interface>(WideStringView(u"ТЕСТ"));
			string::toKoi8r<Interface>(WideStringView(u"0йцукенгшщзхъфывапролджэячсмитьбюё№ЙЦУКЕНГШЩЗХЪФЫВАПРОЛДЖЭЯЧМИТЬБЮЁ123456789"));

			for (char16_t i = 0; i < char16_t(0x3000); ++ i) {
				string::charToKoi8r(i);
			}

			string::toUtf16Html<Interface>("&quot;&apos;&shy;&asdf;");
			string::getUtf8HtmlLength("&quot;&apos;&shy;&asdf;");

			StringView::merge<Interface>("test1", "test2", "test3");

			string::isValidUtf8("ТЕСТ" "𐍈한€И");

			unicode::utf8EncodeLength(char32_t(0x12'345F));
			unicode::utf8EncodeLength(char32_t(0x10'945F));
			unicode::utf8EncodeLength(char32_t(0x00'945F));
			unicode::utf8EncodeLength(char32_t(0x00'145F));

			unicode::utf16EncodeLength(char32_t(0x12'345F));
			unicode::utf16EncodeLength(char32_t(0x10'945F));
			unicode::utf16EncodeLength(char32_t(0x00'945F));
			unicode::utf16EncodeLength(char32_t(0x00'145F));

			memory::string memStr;
			unicode::utf8Encode(memStr, char32_t(0x12'345F));
			unicode::utf8Encode(memStr, char32_t(0x10'945F));
			unicode::utf8Encode(memStr, char32_t(0x00'945F));
			unicode::utf8Encode(memStr, char32_t(0x00'145F));
			unicode::utf8Encode(memStr, char16_t(0x00'945F));
			unicode::utf8Encode(memStr, char16_t(0x00'145F));

			memory::u16string memWideStr;
			unicode::utf16Encode(memWideStr, char32_t(0x12'345F));
			unicode::utf16Decode32(memWideStr.data());

			unicode::utf16Encode(memWideStr, char32_t(0x10'945F));
			unicode::utf16Encode(memWideStr, char32_t(0x00'945F));
			unicode::utf16Encode(memWideStr, char32_t(0x00'145F));
			unicode::utf16Encode(memWideStr, char32_t(0xDFFF));
			unicode::utf16Encode(memWideStr, char32_t(0xDFFF + 8));

			return str1 == "12.34"
					&& str2 == "-1234567890123456"
					&& str3 == "1234567890123456"
					&& str4 == "test string"
					&& str5 == "test string1 1234 & 12.34 test string 2";
		});

		runTest(stream, "toupper/tolower", count, passed, [&] {
			StringViewUtf8 str("тестовая Строка");
			auto s1 = string::tolower<memory::StandartInterface>(str);
			auto s2 = string::toupper<memory::StandartInterface>(str);
			auto s3 = string::totitle<memory::StandartInterface>(str);

			string::tolower(char32_t(0x00'945F));

			if (s1 == StringViewUtf8("тестовая строка") &&  s2 == StringViewUtf8("ТЕСТОВАЯ СТРОКА")) {
				return true;
			} else {
				std::cout << "'" << s1 << "': '" <<  "тестовая строка" << "' '" << s2 << "': '" << "ТЕСТОВАЯ СТРОКА" << "'\n";
				return false;
			}
		});
		runTest(stream, "IO", count, passed, [&] {
			auto path = filesystem::currentDir<Interface>("resources/mnist/t10k-images.idx3-ubyte");

			do {
				auto d = filesystem::readIntoMemory<Interface>(path);
				CoderSource s((const char *)d.data(), d.size());
				s.seek(20, io::Seek::Current);
				s.seek(d.size() + 20, io::Seek::Current);
				s.seek(-d.size() -20, io::Seek::Current);
				s.seek(20, io::Seek::Set);
				s.seek(-20, io::Seek::Set);
				s.seek(d.size() + 20, io::Seek::Set);
				s.seek(0, io::Seek::Set);
				s.seek(20, io::Seek::End);
				s.seek(-20, io::Seek::End);
				s.seek(-d.size() -20, io::Seek::End);
				s.tell();

				BufferTemplate<Interface> buf2(24);
				auto bufWrapper = io::Buffer(buf2);

				bufWrapper.size();
				bufWrapper.capacity();
				bufWrapper.data();
				bufWrapper.clear();

				size_t sz = 0;
				auto target = bufWrapper.prepare(sz);
				memcpy(target, d.data(), sz);
				bufWrapper.save(target, sz, sz);

				buf2.read(12);
				buf2.seek(d.size() + 10);
				buf2.pop(10);

			} while (0);

			CoderSource source("test", 4);

			do {
				auto f = filesystem::openForReading(path);
				if (f) {
					io::read(f, [] (const io::Buffer &) { });
				}
			} while (0);

			do {
				StackBuffer<10_KiB> buf;
				auto f = filesystem::openForReading(path);
				if (f) {
					io::read(f, io::Buffer(buf), [] (const io::Buffer &) { });
				}
			} while (0);

			do {
				StringStream out;
				StackBuffer<10_KiB> buf;
				auto f = filesystem::openForReading(path);
				if (f) {
					io::read(f, io::Consumer(out));
				}
			} while (0);

			do {
				StringStream out;
				StackBuffer<10_KiB> buf;
				auto f = filesystem::openForReading(path);
				if (f) {
					io::read(f, io::Consumer(out), [] (const io::Buffer &) { });
				}
			} while (0);

			do {
				StringStream out;
				StackBuffer<10_KiB> buf;
				auto f = filesystem::openForReading(path);
				if (f) {
					io::read(f, io::Consumer(out), io::Buffer(buf));
				}
			} while (0);

			do {
				StringStream out;
				StackBuffer<10_KiB> buf;
				auto f = filesystem::openForReading(path);
				if (f) {
					io::read(f, io::Consumer(out), io::Buffer(buf), [] (const io::Buffer &) { });
				}
			} while (0);

			return true;
		});

		runTest(stream, "base64", count, passed, [&] {
			StringStream stream;
			StackBuffer<1_KiB> buf;
			StackBuffer<1_KiB> buf2;

			auto bytes = valid::makeRandomBytes<Interface>(16);

			base64::encode(stream, bytes);

			base64::encode((char *)buf.data(), 10, CoderSource(bytes));
			auto len = base64::encode((char *)buf.data(), buf.capacity(), CoderSource(bytes));
			base64::decode(stream, CoderSource(buf.data(), len));
			base64::decode(buf2.data(), 10, CoderSource(buf.data(), len));
			base64::decode(buf2.data(), buf2.capacity(), CoderSource(buf.data(), len));

			base64url::encode(stream, bytes);

			len = base64url::encode((char *)buf.data(), 10, CoderSource(bytes));
			len = base64url::encode((char *)buf.data(), buf.capacity(), CoderSource(bytes));
			base64url::decode(stream, CoderSource(buf.data(), len));
			base64url::decode(buf2.data(), buf2.capacity(), CoderSource(buf.data(), len));
			base64url::decode(buf2.data(), 10, CoderSource(buf.data(), len));

			base16::encodeSize(10);
			base16::decodeSize(10);
			base16::encode(stream, bytes);
			base16::encode([] (char) {}, bytes);
			len = base16::encode((char *)buf.data(), buf.capacity(), bytes);
			base16::decode<mem_pool::Interface>(CoderSource(buf.data(), len));
			base16::decode(stream, CoderSource(buf.data(), len));
			base16::decode([] (uint8_t) { }, CoderSource(buf.data(), len));
			base16::decode(buf2.data(), buf2.capacity(), CoderSource(buf.data(), len));

			return true;
		});

		runTest(stream, "BytesView", count, passed, [&] {
			auto p = memory::pool::create(memory::app_root_pool);
			auto bytes = valid::makeRandomBytes<Interface>(16);
			BytesView view(bytes);
			view.sub();

			BytesViewTemplate<Endian::Network> viewNet(view, 10);
			BytesViewTemplate<Endian::Network> viewNet2(view, 1, 10);
			viewNet2.pdup(p);
			view.readUnsigned24();

			return true;
		});

		runTest(stream, "CharGroup", count, passed, [&] {
			for (uint32_t i = 0; i <= toInt(CharGroupId::TextPunctuation); ++ i) {
				inCharGroup(CharGroupId(i), u'№');
				inCharGroupMask(CharGroupId(i), u'№');
			}
			chars::CharGroup<char, CharGroupId::PunctuationBasic>::match(',');
			chars::CharGroup<char, CharGroupId::TextPunctuation>::match(',');
			chars::isxdigit('a');

			chars::CharGroup<char, CharGroupId::PunctuationBasic>::foreach([] (char) { });
			chars::CharGroup<char, CharGroupId::TextPunctuation>::foreach([] (char) { });

			return true;
		});

		runTest(stream, "HTML", count, passed, [&] {
			auto parseHtml = [] (StringView str) {
				HtmlReader reader;
				html::parse<HtmlReader, StringView, HtmlTag>(reader, StringView(str), false);
			};

			auto readTag = [] (auto str) {
				html::Tag_readName(str);
			};

			auto readAttr = [] (auto str) {
				html::Tag_readAttrName(str);
			};

			auto readValue = [] (auto str) {
				html::Tag_readAttrValue(str);
			};

			parseHtml("<html><nocontent> content <p>test</p> 123213 </nocontent></html>");
			parseHtml("<html><content> content \"<p>test</p>\" 123213 </html>");
			parseHtml("<html><content> content '<p>test</p>' 123213 </html>");
			parseHtml("<ignored><!-- --><html><тест></html></ignored>");
			parseHtml("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"http://www.w3.org/TR/xht\\ml1/DTD/xhtml1-strict.dtd\">");
			parseHtml("<!DOCTYPE html PUBLIC '-//W3C//DTD XHTML 1.0 Strict//EN' 'http://www.w3.org/TR/xhtm\\l1/DTD/xhtml1-strict.dtd'>");

			parseHtml("<!doctype><![CDATA[123456]]><html><![CDATA[123456]]></html>");
			parseHtml("<html></");
			parseHtml("<html><тест></html>");
			parseHtml("<html><!-- --></html>");

			readTag(StringView("!--"));
			readTag(StringView("html"));
			readTag(StringView("/html"));
			readTag(StringView("html/"));
			readAttr(StringView("attribute"));
			readValue(StringView("attribute"));
			readValue(StringView("=attribute"));
			readValue(StringView("=\"attribute\""));
			readValue(StringView("=\"att\\\\ri\\\"bute\""));
			readValue(StringView("='attribute'"));
			readValue(StringView("='at\\\\tri\\'bute'"));

			readTag(StringViewUtf8("!--"));
			readTag(StringViewUtf8("html"));
			readTag(StringViewUtf8("/html"));
			readTag(StringViewUtf8("html/"));
			readAttr(StringViewUtf8("attribute"));
			readValue(StringViewUtf8("attribute"));
			readValue(StringViewUtf8("=attribute"));
			readValue(StringViewUtf8("=\"attribute\""));
			readValue(StringViewUtf8("=\"att\\\\ri\\\"bute\""));
			readValue(StringViewUtf8("='attribute'"));
			readValue(StringViewUtf8("='at\\\\tri\\'bute'"));

			readTag(WideStringView(u"!--"));
			readTag(WideStringView(u"html"));
			readTag(WideStringView(u"/html"));
			readTag(WideStringView(u"html/"));
			readAttr(WideStringView(u"attribute"));
			readValue(WideStringView(u"attribute"));
			readValue(WideStringView(u"=attribute"));
			readValue(WideStringView(u"=\"attribute\""));
			readValue(WideStringView(u"=\"att\\\\ri\\\"bute\""));
			readValue(WideStringView(u"='attribute'"));
			readValue(WideStringView(u"='at\\\\tri\\'bute'"));

			return true;
		});

		runTest(stream, "Custom log", count, passed, [&] {
			do {
				log::CustomLog custom1([] (log::LogType, StringView, log::CustomLog::Type, log::CustomLog::VA &) {
					return true;
				});

				log::CustomLog custom2([] (log::LogType, StringView, log::CustomLog::Type, log::CustomLog::VA &) {
					return false;
				});

				log::CustomLog custom3(move(custom2));
				custom3 = move(custom2);

				auto str = "f241b1fb1eb7e3a9e246e1457c005fc262d0540820f827b11b2316e8e916e7cd908d4"
						"3ef760f0e5a25522ef99c6372ed33c2dc9fd7efc9c4a79a076866fb6a325de40d509bb7d"
						"71b31ba2d58e483a330715ad9bb25616c5d898c05b13a17a31d58a843e95a1d5e8463d4a"
						"8a2abbdedb28c83cdd053c6e0039b769ab49ee45424f31186a80ec436821d9074f895418"
						"61321f57f3dbd3ddc8d7b87f0ea701a22e9dd163ec806a1d44f41c29ae1f877fcad0b672"
						"3073e8204015f467ec624972a48eab963dc4a886bd3ea9677518fd70b38a43e5c58552a4"
						"ea6ec4223b07c483bd44efb065a329ad8afcce43c8d6d548f58ec8922181be9db7f435ca"
						"44402f9ff1a484afe789443bb9afb40c2fa26a0aadea13ac3c6fb85e629a5c6d1ed0ee63f"
						"4a05f04e3cc1f97cb79b94c75a756538d80ff1be150161a85b898c39f9e94f5cafb5bb26"
						"12a8b634993c230fb66a954057b1570ceb35aef1e49bb3500c057ef8ef5d30f70ba700bf"
						"bae984d0a1ea483a8281e50ab7ca8f0f0b7090cb4a01d07f5e48318a47029d6447b9bd96"
						"c21f128e235dba1e3f1ea89ec71c470a7beed364c5d9c5fd3e142bf7737b445a3e96c582"
						"035eb1f24b233279d30f632feb3b816270dfb664eea524063311caff9e70270911eadc59"
						"0faae0b7d45bebacd96df211421b5e88102127afa8043b7016337ee1cde722e30ede50c5"
						"eb48d6f647d4b715c32a5c01eebb4ffc6f01c12284f11901efba24f39b57017d30daf5a55"
						"50b37b8e11e801aac881c01b87f8fa723bd72f85ac729705ff8185ea434351b82a11377e"
						"6b5555cca2aa64ee7fce8d33d9812c932f6da389929e0e8bf79ca924d5c543deaad341a8"
						"fbc5d155f6e9f4e1b7e778208fac5a0cb4b586c285e05fd58ac6ad886d4e621fe102bfa7"
						"faa91591a59d8556adfa2aab7be8d0d70ec80b5219e73e3693b172704099b5cb7a7d329f"
						"036ff759450d1b312b0afc908e2ac25f0fc5de4e3979e0a3b4f6bfd32ff15158d4349319"
						"efd4a6e82b5a14e2ef7a9b546088a88702c08d4afc6b21be28a85e0291aa654c6994813e"
						"e9a06270ae837bcf657d51d4e";

				log::format(log::Fatal, "LogTess", "%s %s %d", "String", str, 1234);
				log::text(log::Fatal, "LogTess", "Text string");
			} while (0);
			do {
				log::CustomLog custom1([] (log::LogType, StringView, log::CustomLog::Type, log::CustomLog::VA &) {
					return true;
				});

				log::CustomLog custom2([] (log::LogType, StringView, log::CustomLog::Type, log::CustomLog::VA &) {
					return true;
				});

				log::format(log::Fatal, "LogTess", "%s %d", "String", 1234);
				log::text(log::Fatal, "LogTess", "Text string");
			} while (0);

			log::setLogFilterMask(log::getlogFilterMask());

			return true;
		});
		runTest(stream, "Subscription", count, passed, [&] {
			Subscription::Id id1 = Subscription::getNextId();
			Subscription::Id id3 = Subscription::getNextId();

			Subscription sub1;
			Subscription sub2;

			sub2.setForwardedSubscription(&sub1);

			sub2.subscribe(id1);
			sub1.subscribe(id3);

			sub1.setDirty(SubscriptionFlags(2), false);
			sub2.setDirty(SubscriptionFlags(4), true);

			auto test1 = sub2.check(id1) == SubscriptionFlags(7);
			auto test2 = sub1.check(id3) == SubscriptionFlags(3);

			sub1.unsubscribe(id3);
			sub2.unsubscribe(id1);

			return test1 && test2;
		});

		Dso dso(StringView(), DsoFlags::Self);
		dso.getError();

		halffloat::nan();
		halffloat::posinf();
		halffloat::neginf();
		halffloat::encode(std::numeric_limits<float>::quiet_NaN());
		halffloat::encode(std::numeric_limits<float>::infinity());
		halffloat::encode(-std::numeric_limits<float>::infinity());
		halffloat::encode(0.0f);

		_desc = stream.str();
		return count == passed;
	}
} _CoreTest;

}
