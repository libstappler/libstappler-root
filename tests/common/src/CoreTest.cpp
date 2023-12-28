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

namespace stappler::app::test {

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
				&& FloatValueWrapper().get() != nan()
				&& one == progress(zero, two, 0.5f)
				&& zero.empty()
				&& FloatValueWrapper(one) != two
				&& one < two && one <= two
				&& two > one && two >= FloatValueWrapper(move(one))
				&& six == IntValueWrapper(6) && ten.get() == tenInt
				&& (IntValueWrapper(2) | IntValueWrapper(4)) == IntValueWrapper(6)
				&& (IntValueWrapper(6) & IntValueWrapper(4)) == IntValueWrapper(4)
				&& (IntValueWrapper(2) ^ IntValueWrapper(4)) == IntValueWrapper(6)
				&& (IntValueWrapper(4) - IntValueWrapper(2)) == IntValueWrapper(2)
				&& (IntValueWrapper(4) * IntValueWrapper(2)) == IntValueWrapper(8)
				&& (IntValueWrapper(4) / IntValueWrapper(2)) == IntValueWrapper(2)
				&& -FloatValueWrapper(1.0f) == FloatValueWrapper(-1.0f)
				&& std::hash<IntValueWrapper>()(ten)
				;
		});

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

			callback << int64_t(-12345);
			wcallback << int64_t(-12345);
			auto ret6 = out == "-12345" && wout == u"-12345";

			return ret1 && ret2 && ret3 && ret4 && ret5 && ret6 && retd1;
		});

		runTest(stream, "toupper/tolower", count, passed, [&] {
			StringViewUtf8 str("тестовая Строка");
			auto s1 = string::tolower<memory::StandartInterface>(str);
			auto s2 = string::toupper<memory::StandartInterface>(str);
			auto s3 = string::totitle<memory::StandartInterface>(str);

			if (s1 == StringViewUtf8("тестовая строка") &&  s2 == StringViewUtf8("ТЕСТОВАЯ СТРОКА")) {
				return true;
			} else {
				std::cout << "'" << s1 << "': '" <<  "тестовая строка" << "' '" << s2 << "': '" << "ТЕСТОВАЯ СТРОКА" << "'\n";
				return false;
			}
		});

		_desc = stream.str();
		return count == passed;
	}
} _CoreTest;

}
