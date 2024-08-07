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

#if MODULE_STAPPLER_CRYPTO && MODULE_STAPPLER_DATA

#include "SPCrypto.h"
#include "SPJsonWebToken.h"
#include "SPValid.h"
#include "Test.h"

namespace STAPPLER_VERSIONIZED stappler::app::test {

static constexpr auto PRIVATE_KEY =
R"PubKey(-----BEGIN RSA PRIVATE KEY-----
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
-----END RSA PRIVATE KEY-----
)PubKey";

// Открытый ключ ключ, использовается для защиты авторизации
static constexpr auto PUBLIC_KEY =
R"PubKey(-----BEGIN PUBLIC KEY-----
MIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEAvcL8IPbL16loGZNKBlXG
fJc8ffZjqlYjS3fwar/GSQbl8phRQXtMeqqJzn32IEjGYQOmxRaiP+eRVFiPES6b
doP01ajv2CUPY4R/56IBm0ID+Ui774Kn0ZB3XW9UWfZUfqH172JUlOTtZ396rYEO
VcoOrnL+9kyqfdGvgbaxEhx9RDDxd4xGHeRKQxbB48j992qHgeL2XPOyGlMuHLx8
hbWmSsVL4KT24Vo+0pqvkNQh04Lz154/4EDcuAMCPrwdsclQrN+mG9PcgySBpSKR
bB4Tpgb5QkufVOloFXFEfP49I8+8vF/D6nrfvRMEMr5jJxhoFyXjCdSwP+vHoNYe
0in5/TDNij2TptcIE0dKlxlr+Q2w/ywhZ711YKOURYEM6y/kBRxHoGBB6CmZLzRb
xAoehl1WqAb4GMUmU6HKLnopJEwbQA0jFg7wOQ81Pl5XyzCWsu0miiuDU169CnW9
rnuUieft8acxcmT1XjFZQlBpj12vYxjrzOFQPwS4Nr5iYFGKaCj9AqPfoX7+5e8A
7/wW5CRWXE/NPyIRS1yPKiUTihoc0WOcUgB1Puc/S4LLl9wDY5cEpaOAU+K563rx
h+re3Rq4cSH7RMj0sePdc202ZpzVptcdiYJEjgkutNCyFAS3ghHPthjA8EKlBE9p
zziPCKD41RKzWE1H+jDXx2MCAwEAAQ==
-----END PUBLIC KEY-----
)PubKey";

static constexpr auto OPENSSH_KEY =
"ssh-rsa AAAAB3NzaC1yc2EAAAADAQABAAABAQDcgSBj1YSdYIpjQ087Gr7e5z6Y7XmY6WbjTuIezvE8MbdGIk3+0ItUATaAdIXPXHX/+7kLULeOXyZxw/VaUGu1c3TcAa9romGK1ghiFSH3f0HuYZL2dwqrPhn9ZYT/3TgQVlTMKStEBJ4qpAWtHmNqnyCPDptsjkHgQP8UYDrcvbGR6mqWKEaKqVgC551/TPiRdtRG47zFEXJkvH7r4Qgj318b1qOP2wyZ+9AlCjyZABOvCPbapSg5OlppUh2rkhF6fQVLMJEYLIwbXa8g6Fu5wMiRqS1209nkpKmYXxVeIkZf1/I7CrppeXsnABIfoWfx6Hk34Dp9JV/p6Le9KPJJ sbkarr@sbkarr-virtual-machine";

struct JwtTest : Test {
	JwtTest() : Test("JwtTest") { }

