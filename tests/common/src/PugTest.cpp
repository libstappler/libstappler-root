/**
 Copyright (c) 2024 Stappler LLC <admin@stappler.dev>

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
#include "SPPugCache.h"
#include "SPPugContext.h"

#if LINUX
#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <string.h>
#endif

namespace STAPPLER_VERSIONIZED stappler::app::test {

static StringView s_templateData1(
R"(
mixin include1(value1,value2=true)
	.include1
		h Include 1
		p=value1
		|
		|
		p=value2
	.attrs(id="id" class='class1,class2')
	a(class='button', href='//google.com') Google
	- var authenticated = true
	p(class=authenticated ? 'authed' : 'anon')
	- authenticated = false
	p(class=authenticated ? 'authed' : 'anon')
	input(
		type='checkbox'
		name='agreement'
		checked
	)
	- var url = 'pug-test.html';
	a(href='/' + url) Link
	|
	|
	- url = 'https://example.com/'
	a(href!=url) Another link
	div(escaped="<code>")
	div(unescaped!="<code>")
	input(type='checkbox' checked)
	|
	|
	input(type='checkbox' checked=true)
	|
	|
	input(type='checkbox' checked=false)
	|
	|
	input#input(type='checkbox' checked=true && 'checked')
	|
	|
	a.button(style={color: 'red', background: 'green'})
	|
	|
	- var classes = ['foo', 'bar', 'baz']
	a(class=classes)
	|
	|
	//- the class attribute may also be repeated to merge arrays
	a.bang(class=classes class=['bing'])
	|
	|
	div#foo("data\-bar"="foo")&attributes({'data-foo': 'bar'})
	|
	|
	- var attributes = {};
	- attributes.class = 'baz';
	div#foo('data\-bar'="foo")&attributes(attributes)
	.
		dottest
)");

static StringView s_templateData2(
R"(
mixin include2(value1,value2=false)
	.include2
		h Include 2
		p=value1
		|
		|
		p=value2
	-
		var list = ["Uno", "Dos", "Tres",
			"Cuatro", "Cinco", "Seis"]
	each item in list
		li= item
	p
		= 'This code is <escaped>!'
	p= 'This code is' + ' <escaped>!'
	p(style="background: blue")= 'A message with a ' + 'blue' + ' background'
	p
		!= 'This code is <strong>not</strong> escaped!'
	p!= 'This code is' + ' <strong>not</strong> escaped!'
	customtag/
	<table>
	</table>
	- var title = "On Dogs: Man's Best Friend";
	- var author = "enlore";
	- var theGreat = "<span>escape!</span>";
	h1= title
	p Written with\ love by #{author} \#{author}
	p This will be safe: #{theGreat}
	- var riskyBusiness = "<em>Some of the girls are wearing my mother's clothing.</em>";
	.quote
		p Joel: !{riskyBusiness}
	- var friends = 10
	case friends
		when 0
			p you have no friends
		when 1
		when 2
			p you have a friend
		default
			p you have #{friends} friends
	case friends
		when 0: p you have no friends
		when 10: p you have a 10 friend
		default: p you have #{friends} friends
	- var user = {description: 'foo bar baz',name:'Username'}
	- var authorised = false
	#user
		if user.description
			h2.green Description
			p.description= user.description
			unless user.isAnonymous
				p You're logged in as #{user.name}
		else if authorised
			h2.blue Description
			p.description.
				User has no description,
				why not add one...
		else
			h2.red Description
			p.description User has no description
	- var n = 0;
	ul
		while n < 4
			li= n++
	ul
		each val, key in {1: 'one', 2: 'two', 3: 'three'}
			li= key + ': ' + val
	- var values = [];
	ul
		each val in values
			li= val
		else
			li There are no values
	each val in authorised
		p= val
)");

static StringView s_templateData3(
R"(
html
	<p>
	div
		p
			include HelloWorld
		- var friends = 1
		case friends
			when 0
				p you have no friends
			when 1
			when 2
				p you have a friend
			default
				p you have #{friends} friends
	</p>
)");

static StringView s_templateData4(
R"(
p.
	Hello world
)");

static StringView s_cssData(
R"(html {
	font-size: 18px;
}

body {
	background: #aaa;
	margin: 0;
	padding: 0;
}
)");

static StringView s_err1(
R"(
.test1
	.test2
    .test3
)");

static StringView s_err2(
R"(
.test1
  .test2
    .test3
        .test4
)");

struct PugTest : MemPoolTest {
	PugTest() : MemPoolTest("PugTest") { }

	virtual void onError(StringView err) {
		//std::cout << "Template error:\n" << err << "\n";
	}

	virtual void updateNotify(int notify) {
#if LINUX
		static bool regenerate = false;
		char buf[4096] __attribute__ ((aligned(__alignof__(struct inotify_event))));
		const struct inotify_event *event;
		ssize_t len;

		for (;;) {
			len = ::read(notify, buf, sizeof(buf));
			if (len == -1 && errno != EAGAIN) {
				perror("read");
				exit(EXIT_FAILURE);
			}

			if (len <= 0)
				break;

			for (char *ptr = buf; ptr < buf + len; ptr += sizeof(struct inotify_event) + event->len) {
				event = (const struct inotify_event*) ptr;
				if (event->mask & IN_CLOSE_WRITE) {
					_cache->update(event->wd, regenerate);
					regenerate = !regenerate;
				}
			}
		}
#endif
	}

	virtual bool run(pool_t *p) override {
		StringStream stream;
		size_t count = 0;
		size_t passed = 0;
		stream << "\n";

		_cache = new (p) pug::Cache(pug::Template::Options::getPretty(), [&] (StringView err) {
			onError(err);
		});

		auto testPath = filesystem::currentDir<Interface>("resources/test.pug");
		auto test2Path = filesystem::currentDir<Interface>("resources/test2.pug");
		auto test3Path = filesystem::currentDir<Interface>("resources/test3.pug");
		auto cssPath = filesystem::currentDir<Interface>("resources/test.css");

		auto opts = pug::Template::Options::getPretty();
		if (!opts.hasFlag(pug::Template::Options::StopOnError)) {
			opts.clearFlags({pug::Template::Options::StopOnError});
			opts.setFlags({pug::Template::Options::StopOnError});
		}

		_cache->addFile(test3Path);
		_cache->addFile(testPath);
		_cache->addFile(testPath);
		_cache->addFile(cssPath);

		_cache->addContent(StringView("include.css"), s_cssData.str<mem_pool::Interface>());
		_cache->addTemplate(StringView("include1.pug"), s_templateData1.str<mem_pool::Interface>());
		_cache->addTemplate(StringView("include2.pug"), s_templateData2.str<mem_pool::Interface>(), opts);

		_cache->addTemplate(StringView("err1.pug"), s_err1.str<mem_pool::Interface>());
		_cache->addTemplate(StringView("err2.pug"), s_err2.str<mem_pool::Interface>());

		if (_cache->isNotifyAvailable()) {
			auto testData = filesystem::readIntoMemory<Interface>(testPath);
			testData.emplace_back('\n');

			auto cssData = filesystem::readIntoMemory<Interface>(cssPath);
			cssData.emplace_back('\n');

			filesystem::write(testPath, testData);
			filesystem::write(cssPath, cssData);

			updateNotify(_cache->getNotify());

			testData.pop_back();
			filesystem::write(testPath, testData);

			cssData.pop_back();
			filesystem::write(cssPath, cssData);
		}

		filesystem::touch(testPath);
		filesystem::touch(cssPath);

		_cache->update(p, true);
		_cache->regenerate(cssPath);

		runTest(stream, "Expression test", count, passed, [&] {
			auto testExpr = [] (StringView str) {
				pug::Expression::Options opts = pug::Expression::Options::getDefaultScript();
				opts.clearFlags({pug::Expression::StopOnRootSequence});
				auto cexpr = pug::Expression::parse(str, opts);
				cexpr->describe([] (StringView) { }, 0);
			};

			testExpr("a --");
			testExpr("a # b");
			testExpr("a::b");
			testExpr("a::b");
			testExpr("a[b]");
			testExpr("a(b)");
			testExpr("a{b}");
			testExpr("++ a");
			testExpr("-- a");
			testExpr("- a");
			testExpr("!a");
			testExpr("~a");
			testExpr("a * b");
			testExpr("a / b");
			testExpr("a % b");
			testExpr("a - b");
			testExpr("a << b");
			testExpr("a >> b");
			testExpr("a < b");
			testExpr("a <= b");
			testExpr("a > b");
			testExpr("a >= b");
			testExpr("a == b");
			testExpr("a != b");
			testExpr("a & b");
			testExpr("a | b");
			testExpr("a ^ b");
			testExpr("a || b");
			testExpr("a += b");
			testExpr("a -= b");
			testExpr("a *= b");
			testExpr("a /= b");
			testExpr("a %= b");
			testExpr("a <<= b");
			testExpr("a >>= b");
			testExpr("a &= b");
			testExpr("a |= b");
			testExpr("a ^= b");
			testExpr("a ; b");

			do {
				pug::Expression::Options opts = pug::Expression::Options::getDefaultInline();
				StringView testRem("i + j + k % 32");
				auto cexpr = pug::Expression::parse(testRem, opts);
				cexpr->describe([] (StringView) { }, 0);
			} while (0);

			pug::Context ctx;
			pug::Value value({
				pair("key", pug::Value("value")),
				pair("double", pug::Value(0.5f))
			});

			pug::Value array({
				Value("Array Value")
			});

			pug::Expression noexpr;
			if (noexpr.empty()) {
				noexpr.describe([] (StringView) { }, 0);
			}

			pug::Expression noexpr2(pug::Expression::NoOp, nullptr, nullptr, Value(value));
			if (noexpr2.empty()) {
				return false;
			}

			pug::Expression noexpr3(pug::Expression::NoOp, nullptr, nullptr, StringView("token"));
			if (noexpr3.empty()) {
				return false;
			}

			ctx.set("value", value, nullptr);
			ctx.set("array", array, nullptr);

			ctx.set("fn", [] (pug::VarStorage &, pug::Var *var, size_t argc) -> pug::Var {
				return pug::Var(Value("World"));
			});

			pug::Expression::Options opts = pug::Expression::Options::getDefaultScript();
			opts.clearFlags({pug::Expression::StopOnRootComma});
			opts.disableAllOperators();
			opts.enableAllOperators();
			opts.disableOperators({pug::Expression::Op::Colon});
			opts.enableOperators({pug::Expression::Op::Colon});

			StringView expr("'Hello\\u0020', fn(), ' ', 1.0e-10, ' ', inf, ' ', nan, ' ', ~256 ' ', +256");
			auto cexpr = pug::Expression::parse(expr, opts);

			ctx.print(*cexpr, [] (StringView str) {
				std::cout << str;
			}, false);

			std::cout << "\n";

			return true;
		});

		runTest(stream, "Context test", count, passed, [&] {
			StringStream out;
			pug::VarClass varClass;
			varClass.staticFunctions.emplace(pug::String("static"),
					[] (pug::VarStorage &, pug::Var *var, size_t argc) -> pug::Var {
				return pug::Var(pug::Value(toString("Static: ", argc)));
			});
			varClass.functions.emplace(pug::String("fn"),
					[] (pug::VarStorage &ctx, pug::Var *var, size_t argc) -> pug::Var {
				auto str = data::toString<Interface>(ctx.readValue());
				return pug::Var(pug::Value(toString("Function: ", argc, " ", str)));
			});

			_cache->runTemplate(test3Path, [&] (pug::Context &ctx, const pug::Template &) {
				pug::Value value({
					pair("key", pug::Value("value")),
					pair("double", pug::Value(0.5f))
				});

				pug::Value array({
					Value("Array Value")
				});

				ctx.set("value", value, nullptr);
				ctx.set("array", array, nullptr);

				ctx.set("Class", &varClass);
				ctx.set("object", value, &varClass);

				return true;
			}, [&] (StringView str) { out << str; }, pug::Template::Options::getDefault());

			std::cout << out.str() << "\n";

			return true;
		});

		runTest(stream, "Standalone", count, passed, [&] {
			auto opts = pug::Template::Options::getDefault();
			auto tpl = pug::Template::read(s_templateData3, opts, [] (StringView err) {});

			pug::Context exec;
			exec.loadDefaults();
			exec.setIncludeCallback([&] (const StringView &path, pug::Context &exec, const pug::Context::OutStream &out, pug::Template::RunContext &tpl) -> bool {
				auto incl = pug::Template::read(s_templateData4, opts, [] (StringView err) {});
				incl->run(exec, out, tpl);
				return true;
			});

			StringStream out;
			tpl->run(exec, [&] (StringView str) {
				out << str;
			});

			return !out.str().empty();
		});

		runTest(stream, "Content test", count, passed, [&] {
			auto content = _cache->get(cssPath);
			if (content && content->getContent() == BytesView(filesystem::readIntoMemory<Interface>(cssPath)).toStringView()) {
				return true;
			}
			return false;
		});

		runTest(stream, "Describe test", count, passed, [&] {
			StringStream out;
			auto include1 = _cache->get("include1.pug");
			auto include2 = _cache->get("include2.pug");
			auto tpl = _cache->get(testPath);

			include1->getTemplate()->describe([&] (StringView str) { out << str; }, true);
			include2->getTemplate()->describe([&] (StringView str) { out << str; }, true);
			tpl->getTemplate()->describe([&] (StringView str) { out << str; }, true);
			return !out.str().empty();
		});

		runTest(stream, "Lexer test", count, passed, [&] {
			std::cout << "Test1\n";
			_cache->runTemplate(testPath, [] (pug::Context &, const pug::Template &) {
				return true;
			}, [&] (StringView str) {
				std::cout << str;
			});
			std::cout << "\n";
			return true;
		});

		runTest(stream, "Pug test", count, passed, [&] {
			std::cout << "Test2\n";
			_cache->runTemplate(test2Path, [] (pug::Context &, const pug::Template &) {
				return true;
			}, [&] (StringView str) { });
			_cache->drop(test2Path);
			_cache->runTemplate(test2Path, [] (pug::Context &, const pug::Template &) {
				return true;
			}, [&] (StringView str) { }, pug::Template::Options::getDefault());
			std::cout << "\n";
			return true;
		});

		_desc = stream.str();

		delete _cache;
		_cache = nullptr;

		return count == passed;
	}

protected:
	pug::Cache *_cache = nullptr;
} _PugTest;

}
