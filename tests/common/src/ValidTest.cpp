/**
Copyright (c) 2019 Roman Katuntsev <sbkarr@stappler.org>
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

#include "SPString.h"
#include "SPCommon.h"
#include "SPTime.h"
#include "SPValid.h"
#include "Test.h"

#if MODULE_STAPPLER_DATA

#include "SPData.h"
#include "SPIdn.h"

namespace STAPPLER_VERSIONIZED stappler::app::test {

// test vectors from https://tools.ietf.org/html/rfc4231#section-4.1

struct ValidTest : MemPoolTest {
	ValidTest() : MemPoolTest("ValidTest") { }

	static uint32_t makeIp(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
		return (a << 24) | (b << 16) | (c << 8) | d;
	};

	virtual bool run(pool_t *) override {
		StringStream stream; stream << "\n";

		uint32_t failed = 0;

		valid::readIp("123.45.67.89");
		valid::readIp("123.45.89");
		valid::readIp("123.45.89.12.34");
		valid::readIp("123.345.89.12");

		data::ValueTemplate<memory::PoolInterface> emails;
		if (valid::readIpRange("123.45.67.89-98.76.54.32") == pair(makeIp(123,45,67,89), makeIp(98,76,54,32))) {
			stream << "123.45.67.89-98.76.54.32 : [valid]\n";
		} else {
			stream << "123.45.67.89-98.76.54.32 : [failed]\n";
			++ failed;
		}
		if (valid::readIpRange("127.0.0.123/24") == pair(makeIp(127,0,0,0), makeIp(127,0,0,255))) {
			stream << "127.0.0.123/24 : [valid]\n";
		} else {
			stream << "127.0.0.123/24 : [failed]\n";
			++ failed;
		}
		if (valid::readIpRange("127.0.0.123/255.255.0.0") == pair(makeIp(127,0,0,0), makeIp(127,0,255,255))) {
			stream << "127.0.0.123/255.255.0.0 : [valid]\n";
		} else {
			stream << "127.0.0.123/255.255.0.0 : [failed]\n";
			++ failed;
		}
		if (valid::readIpRange("127.0.0.123") == pair(makeIp(127,0,0,123), makeIp(127,0,0,123))) {
			stream << "127.0.0.123 : [valid]\n";
		} else {
			stream << "127.0.0.123 : [failed]\n";
			++ failed;
		}

		if (valid::readIpRange("127.0.256.123") == pair(uint32_t(0), uint32_t(0))) {
			stream << "127.0.256.123 : [failed-correct]\n";
		} else {
			stream << "127.0.256.123 : [valid]\n";
			++ failed;
		}
		if (valid::readIpRange("127.0.254.123/32") == pair(uint32_t(0), uint32_t(0))) {
			stream << "127.0.254.123/32 : [failed-correct]\n";
		} else {
			stream << "127.0.254.123/32 : [valid]\n";
			++ failed;
		}
		if (valid::readIpRange("127.0.0.123/255.255.0.1") == pair(uint32_t(0), uint32_t(0))) {
			stream << "127.0.0.123/255.255.0.1 : [failed-correct]\n";
		} else {
			stream << "127.0.0.123/255.255.0.1 : [valid]\n";
			++ failed;
		}

		data::ValueTemplate<memory::PoolInterface> urls;
		urls.addString("ssh://git@atom0.stappler.org:21002/StapplerApi.git?qwr#test");
		urls.addString("https://йакреведко.рф/test/.././..////?query[креведко][treas][ds][]=qwert#аяклешня");
		urls.addString("localhost");
		urls.addString("localhost:8080");
		urls.addString("localhost/test1/test2");
		urls.addString("/usr/local/bin/bljad");
		urls.addString("https://localhost/test/.././..////?query[test][treas][ds][]=qwert#rest");
		urls.addString("http://localhost");
		urls.addString("йакреведко.рф");

		for (auto &it : urls.asArray()) {
			std::string str2(StringView(it.getString()).str<mem_std::Interface>());
			memory::string str(it.getString());
			if (!valid::validateUrl(str) || !valid::validateUrl(str2)) {
				stream << "Url: [invalid] " << str << "\n";
				++ failed;
			} else {
				stream << "Url: [valid] " << it.getString() << " >> " << str << "\n";
			}
		}

		emails.addString(" (test comment) user(test comment)@(test comment)localserver(test comment) ");
		emails.addString("аяклешня@xn--80aegcbuie0bo.xn--p1ai");
		emails.addString("аяклешня@йакреведко.рф");
		emails.addString("prettyandsimple@example.com");
		emails.addString("йакреведко@упячка.рф");
		emails.addString("very.common@example.com");
		emails.addString("disposable.style.email.with+symbol@example.com");
		emails.addString("other.email-with-dash@example.com");
		emails.addString("\"much.more unusual\"@example.com");
		emails.addString("\"very.unusual.@.unusual.com\"@example.com");
		emails.addString("\"very.(),:;<>[]\\\".VERY.\\\"very@\\\\ \\\"very\\\".unusual\"@strange.example.com");
		emails.addString("admin@mailserver1");
		emails.addString("#!$%&'*+-/=?^_`{}|~@example.org");
		emails.addString("\"()<>[]:,;@\\\"!#$%&'*+-/=?^_`{}| ~.a\"@example.org");
		emails.addString("\" \"@example.org");
		emails.addString("example@localhost");
		emails.addString("example@s.solutions");
		emails.addString("user@com");
		emails.addString("user@localserver");
		emails.addString("user@[IPv6:2001:db8::1]");

		for (auto &it : emails.asArray()) {
			memory::string str(it.getString());
			if (!valid::validateEmail(str) || !valid::validateEmailWithoutNormalization(it.getString())) {
				stream << "Email: [invalid] " << str << "\n";
				++ failed;
			} else {
				stream << "Email: [valid] " << it.getString() << " >> " << str << "\n";
			}
		}

		emails.clear();

		// should fail
		emails.addString("john.doe@failed..com");
		emails.addString("A@b@c@failed.com");
		emails.addString("Abc.failed.com");
		emails.addString("a\"b(c)d,e:f;g<h>i[j\\k]l@failed.com"); // (none of the special characters in this local part are allowed outside quotation marks)
		emails.addString("just\"not\"right@failed.com"); // (quoted strings must be dot separated or the only element making up the local-part)
		emails.addString("this is\"not\allowed@failed.com"); // (spaces, quotes, and backslashes may only exist when within quoted strings and preceded by a backslash)
		emails.addString("this\\ still\\\"not\\allowed@failed.com"); // (even if escaped (preceded by a backslash), spaces, quotes, and backslashes must still be contained by quotes)
		emails.addString("john..doe@failed.com"); // (double dot before @)

		for (auto &it : emails.asArray()) {
			memory::string str(it.getString());
			if (!valid::validateEmail(str)) {
				stream << "Email (should fail): [invalid] " << str << "\n";
			} else {
				stream << "Email (should fail): [valid] " << it.getString() << " >> " << str << "\n";
				++ failed;
			}
		}

		_desc = stream.str();
		return failed == 0;
	}

} _ValidTest;

}

#endif