	virtual bool run() override {
		crypto::PublicKey sshPk(BytesView((const uint8_t *)OPENSSH_KEY, strlen(OPENSSH_KEY)));
		crypto::PublicKey pub(BytesView((const uint8_t *)PUBLIC_KEY, strlen(PUBLIC_KEY)));
		crypto::PrivateKey priv(BytesView((const uint8_t *)PRIVATE_KEY, strlen(PRIVATE_KEY)));

		StringStream stream;
		size_t count = 0;
		size_t passed = 0;
		stream << "\n";

		BytesView secret((const uint8_t *)PRIVATE_KEY, strlen(PRIVATE_KEY));

		crypto::PrivateKey keyGost(crypto::Backend::OpenSSL);
		keyGost.generate(crypto::KeyType::GOST3410_2012_512);

		crypto::PrivateKey keyGost2(crypto::Backend::OpenSSL);
		keyGost2.generate(crypto::KeyType::GOST3410_2012_256);

		auto pubGost = keyGost.exportPublic();
		auto pubGost2 = keyGost2.exportPublic();

		runTest(stream, "jwt-ecdsa", count, passed, [&] () -> bool {
			crypto::PrivateKey keyEcdsa(crypto::Backend::OpenSSL);
			keyEcdsa.generate(crypto::KeyType::ECDSA);

			crypto::PrivateKey keyEcdsa2(crypto::Backend::OpenSSL);
			keyEcdsa2.generate(crypto::KeyType::EDDSA_ED448);

			JsonWebToken<Interface> token({
				pair("data", Value("data")),
				pair("int", Value(42)),
			});

			auto d1 = token.exportSigned(JsonWebToken<Interface>::ES256, keyEcdsa);
			auto d2 = token.exportSigned(keyEcdsa2);

			token.exportSigned(keyGost2);

			JsonWebToken<Interface> tmpToken1(d1);
			JsonWebToken<Interface> tmpToken2(d2);

			auto pubEcdsa = keyEcdsa.exportPublic();
			auto pubEcdsa2 = keyEcdsa2.exportPublic();

			if (!tmpToken1.validate(pubEcdsa)) {
				return false;
			}
			if (!tmpToken2.validate(pubEcdsa2)) {
				return false;
			}
			return true;
		});

		runTest(stream, "jwt-gost", count, passed, [&] () -> bool {
			JsonWebToken<Interface> token({
				pair("data", Value("data")),
				pair("int", Value(42)),
			});

			auto d1 = token.exportSigned(keyGost);
			auto d2 = token.exportSigned(keyGost2);
			token.exportSigned(JsonWebToken<Interface>::HS256, secret);
			token.exportSigned(JsonWebToken<Interface>::HS512, secret);

			token.exportSigned(keyGost2);

			JsonWebToken<Interface> tmpToken1(d1);
			JsonWebToken<Interface> tmpToken2(d2);

			if (!tmpToken1.validate(pubGost)) {
				return false;
			}
			if (!tmpToken2.validate(pubGost2)) {
				return false;
			}
			return true;
		});

		runTest(stream, "aes-gost", count, passed, [&] () -> bool {
			auto secret = string::Sha512::make(PUBLIC_KEY);

			do {
				AesToken<Interface>::Keys keys({
					&pubGost,
					&keyGost,
					BytesView(secret)
				});

				AesToken<Interface> tok = AesToken<Interface>::create(keys);

				tok.setString(StringView("String"), "string");
				tok.setInteger(42, "integer");

				auto fp = AesToken<Interface>::Fingerprint(crypto::HashFunction::GOST_3411, BytesView(secret));
				auto d = tok.exportData(fp);
				auto jwtTok = tok.exportToken("stappler.org", fp, 720_sec, "stappler.org");

				auto next = AesToken<Interface>::parse(d, fp, keys);
				auto jwtNext = AesToken<Interface>::parse(jwtTok, fp, "stappler.org", "stappler.org", keys);

				if (next.getData() != tok.getData()) {
					std::cout << data::EncodeFormat::Pretty << next.getData() << "\n" << tok.getData() << "\n";
					return false;
				}
				if (jwtNext.getData() != tok.getData()) {
					std::cout << data::EncodeFormat::Pretty << jwtNext.getData() << "\n" << tok.getData() << "\n";
					return false;
				}
			} while (0);

			do {
				AesToken<Interface>::Keys keys({
					&pubGost2,
					&keyGost2,
					BytesView(secret)
				});

				AesToken<Interface> tok = AesToken<Interface>::create(keys);

				tok.setString(StringView("String"), "string");
				tok.setInteger(42, "integer");

				auto fp = AesToken<Interface>::Fingerprint(crypto::HashFunction::GOST_3411, BytesView(secret));
				auto d = tok.exportData(fp);
				auto jwtTok = tok.exportToken("stappler.org", fp, 720_sec, "stappler.org");

				auto next = AesToken<Interface>::parse(d, fp, keys);
				auto jwtNext = AesToken<Interface>::parse(jwtTok, fp, "stappler.org", "stappler.org", keys);

				if (next.getData() != tok.getData()) {
					next = AesToken<Interface>::parse(d, fp, keys);
					std::cout << data::EncodeFormat::Pretty << next.getData() << "\n" << tok.getData() << "\n";
					return false;
				}
				if (jwtNext.getData() != tok.getData()) {
					jwtNext = AesToken<Interface>::parse(jwtTok, fp, "stappler.org", "stappler.org", keys);
					std::cout << data::EncodeFormat::Pretty << jwtNext.getData() << "\n" << tok.getData() << "\n";
					return false;
				}
			} while (0);

			return true;
		});
		runTest(stream, "pubkey", count, passed, [&] () -> bool {
			return sshPk.exportPem([] (BytesView) { }) && pub.exportPem([] (BytesView) { });
		});

		runTest(stream, "privkey", count, passed, [&] () -> bool {
			auto pubKey = priv.exportPublic();

			return pubKey.exportPem([&] (BytesView) { });
		});

		runTest(stream, "jwt", count, passed, [&] () -> bool {
			do {
				JsonWebToken<Interface> token({
					pair("data", Value("data")),
					pair("int", Value(42)),
				});

				auto d = token.exportSigned(JsonWebToken<Interface>::SigAlg::RS256, StringView(PRIVATE_KEY));
				auto d2 = token.exportSigned(JsonWebToken<Interface>::SigAlg::RS512, StringView(PRIVATE_KEY));

				JsonWebToken<Interface> tmpToken(d);
				JsonWebToken<Interface> tmpToken2(d2);

				if (!tmpToken.validate(StringView(PUBLIC_KEY))) {
					return false;
				}
				if (!tmpToken2.validate(StringView(PUBLIC_KEY))) {
					return false;
				}
			} while (0);

			do {
				JsonWebToken<Interface> token({
					pair("data", Value("data")),
					pair("int", Value(42)),
				});

				auto d1 = token.exportSigned(JsonWebToken<Interface>::SigAlg::HS256, secret);
				auto d2 = token.exportSigned(JsonWebToken<Interface>::SigAlg::HS512, secret);

				JsonWebToken<Interface> tmpToken1(d1);
				JsonWebToken<Interface> tmpToken2(d2);

				if (!tmpToken1.validate(secret)) {
					return false;
				}
				if (!tmpToken2.validate(secret)) {
					return false;
				}
			} while (0);

			return true;
		});

		runTest(stream, "aes", count, passed, [&] () -> bool {
			auto secret = string::Sha512::make(PUBLIC_KEY);

			AesToken<Interface>::Keys keys({
				&pub,
				&priv,
				BytesView(secret)
			});

			AesToken<Interface> tok = AesToken<Interface>::create(keys);

			tok.setString(StringView("String"), "string");
			tok.setInteger(42, "integer");

			auto fp = AesToken<Interface>::Fingerprint(crypto::HashFunction::GOST_3411, BytesView(secret));
			auto d = tok.exportData(fp);

			auto next = AesToken<Interface>::parse(d, fp, keys);

			if (next.getData() != tok.getData()) {
				std::cout << data::EncodeFormat::Pretty << next.getData() << "\n" << tok.getData() << "\n";
				return false;
			}

			tok.exportToken("stappler.org", fp, TimeInterval::seconds(720), "stappler.org");

			auto fp2 = AesToken<Interface>::Fingerprint(crypto::HashFunction::GOST_3411,
					[&] (const Callback<bool(const CoderSource &)> &cb) {
				cb(BytesView(secret));
			});
			tok.exportData(fp2);

			auto fp3 = AesToken<Interface>::Fingerprint(crypto::HashFunction::GOST_3411, nullptr);
			tok.exportData(fp3);

			auto fp4 = AesToken<Interface>::Fingerprint(crypto::HashFunction::SHA_2,
					[&] (const Callback<bool(const CoderSource &)> &cb) {
				cb(BytesView(secret));
			});
			tok.exportData(fp4);

			auto fp5 = AesToken<Interface>::Fingerprint(crypto::HashFunction::SHA_2, nullptr);
			tok.exportData(fp5);

			auto fp6 = AesToken<Interface>::Fingerprint(crypto::HashFunction::SHA_2, BytesView(secret));
			tok.exportData(fp6);

			return tok ? true : false;
		});

		_desc = stream.str();
		return count == passed;
	}
} _JwtTest;

}

#endif
