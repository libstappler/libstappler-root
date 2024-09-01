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
#include "SPString.h"
#include "Test.h"

#if MODULE_STAPPLER_SEARCH

#include "SPSearchConfiguration.h"
#include "SPSearchDistance.h"
#include "SPSearchIndex.h"
#include "SPUrl.h"

namespace STAPPLER_VERSIONIZED stappler::app::test {

static constexpr auto SearchParserTest =
R"( 7'/10,5' 2011 шт. 20 лестница крыши 7\' 2011 шт. 20 X - арматурный прут 2011 шт.7 шт. 10)";

static constexpr auto SearchParserTest2 =
R"( 7'/10,5' 2011 шт. 20 лестница крыши 7\' 2011 шт. 20 X - арматурный прут 2011 шт.7 шт. 10
1.0.0 2.3.4 word
122:123:123:1234 word
122-123-123:1234: word
122-123:123:1234: word
122-123-123:1234-124:124  word
12:123:123:1234 word
12-123-123:1234: word
12-123:123:1234: word
12-123-123:1234-124:124  word
12-123-123:1234-124: word
12–123-123:1234: word
12–123:123:1234: word
12–123-123:1234-124:124  word
12–123-123:1234-124: word
12@ / 12: ntcn 12 123:123:123-1234-124:124/1234
123:123:123:1234:124:124
test-123-word test/123/word тест-123-слово тест/123/слово/ тест/
test-123-word test/123/word тест-123-слово тест/123/слово/ тест/
-124.567. -124.456@ -124.456№ -124.567
-124.125e125 124:123:234 -124e125
/usr/local/bin/bljad
)";

auto s_lipsum1 = StringView("Lorem ipsum dolor sit amet, consectetur adipiscing elit. "
		"Mauris dapibus, ex nec varius euismod, mi orci cursus sapien, "
		"sed pharetra lorem lorem non urna. Lorem ipsum dolor sit amet, "
		"consectetur adipiscing elit. Curabitur erat ex, consectetur "
		"consequat porta sed, maximus id sem. Nulla at mi augue. Donec "
		"sapien felis, bibendum a sem in, elementum tincidunt ipsum. "
		"Phasellus eget dapibus metus, vitae dignissim nisl. Integer "
		"mauris est, sagittis ac quam vitae, efficitur interdum velit. "
		"Fusce ut velit at elit rhoncus pharetra vitae sit amet elit. "
		"Lorem ipsum dolor sit amet, consectetur adipiscing elit. "
		"Mauris dapibus, ex nec varius euismod, mi orci cursus sapien, "
		"sed pharetra lorem lorem non urna. Lorem ipsum dolor sit amet, "
		"consectetur adipiscing elit. Curabitur erat ex, consectetur "
		"consequat porta sed, maximus id sem. Nulla at mi augue. Donec "
		"sapien felis, bibendum a sem in, elementum tincidunt ipsum. "
		"Phasellus eget dapibus metus, vitae dignissim nisl. Integer "
		"mauris est, sagittis ac quam vitae, efficitur interdum velit. "
		"Fusce ut velit at elit rhoncus pharetra vitae sit amet elit. "
		"Lorem ipsum dolor sit amet, consectetur adipiscing elit. "
		"Mauris dapibus, ex nec varius euismod, mi orci cursus sapien, "
		"sed pharetra lorem lorem non urna. Lorem ipsum dolor sit amet, "
		"consectetur adipiscing elit. Curabitur erat ex, consectetur "
		"consequat porta sed, maximus id sem. Nulla at mi augue. Donec "
		"sapien felis, bibendum a sem in, elementum tincidunt ipsum. "
		"Phasellus eget dapibus metus, vitae dignissim nisl. Integer "
		"mauris est, sagittis ac quam vitae, efficitur interdum velit. "
		"Fusce ut velit at elit rhoncus pharetra vitae sit amet elit. "
		"Lorem ipsum dolor sit amet, consectetur adipiscing elit. "
		"Mauris dapibus, ex nec varius euismod, mi orci cursus sapien, "
		"sed pharetra lorem lorem non urna. Lorem ipsum dolor sit amet, "
		"consectetur adipiscing elit. Curabitur erat ex, consectetur "
		"consequat porta sed, maximus id sem. Nulla at mi augue. Donec "
		"sapien felis, bibendum a sem in, elementum tincidunt ipsum. "
		"Phasellus eget dapibus metus, vitae dignissim nisl. Integer "
		"mauris est, sagittis ac quam vitae, efficitur interdum velit. "
		"Fusce ut velit at elit rhoncus pharetra vitae sit amet elit pharetra vitae sit amet elit. ");

auto s_lipsum2 = StringView("Lorem ipsum dolor sit amet, consectetur adipiscing elit. "
		"Muris dapibus, ex nec varius euismod, mi orci cursus sapien, "
		"sed pharetra lorem lorem non urna. Lorem ium dolor sit amet, "
		"consectetur adipiscing elit. Curabitur erat ex, consectetur "
		"conseqguat porta sed, maimus id sem. Nulla at mhi augue. Donec "
		"sapien felis, bibendum a sem in, elementum tincidunt ipsum. "
		"Phasellus eget dapdibus metus, vitae dignissim nisl. Integer "
		"mauris est, sagittis ac quam vitae, efficitur ierdum velit. "
		"Fusce ut velit at elit rhofgg phavxretra vitae sit amet elit adf aa4ea32 zxxcv. "
		"Lorem ipsum dolor sit amet, consectetur adipiscing elit. "
		"Muris dapibus, ex nec varius euismod, mi orci cursus sapien, "
		"sed pharetra lorem lorem non urna. Lorem ium dolor sit amet, "
		"consectetur adipiscing elit. Curabitur erat ex, consectetur "
		"conseqguat porta sed, maimus id sem. Nulla at mhi augue. Donec "
		"sapien felis, bibendum a sem in, elementum tincidunt ipsum. "
		"Phasellus eget dapdibus metus, vitae dignissim nisl. Integer "
		"mauris est, sagittis ac quam vitae, efficitur ierdum velit. "
		"Fusce ut velit at elit rhofgg phavxretra vitae sit amet elit adf aa4ea32 zxxcv. "
		"Lorem ipsum dolor sit amet, consectetur adipiscing elit. "
		"Muris dapibus, ex nec varius euismod, mi orci cursus sapien, "
		"sed pharetra lorem lorem non urna. Lorem ium dolor sit amet, "
		"consectetur adipiscing elit. Curabitur erat ex, consectetur "
		"conseqguat porta sed, maimus id sem. Nulla at mhi augue. Donec "
		"sapien felis, bibendum a sem in, elementum tincidunt ipsum. "
		"Phasellus eget dapdibus metus, vitae dignissim nisl. Integer "
		"mauris est, sagittis ac quam vitae, efficitur ierdum velit. "
		"Fusce ut velit at elit rhofgg phavxretra vitae sit amet elit adf aa4ea32 zxxcv. "
		"Lorem ipsum dolor sit amet, consectetur adipiscing elit. "
		"Muris dapibus, ex nec varius euismod, mi orci cursus sapien, "
		"sed pharetra lorem lorem non urna. Lorem ium dolor sit amet, "
		"consectetur adipiscing elit. Curabitur erat ex, consectetur "
		"conseqguat porta sed, maimus id sem. Nulla at mhi augue. Donec "
		"sapien felis, bibendum a sem in, elementum tincidunt ipsum. "
		"Phasellus eget dapdibus metus, vitae dignissim nisl. Integer "
		"mauris est, sagittis ac quam vitae, efficitur ierdum velit. "
		"Fusce ut velit at elit rhofgg phavxretra vitae sit amet elit adf aa4ea32 zxxcv. ");

auto s_lipsum3 = StringView("Lorem ipsum dolor sit amet, consectetur adipiscing elit. "
		"Mauris dapibus, ex nec varius euismod, mi orci cursus sapien, "
		"sed pharetra lorem lorem non urna. Lorem ipsum dolor sit amet, "
		"consectetur adipiscing elit. Curabitur erat ex, consectetur "
		"consequat porta sed, maximus id sem. Nulla at mi augue. Donec "
		"sapien felis, bibendum a sem in, elementum tincidunt ipsum. "
		"Phasellus eget dapibus metus, vitae dignissim nisl. Integer "
		"mauris est, sagittis ac quam vitae, efficitur interdum velit. "
		"Fusce ut velit at elit rhoncus pharetra vitae sit amet elit. ");

auto s_lipsum4 = StringView("Lorem ipsum dolor sit amet, consectetur adipiscing elit. "
		"Muris dapibus, ex nec varius euismod, mi orci cursus sapien, "
		"sed pharetra lorem lorem non urna. Lorem ium dolor sit amet, "
		"consectetur adipiscing elit. Curabitur erat ex, consectetur "
		"conseqguat porta sed, maimus id sem. Nulla at mhi augue. Donec "
		"sapien felis, bibendum a sem in, elementum tincidunt ipsum. "
		"Phasellus eget dapdibus metus, vitae dignissim nisl. Integer "
		"mauris est, sagittis ac quam vitae, efficitur ierdum velit. "
		"Fusce ut velit at elit rhofgg phavxretra vitae sit amet elit adf aa4ea32 zxxcv. ");

auto s_html = StringView(R"(<figure>
<a href="/api/v1/pages/id8030/images/id70550/content" data-fancybox="gallery" data-caption="Что влияет на наше восприятие данных"><img align="middle" type="image" src="/api/v1/pages/id8030/images/id70550/content" alt="Что влияет на наше восприятие данных" title="Что влияет на наше восприятие данных"/></a>
<figcaption data-type="image" data-src="/api/v1/pages/id8030/images/id70550/content">Что влияет на наше восприятие 70550 данных</figcaption>
</figure>

<h4 id="влияниекультурныхнорм" data-hash="r3CxrYbBrOE" data-index="1"><a class="tb-header-link" href="#влияниекультурныхнорм"><i class="fa fa-link"></i></a>Влияние культурных норм</h4>

<p data-hash="Ug5WDp4Aj84" data-index="2">Под культурными нормами мы понимаем разделяемые группой людей ценности и убеждения. Говоря очень коротко, это представления о том, что такое хорошо и что такое плохо, разделяемые большинством. Можно или нет списывать на уроках и экзаменах, стоит ли откладывать средства на черный день, могут ли женщины управлять государством, должно ли государство помогать бедным… Если решение, которое надо принять человеку, лежит в той же сфере, что и разделяемые им культурные ценности, то человек зачастую принимает решение, исходя из этих ценностей, не подвергая их мыслительному анализу.</p>

<p data-hash="gDh6lK_Htjg" data-index="12">На одинаковые переживания люди test-word реагируют похожим образом. И эта реакция также предсказуемо влияет на принимаемые решения. Подробнее об этом — в <a href="/lectures/7946">подразделе «Эвристика аффекта»</a>.</p>

<h4 id="влияютлиличныезаблуждения" data-hash="iPtCs0RcF28" data-index="13"><a class="tb-header-link" href="#влияютлиличныезаблуждения"><i class="fa fa-link"></i></a>Влияют ли личные заблуждения?</h4>

<p data-hash="NMDALxUnpUU" data-index="14">Влияют ли на принимаемые<sup>1</sup> решения личные заблуждения человека? Конечно, влияют. слово-через-дефиз Нужно ли их принимать во внимание при изучении того, как люди принимают решения? Если мы изучаем какого-то конкретного индивида, то да, это очень важный фактор. Но на уровне общности людей изучать разнообразные личные заблуждения не получается: как шутят психологи, у каждого психа своя программа.</p>

<div class="citations">
<hr>
<h6 class="citations_header">Список источников</h6>
<ol>

<li id="cn_1">
<p>Buggle, Johannes and Durante, Ruben, Climate Risk, Cooperation, and the Co-Evolution of Culture and Institutions (October 2017). CEPR Discussion Paper No. DP12380. Available at SSRN: https://ssrn.com/abstract=3057294. <a href="#cnref_1" title="Вернуться" class="reversecitation">&nbsp;↩</a></p>
</li>

</ol>
</div>
)");

static bool test1() {
    int queryLength = 4;
    int targetLength = 4;
    char query[4] = {0,1,2,3};
    char target[4] = {0,1,2,3};

    StringView in1, in2;
    in1.set(query, queryLength);
    in2.set(target, targetLength);

    return !search::Distance(in1, in2).empty();
}

static bool test2() {
    int queryLength = 5;
    int targetLength = 9;
    char query[5] = {0,1,2,3,4}; // "match"
    char target[9] = {8,5,0,1,3,4,6,7,5}; // "remachine"

    StringView in1, in2;
    in1.set(query, queryLength);
    in2.set(target, targetLength);

    return !search::Distance(in1, in2).empty();
}

static bool test3() {
    int queryLength = 5;
    int targetLength = 9;
    char query[5] = {0,1,2,3,4};
    char target[9] = {1,2,0,1,2,3,4,5,4};

    StringView in1, in2;
    in1.set(query, queryLength);
    in2.set(target, targetLength);

    return !search::Distance(in1, in2).empty();
}

static bool test4() {
    int queryLength = 200;
    int targetLength = 200;
    char query[200] = {0};
    char target[200] = {1};

    StringView in1, in2;
    in1.set(query, queryLength);
    in2.set(target, targetLength);

    return !search::Distance(in1, in2).empty();
}

static bool test5() {
    int queryLength = 64; // Testing for special case when queryLength == word size
    int targetLength = 64;
    char query[64] = {0};
    char target[64] = {1};

    StringView in1, in2;
    in1.set(query, queryLength);
    in2.set(target, targetLength);

    return !search::Distance(in1, in2).empty();
}

static bool test6() {
    int queryLength = 13; // Testing for special case when queryLength == word size
    int targetLength = 420;
    char query[13] = {1,3,0,1,1,1,3,0,1,3,1,3,3};
    char target[420] = {0,1,1,1,0,1,3,0,1,3,3,3,1,3,2,2,3,2,3,3,1,0,1,1,1,0,1,3,0,1,3,
                        3,3,1,3,2,2,3,2,3,3,1,0,1,1,1,0,1,3,0,1,3,3,3,1,3,2,2,3,2,3,3,
                        1,0,1,1,1,0,1,3,0,1,3,3,3,1,3,2,2,3,2,3,3,1,0,1,1,1,0,1,3,0,1,
                        3,3,3,1,3,2,2,3,2,3,3,1,0,1,1,1,0,1,3,0,1,3,3,3,1,3,2,2,3,2,3,
                        3,1,0,1,1,1,0,1,3,0,1,3,3,3,1,3,2,2,3,2,3,3,1,0,1,1,1,0,1,3,0,
                        1,3,3,3,1,3,2,2,3,2,3,3,1,0,1,1,1,0,1,3,0,1,3,3,3,1,3,2,2,3,2,
                        3,3,1,0,1,1,1,0,1,3,0,1,3,3,3,1,3,2,2,3,2,3,3,1,0,1,1,1,0,1,3,
                        0,1,3,3,3,1,3,2,2,3,2,3,3,1,0,1,1,1,0,1,3,0,1,3,3,3,1,3,2,2,3,
                        2,3,3,1,0,1,1,1,0,1,3,0,1,3,3,3,1,3,2,2,3,2,3,3,1,0,1,1,1,0,1,
                        3,0,1,3,3,3,1,3,2,2,3,2,3,3,1,0,1,1,1,0,1,3,0,1,3,3,3,1,3,2,2,
                        3,2,3,3,1,0,1,1,1,0,1,3,0,1,3,3,3,1,3,2,2,3,2,3,3,1,0,1,1,1,0,
                        1,3,0,1,3,3,3,1,3,2,2,3,2,3,3,1,0,1,1,1,0,1,3,0,1,3,3,3,1,3,2,
                        2,3,2,3,3,1,0,1,1,1,0,1,3,0,1,3,3,3,1,3,2,2,3,2,3,3,1,0,1,1,1,
                        0,1,3,0,1,3,3,3,1,3,2,2,3,2,3,3,1};

    StringView in1, in2;
    in1.set(query, queryLength);
    in2.set(target, targetLength);

    return !search::Distance(in1, in2).empty();
}

static bool test7() {
    int queryLength = 3;
    int targetLength = 5;
    char query[3] = {2,3,0};
    char target[5] = {0,1,2,2,0};

    StringView in1, in2;
    in1.set(query, queryLength);
    in2.set(target, targetLength);

    return !search::Distance(in1, in2).empty();
}

static bool test8() {
    int queryLength = 3;
    int targetLength = 3;
    char query[3] = {2,3,0};
    char target[3] = {2,2,0};

    StringView in1, in2;
    in1.set(query, queryLength);
    in2.set(target, targetLength);

    return !search::Distance(in1, in2).empty();
}

static bool test9() {
    int queryLength = 64;
    int targetLength = 393;
    char query[64] = {9,5,5,9,9,4,6,0,1,1,5,4,6,0,6,5,5,6,5,2,2,0,6,0,8,3,7,0,6,6,4,8,3,1,9,4,5,5,5,7,8,2,
                      3,6,4,1,1,2,7,7,6,0,9,2,0,9,6,9,9,4,6,5,2,9};
    char target[393] = {7,1,6,2,9,1,1,7,5,5,4,9,6,7,3,4,6,9,4,5,2,6,6,0,7,8,4,3,3,9,5,2,0,1,7,1,4,0,9,9,7,
                        5,0,6,2,4,0,9,3,6,6,7,4,3,9,3,3,4,7,8,5,4,1,7,7,0,9,3,0,8,4,0,3,4,6,7,0,8,6,6,6,5,
                        5,2,0,5,5,3,1,4,1,6,8,4,3,7,6,2,0,9,0,4,9,5,1,5,3,1,3,1,9,9,6,5,1,8,0,6,1,1,1,5,9,
                        1,1,2,1,8,5,1,7,7,8,6,5,9,1,0,2,4,1,2,5,0,9,6,8,1,4,2,4,5,9,3,9,0,5,0,8,0,3,7,0,1,
                        3,5,0,6,5,5,2,8,9,7,0,8,5,1,9,0,3,3,7,2,6,6,4,3,8,5,6,2,2,6,5,8,3,8,4,0,3,7,8,2,6,
                        9,0,2,0,1,2,5,6,1,9,4,8,3,7,8,8,5,2,3,1,8,1,6,6,7,6,9,6,5,3,3,6,5,7,8,6,1,3,4,2,4,
                        0,0,7,7,1,8,5,3,3,6,1,4,5,7,3,1,8,0,8,1,5,6,6,2,4,4,3,9,8,7,3,8,0,3,8,1,3,3,4,6,1,
                        8,2,6,7,5,8,6,7,8,7,4,5,6,6,9,0,1,1,1,9,4,9,1,9,9,2,2,4,8,0,6,6,4,4,4,2,2,2,9,3,1,
                        6,8,7,2,9,8,6,0,1,7,7,2,8,6,2,2,1,6,0,3,4,9,8,9,3,2,3,5,3,6,6,9,6,6,2,6,6,0,8,7,9,
                        5,9,7,4,3,1,7,2,1,0,6,0,0,7,5,2,1,2,6,9,1,5,6,7};

    StringView in1, in2;
    in1.set(query, queryLength);
    in2.set(target, targetLength);

    return !search::Distance(in1, in2).empty();
}

static bool test10() {
    int queryLength = 3;
    int targetLength = 3;
    char query[3] = {0,1,2};
    char target[3] = {1,1,1};

    StringView in1, in2;
    in1.set(query, queryLength);
    in2.set(target, targetLength);

    return !search::Distance(in1, in2).empty();
}

// Check if edlib works for whole range of char values.
static bool test11() {
    int queryLength = 8;
    int targetLength = 8;
    // NOTE(Martin): I am using CHAR_MIN and CHAR_MAX because 'char' type is not guaranteed to be
    //   signed or unsigned by compiler, we can't know if it is signed or unsigned.
    char query[8] =  {CHAR_MIN, CHAR_MIN + (CHAR_MAX - CHAR_MIN) / 2, CHAR_MAX};
    char target[8] = {CHAR_MIN, CHAR_MIN + (CHAR_MAX - CHAR_MIN) / 2 + 1, CHAR_MAX};

    StringView in1, in2;
    in1.set(query, queryLength);
    in2.set(target, targetLength);

    return !search::Distance(in1, in2).empty();
}

struct SearchTest : MemPoolTest {
	SearchTest() : MemPoolTest("SearchTest") { }

	virtual bool run(pool_t *pool) {
		StringStream stream;
		size_t count = 0;
		size_t passed = 0;
		stream << "\n";

		runTest(stream, "Search index", count, passed, [&] {
			search::Configuration cfg(search::Language::Russian);
			search::SearchIndex index;
			index.init([&cfg] (const StringView &str, const search::SearchIndex::TokenCallback &tcb, search::SearchIndex::TokenType) {
				cfg.stemPhrase(str, [&] (StringView, StringView stem, search::ParserToken tok) {
					tcb(stem);
				});
			});

			index.add(s_lipsum1, 1, 123);
			index.add(s_lipsum3, 2, 123);
			index.add(SearchParserTest, 3, 456);
			index.add(SearchParserTest2, 4, 456);

			index.reserve(100);
			index.print([] (const StringView &) { });

			auto res1 = index.performSearch("ipsum", 3);
			for (auto &it : res1.nodes) {
				for (auto &m : it.matches) {
					index.resolveToken(*it.node, m);
					index.convertToken(*it.node, m);
				}
			}

			auto res2 = index.performSearch("ipsem", 3);
			for (auto &it : res2.nodes) {
				for (auto &m : it.matches) {
					index.resolveToken(*it.node, m);
					index.convertToken(*it.node, m);
				}
			}

			auto res3 = index.performSearch("lorem ipsum", 3);
			for (auto &it : res3.nodes) {
				for (auto &m : it.matches) {
					index.resolveToken(*it.node, m);
					index.convertToken(*it.node, m);
				}
			}

			return true;
		});

		runTest(stream, "Common test", count, passed, [&] {
			for (auto l = toInt(search::Language::Unknown); l <= toInt(search::Language::Simple); ++ l) {
				search::isStopword("test", search::Language(l));
				search::parseLanguage(search::getLanguageName(search::Language(l)));
			}

			for (auto l = toInt(search::ParserToken::AsciiWord); l <= toInt(search::ParserToken::HyphenatedWord_AsciiPart); ++ l) {
				search::getParserTokenName(search::ParserToken(l));
			}

			search::isStopword("test", (search::StemmerEnv *)nullptr);
			search::detectLanguage("test");
			search::detectLanguage("тест");
			search::detectLanguage("δοκιμή");
			search::detectLanguage("123");
			search::detectLanguage("_&123");
			search::detectLanguage(StringView());

			search::parseHtml(s_html, [] (StringView w) {
				search::stemWord(w, [] (StringView) { });
			});

			search::SearchData data; data.getLanguage();

			search::parsePhrase("12-123-123:1234:", [] (StringView, search::ParserToken) -> search::ParserStatus {
				return search::ParserStatus::PreventSubdivide;
			});

			search::parsePhrase("12-123:123:1234:", [] (StringView, search::ParserToken) -> search::ParserStatus {
				return search::ParserStatus::PreventSubdivide;
			});

			search::parsePhrase("12-123-123:1234-124:", [] (StringView, search::ParserToken) -> search::ParserStatus {
				return search::ParserStatus::PreventSubdivide;
			});

			search::parsePhrase("12-123-123:1234-124:124", [] (StringView, search::ParserToken) -> search::ParserStatus {
				return search::ParserStatus::PreventSubdivide;
			});

			search::parsePhrase("12@ / 12: ntcn 12 123:123:123-1234-124:124/1234", [] (StringView, search::ParserToken) -> search::ParserStatus {
				return search::ParserStatus::PreventSubdivide;
			});

			search::parsePhrase("123:123:123:1234:124:124", [] (StringView, search::ParserToken) -> search::ParserStatus {
				return search::ParserStatus::PreventSubdivide;
			});

			search::parsePhrase("test-123-word test/123/word тест-123-слово тест/123/слово/ тест/", [] (StringView, search::ParserToken) -> search::ParserStatus {
				return search::ParserStatus::PreventSubdivide;
			});

			search::parsePhrase("test-123-word test/123/word тест-123-слово тест/123/слово/ тест/", [] (StringView, search::ParserToken) -> search::ParserStatus {
				return search::ParserStatus::Continue;
			});

			search::parsePhrase("-124.567. -124.456@ -124.456№", [] (StringView, search::ParserToken) -> search::ParserStatus {
				return search::ParserStatus::PreventSubdivide;
			});

			search::parsePhrase("test-word кто-то кто-т13о", [] (StringView, search::ParserToken) -> search::ParserStatus {
				return search::ParserStatus::PreventSubdivide;
			});

			search::parsePhrase("-124.1256", [] (StringView, search::ParserToken) -> search::ParserStatus {
				return search::ParserStatus::Continue;
			});

			search::parsePhrase("-124.1256", [] (StringView, search::ParserToken) -> search::ParserStatus {
				return search::ParserStatus::PreventSubdivide;
			});

			search::parsePhrase("-124.125e125 124:123:234", [] (StringView, search::ParserToken) -> search::ParserStatus {
				return search::ParserStatus::PreventSubdivide;
			});

			search::parsePhrase("124.125.16 & test 12.3[]4@test", [] (StringView, search::ParserToken) -> search::ParserStatus {
				return search::ParserStatus::Continue;
			});

			search::parsePhrase("124.125.16", [] (StringView, search::ParserToken) -> search::ParserStatus {
				return search::ParserStatus::PreventSubdivide;
			});

			search::parsePhrase("/usr/local/bin/bljad", [] (StringView, search::ParserToken) -> search::ParserStatus {
				return search::ParserStatus::PreventSubdivide;
			});

			search::parsePhrase("_/usr/local/bin/bljad", [] (StringView, search::ParserToken) -> search::ParserStatus {
				return search::ParserStatus::Continue;
			});

			search::parsePhrase("_/usr/local/bin/bljad", [] (StringView, search::ParserToken) -> search::ParserStatus {
				return search::ParserStatus::PreventSubdivide;
			});

			return true;
		});

		runTest(stream, "Headlines", count, passed, [&] {
			search::Configuration cfg(search::Language::Russian);

			do {
				search::Configuration::HeadlineConfig headlines;
				// words at the end
				auto q = cfg.parseQuery("слово-через-дефиз");
				search::Vector<search::String> words = cfg.stemQuery(q);

				cfg.makeHtmlHeadlines(headlines, s_html, words, 10);
			} while (0);

			do {
				search::Configuration::HeadlineConfig headlines;
				headlines.maxWords = 2;
				auto q = cfg.parseQuery("личные");
				search::Vector<search::String> words = cfg.stemQuery(q);

				cfg.makeHtmlHeadlines(headlines, s_html, words, 10);
			} while (0);

			do {
				search::Configuration::HeadlineConfig headlines;
				// words at the end
				auto q = cfg.parseQuery("шутят психологи");
				search::Vector<search::String> words = cfg.stemQuery(q);

				cfg.makeHtmlHeadlines(headlines, s_html, words, 10);
			} while (0);

			do {
				search::Configuration::HeadlineConfig headlines;
				// words in middle
				auto q = cfg.parseQuery("уроках и экзаменах");
				search::Vector<search::String> words = cfg.stemQuery(q);

				cfg.makeHtmlHeadlines(headlines, s_html, words, 10);
			} while (0);

			do {
				search::Configuration::HeadlineConfig headlines;
				// words at the beginning
				auto q = cfg.parseQuery("люди похожий образ");
				search::Vector<search::String> words = cfg.stemQuery(q);

				cfg.makeHtmlHeadlines(headlines, s_html, words, 10);
			} while (0);

			do {
				search::Configuration::HeadlineConfig headlines;
				headlines.fragmentCallback = [] (StringView str, StringView) {

				};
				// words at the beginning
				auto q = cfg.parseQuery("люди похожий образ");
				search::Vector<search::String> words = cfg.stemQuery(q);
				cfg.makeHtmlHeadlines(headlines, s_html, words, 10);
			} while (0);
			return true;
		});

		runTest(stream, "Edit distance", count, passed, [&] {
			auto path1 = filesystem::currentDir<Interface>("resources/Enterobacteria_phage_1.fasta");
			auto path2 = filesystem::currentDir<Interface>("resources/mutated_99_perc.fasta");

			auto data1 = filesystem::readTextFile<Interface>(path1);
			auto data2 = filesystem::readTextFile<Interface>(path2);

			search::Distance dst(s_lipsum2, s_lipsum1);
			search::Distance dst2(s_lipsum4, s_lipsum3);
			search::Distance dst3(StringView(), s_lipsum3);
			search::Distance dst4(s_lipsum3, StringView());
			search::Distance dst5(data1, data2);

			search::Distance dst6 = dst;
			search::Distance dst7;
			dst6 = dst5;
			dst7 = move(dst6);
			search::Distance dst8 = move(dst7);

			auto s1 = dst.storage();
			auto s2 = dst2.storage();
			auto s3 = search::Distance::Storage::merge(s2, s1);
			s3.capacity();
			s3.clear();

			search::Distance::Storage s4 = s1;
			search::Distance::Storage s5;
			s4 = s2;
			s5 = move(s4);
			search::Distance::Storage s6 = move(s5);
			s6.set(0, search::Distance::Value(0));
			s6.reverse();

			auto info = dst2.info();
			stream << info << " " << !dst5.empty() << !dst4.empty() << !dst3.empty() << !dst2.empty() << !dst.empty() << !info.empty();
			if (!dst5.empty() && !dst4.empty() && !dst3.empty() && !dst2.empty() && !dst.empty() && !info.empty()) {
				auto c5 = dst.nmatch();
				auto c1 = dst.diff_canonical(c5, true);
				auto c2 = dst.diff_canonical(c5, false);
				auto c3 = dst.diff_original(c5, true);
				auto c4 = dst.diff_original(c5, false);
				if (c5 >= dst.size()) {
					return false;
				}

				return c1 == 47 && c2 == 47 && c3 == -47 && c4 == -47
						&& test1()
						&& test2()
						&& test3()
						&& test4()
						&& test5()
						&& test6()
						&& test7()
						&& test8()
						&& test9()
						&& test10()
						&& test11();
			}
			return false;
		});

		runTest(stream, "StringViewUtf8 reverse", count, passed, [&] {
			String test1("——test——");
			String test2("test——");
			String test3("тест——тест");
			String test4("——тест");

			StringViewUtf8 r1(test1);
			r1.trimUntil<StringViewUtf8::CharGroup<CharGroupId::Latin>>();

			StringViewUtf8 r2(test2);
			auto r3 = r2.backwardReadUntil<StringViewUtf8::CharGroup<CharGroupId::Latin>>();

			StringViewUtf8 r4(test3);
			r4.trimChars<StringViewUtf8::CharGroup<CharGroupId::Cyrillic>>();

			StringViewUtf8 r5(test4);
			auto r6 = r5.backwardReadChars<StringViewUtf8::CharGroup<CharGroupId::Cyrillic>>();

			if (r1 == "test" && r2 == "test" && r3 == "——" && r4 == "——" && r5 == "——" && r6 == "тест") {
				return true;
			} else {
				stream << "'" << r1 << "':" << (r1 == "test")
						<< " '" << r2 << "':" << (r2 == "test")
						<< " '" << r3 << "':" << (r3 == "——")
						<< " '" << r4 << "':" << (r4 == "——")
						<< " '" << r5 << "':" << (r5 == "——")
						<< " '" << r6 << "':" << (r6 == "тест");
				return false;
			}
		});

		runTest(stream, "StringView table optimization", count, passed, [&] {
			StringView test1("test \n\t test");
			test1.skipUntil<StringView::CharGroup<CharGroupId::WhiteSpace>>();
			auto r2 = test1.readChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();

			StringView test2("tEst12TeST test");
			auto r3 = test2.readChars<StringView::CharGroup<CharGroupId::Alphanumeric>>();

			return r2 == " \n\t " && r3 == "tEst12TeST" && test2 == " test";
		});

		runTest(stream, "Search url parser", count, passed, [&] {
			Vector<String> urls;
			urls.emplace_back("https://user0:1233456@google2.com.:8080#fragment");
			urls.emplace_back("https://user0:password@google.com:8080#fragment");
			urls.emplace_back("https://user1@google.com:8080#fragment");
			urls.emplace_back("user2:password@google.com:8080#fragment");
			urls.emplace_back("user3@google.com:8080#fragment");
			urls.emplace_back("https://йакреведко.рф/test/.././..////?query[креведко][treas][ds][]=qwert#аяклешня");
			urls.emplace_back("mailto:user@example.org?subject=caf%C3%A9&body=caf%C3%A9");
			urls.emplace_back("mailto:John.Doe@example.com");
			urls.emplace_back("https://john.doe@www.example.com:123/forum/questions/?tag=networking&order=newest#top");
			urls.emplace_back("ldap://[2001:db8::7]/c=GB?objectClass?one");
			urls.emplace_back("news:comp.infosystems.www.servers.unix");
			urls.emplace_back("tel:+1-816-555-1212");
			urls.emplace_back("telnet://192.0.2.16:80/");
			urls.emplace_back("urn:oasis:names:specification:docbook:dtd:xml:4.1.2");
			urls.emplace_back("user@[2001:db8::1]");
			urls.emplace_back("user:123@[2001:db8::1]:1234");
			urls.emplace_back("[2001:db8::1]/test");
			urls.emplace_back("sbkarr@127.0.0.1:1234");
			urls.emplace_back("https://sbkarr@127.0.0.1:1234");
			urls.emplace_back("git:1234@github.com:SBKarr/stappler.git");
			urls.emplace_back("doi:10.1000/182");
			urls.emplace_back("git@github.com:SBKarr/stappler.git");
			urls.emplace_back("ssh://git@atom0.stappler.org:21002/StapplerApi.git?qwr#test");
			urls.emplace_back("localhost");
			urls.emplace_back("localhost:8080");
			urls.emplace_back("localhost:8080/path?query#fragment");
			urls.emplace_back("localhost:8080?query#fragment");
			urls.emplace_back("localhost:8080#fragment");
			urls.emplace_back("localhost/test1/test2");
			urls.emplace_back("/usr/local/bin/bljad");
			urls.emplace_back("https://localhost/test/.././..////?query[test][treas][ds][]=qwert#rest");
			urls.emplace_back("http://localhost");
			urls.emplace_back("йакреведко.рф");
			urls.emplace_back("google.com#fragment");
			urls.emplace_back("google.com?testquery=123#fragment");
			urls.emplace_back("https://google.com?testquery=123#fragment");
			urls.emplace_back("https://google.com:8080?testquery=123#fragment");
			urls.emplace_back("https://google.com:8080#fragment");

			for (auto &it : urls) {
				StringView r(it);
				if (search::parseUrl(r, [&] (StringView r, search::UrlToken tok) {

				})) {
					if (!r.empty()) {
						return false;
					} else {
						UrlView v(it);
						auto viewString = v.get<memory::PoolInterface>();
						if (viewString != it && v.host != "google2.com") {
							stream << viewString << "!= " << it;
							return false;
						}
					}
				} else {
					stream << "Failed: " << it;
					return false;
				}
			}

			return true;
		});

		runTest(stream, "Search parser 2", count, passed, [&] {
			bool success = true;

			size_t count = 0;
			memory::dict<StringView, StringView> dict;
			search::Configuration cfg(search::Language::Russian);

			auto stemUrlCb = [&] (StringView v, const Callback<void(StringView)> &stemCb) -> bool {
				UrlView u(v);
				if (!u.user.empty()) { stemCb(u.user); }
				if (!u.host.empty()) { stemCb(u.host); }
				if (!u.port.empty()) { stemCb(u.port); }
				u.path.split<StringView::Chars<'/'>>([&] (StringView cmp) {
					stemCb(cmp);
				});
				return true;
			};

			cfg.setStemmer(search::ParserToken::Email, stemUrlCb);
			cfg.setStemmer(search::ParserToken::Path, stemUrlCb);
			cfg.setStemmer(search::ParserToken::Url, stemUrlCb);
			cfg.setStemmer(search::ParserToken::Custom, [&] (StringView v, const Callback<void(StringView)> &stemCb) -> bool {
				stemCb(v);
				return true;
			});

			search::SearchVector vec;

			auto pushWord = [&] (StringView stem, StringView word, search::ParserToken) {

			};

			count = cfg.makeSearchVector(vec, SearchParserTest, search::SearchRank::A, count, pushWord);

			return success;
		});

		runTest(stream, "Search configuration", count, passed, [&] {
			auto str = s_lipsum1;

			search::Configuration cfg(search::Language::Russian);
			search::SearchVector vec;

			cfg.makeSearchVector(vec, str);

			auto vecData = cfg.encodeSearchVectorData(vec);

			cfg.parseQuery("(Mauris (dapibus dignissim))");
			cfg.parseQuery("Mauris | (dapibus dignissim)  (dapibus dignissim) | (dapibus dignissim)");
			cfg.parseQuery(R"Query("!bibendum !sem")Query");
			cfg.parseQuery(R"Query((!bibendum) !sem)")Query");
			cfg.parseQuery(R"Query((bibendum | sem asdf a) "test a test")Query");
			cfg.parseQuery(R"Query((bibendum bibendum | sem) "test a test")Query");
			cfg.parseQuery(R"Query(test | (bibendum | sem))Query");
			cfg.parseQuery(R"Query(test | "bibendum | sem")Query");
			cfg.parseQuery(R"Query(bibendum test | "!bibendum | sem")Query");

			cfg.parseQuery("(Mauris (dapibus dignissim))", true);
			cfg.parseQuery("Mauris | (dapibus dignissim)  (dapibus dignissim) | (dapibus dignissim)", true);
			cfg.parseQuery(R"Query("!bibendum !sem")Query", true);
			cfg.parseQuery(R"Query((!bibendum) !sem)")Query", true);
			cfg.parseQuery(R"Query((bibendum | sem) "test a test")Query", true);
			cfg.parseQuery(R"Query((bibendum bibendum | sem) "test a test")Query", true);
			cfg.parseQuery(R"Query(test | (bibendum | sem))Query", true);
			cfg.parseQuery(R"Query(test | "bibendum | sem")Query", true);
			cfg.parseQuery(R"Query(bibendum test | "!bibendum | sem")Query", true);

			auto q4 = cfg.parseQuery("\"Mauris dapibus\" dignissim");
			auto q5 = cfg.parseQuery("Mauris | dapibus | dignissim");

			if (cfg.isMatch(vec, q4) && q4.isMatch(vecData)) {
				q4.rankQuery(vec, search::Normalization::Default
						| search::Normalization::DocLengthLog
						| search::Normalization::DocLength
						| search::Normalization::UniqueWordsCount
						| search::Normalization::UniqueWordsCountLog
						| search::Normalization::Self);
			}

			cfg.isMatch(vec, "\"Mauris dapibus\" dignissim");
			cfg.isMatch(vec, "Mauris | dapibus | dignissim");
			cfg.isMatch(vec, q5);
			q5.isMatch(vecData);

			auto q3 = cfg.parseQuery(R"Query(!(!test subtest1) !test (test | !subtest2 | subtest3) !"test1 | test2" "!test3 test4")Query");
			q3.describe(std::cout, 1); std::cout << "\n";
			q3.decompose([] (StringView) { }, [] (StringView) { });
			q3.normalize();
			q3.decompose([] (StringView) { }, [] (StringView) { });
			q3.encode([] (StringView) {}, search::SearchQuery::Format::Stappler);
			q3.encode([] (StringView) {}, search::SearchQuery::Format::Postgresql);

			auto q2 = cfg.parseQuery(R"Query(!test1  !(test2 test3) (!test4 test5))Query");
			q2.describe(std::cout, 1); std::cout << "\n";
			q2.decompose([] (StringView) { }, [] (StringView) { });
			q2.normalize();
			q2.encode([] (StringView) {}, search::SearchQuery::Format::Stappler);
			q2.encode([] (StringView) {}, search::SearchQuery::Format::Postgresql);

			auto q1 = cfg.parseQuery(R"Query("bibendum a sem")Query");
			q1.describe(std::cout, 1); std::cout << "\n";
			q1.decompose([] (StringView) { }, [] (StringView) { });
			q1.normalize();
			q1.decompose([] (StringView) { }, [] (StringView) { });

			search::SearchQuery q0;
			q0.neg = true;
			q0.op = search::SearchOp::And;
			q0.args.emplace_back(search::SearchQuery("bibendum"));
			q0.args.emplace_back(search::SearchQuery("sem"));
			q0.decompose([] (StringView) { }, [] (StringView) { });
			q0.normalize();
			q0.decompose([] (StringView) { }, [] (StringView) { });
			q0.encode([] (StringView) {}, search::SearchQuery::Format::Stappler);
			q0.encode([] (StringView) {}, search::SearchQuery::Format::Postgresql);

			search::SearchQuery q6;
			q6.neg = true;
			q6.op = search::SearchOp::Or;
			q6.args.emplace_back(search::SearchQuery("bibendum"));
			q6.args.emplace_back(search::SearchQuery("sem"));
			q6.decompose([] (StringView) { }, [] (StringView) { });
			q6.normalize();
			q6.decompose([] (StringView) { }, [] (StringView) { });
			q6.encode([] (StringView) {}, search::SearchQuery::Format::Stappler);
			q6.encode([] (StringView) {}, search::SearchQuery::Format::Postgresql);
			q6.clear();

			search::SearchQuery q7(search::SearchOp::Or, "test");
			q7.decompose([] (StringView) { }, [] (StringView) { });
			q7.encode([] (StringView) {}, search::SearchQuery::Format::Stappler);
			q7.encode([] (StringView) {}, search::SearchQuery::Format::Postgresql);
			q7.neg = true;
			q7.decompose([] (StringView) { }, [] (StringView) { });
			q7.encode([] (StringView) {}, search::SearchQuery::Format::Stappler);
			q7.encode([] (StringView) {}, search::SearchQuery::Format::Postgresql);
			q7.block = search::SearchQuery::Block::Parentesis;
			q7.encode([] (StringView) {}, search::SearchQuery::Format::Stappler);
			q7.encode([] (StringView) {}, search::SearchQuery::Format::Postgresql);
			q7.block = search::SearchQuery::Block::Quoted;

			search::RankingValues values;
			values.rank(search::SearchRank::A);
			values.rank(search::SearchRank::B);
			values.rank(search::SearchRank::C);
			values.rank(search::SearchRank::D);
			values.rank(search::SearchRank::Unknown);
			values.rank(search::SearchRank(5));

			cfg.setLanguage(search::Language::Russian);
			cfg.getLanguage();

			cfg.getPreStem();
			cfg.setPreStem([] (StringView str, search::ParserToken) -> search::Vector<search::StringView> {
				if (str == "Mauris" || str == "Mauris-san") {
					return search::Vector<search::StringView>{
						StringView("Mauris"),
						StringView("Maurus"),
						StringView("Maures-san"),
					};
				} else if (str == "ipsum") {
					return search::Vector<search::StringView>{
						StringView("ipsum"),
					};
				}
				return search::Vector<search::StringView>();
			});

			cfg.parseQuery("admin@stappler.org");
			cfg.parseQuery("\"Mauris dapibus\" dignissim");
			cfg.parseQuery("Mauris-san | Mauris | dapibus | dignissim");
			cfg.stemPhrase(s_lipsum1, [] (StringView, StringView, search::ParserToken) { });
			cfg.stemPhrase(s_lipsum3, [] (StringView, StringView, search::ParserToken) { });
			cfg.stemPhrase(SearchParserTest, [] (StringView, StringView, search::ParserToken) { });
			cfg.stemPhrase(SearchParserTest2, [] (StringView, StringView, search::ParserToken) { });
			cfg.stemHtml(s_html, [] (StringView, StringView, search::ParserToken) { });

			search::SearchVector svec;
			size_t counter = 0;
			counter += cfg.makeSearchVector(svec, s_lipsum1, search::SearchRank::A, counter);
			counter += cfg.makeSearchVector(svec, s_lipsum3, search::SearchRank::A, counter);
			counter += cfg.makeSearchVector(svec, SearchParserTest, search::SearchRank::A, counter);
			counter += cfg.makeSearchVector(svec, SearchParserTest2, search::SearchRank::A, counter);

			cfg.encodeSearchVectorPostgres(svec);

			cfg.getStemmer(search::ParserToken::AsciiHyphenatedWord)("aeetds-adff", [] (StringView) { });
			cfg.getStemmer(search::ParserToken::Blank)("#$%^&*", [] (StringView) { });
			cfg.setStemmer(search::ParserToken::Blank, [] (StringView, const Callback<void(StringView)> &) {
				return false;
			});
			cfg.getStemmer(search::ParserToken::Blank)("#$%^&*", [] (StringView) { });

			cfg.getCustomStopwords();
			cfg.setCustomStopwords(nullptr);

			search::Configuration cfg2;
			cfg2.setLanguage(search::Language::Simple);
			cfg2.stemPhrase(s_lipsum1, [] (StringView, StringView, search::ParserToken) { });
			cfg2.stemPhrase(s_lipsum3, [] (StringView, StringView, search::ParserToken) { });
			cfg2.stemPhrase(SearchParserTest, [] (StringView, StringView, search::ParserToken) { });
			cfg2.stemPhrase(SearchParserTest2, [] (StringView, StringView, search::ParserToken) { });

			if (q1.isMatch(vec)) {
				return true;
			}
			return false;
		});

		runTest(stream, "Search parser", count, passed, [&] {
			auto str = StringView("31:26:00 00 :14239/0/11/04:1001/Б 36–34–6:00–00–00:00:2780:2–27–3 №52:18:05 00 00:0000:05965 123test "
					"77-77-09/020/2008-082  63:01:0736001:550  63:01:0736001:550 "
					"/usr/local/foo.txt test/stest 12.34.56.78:1234 12.34.56.78: "
					"12.34.56.78@1234 12.34.56.78@test 12@test 12.34@test "
					"John.Doe@example.com mailto:John.Doe@example.com up-to-date https://sbkarr@127.0.0.1:1234 "
					"postgresql-beta1 123.test -1.234e56 -1.234 -1234 1234 8.3.0 &amp; a_nichkov@mail.ru");

			Vector<Pair<StringView, search::ParserToken>> vec{
				pair("31:26:00 00 :14239", search::ParserToken::Custom),
				pair("/0/11/04:1001/Б", search::ParserToken::Path),
				pair("36–34–6:00–00–00:00:2780:2–27–3", search::ParserToken::Custom),
				pair("52:18:05 00 00:0000:05965", search::ParserToken::Custom),
				pair("123test", search::ParserToken::NumWord),
				pair("77-77-09/020/2008-082", search::ParserToken::Custom),
				pair("63:01:0736001:550", search::ParserToken::Custom),
				pair("63:01:0736001:550", search::ParserToken::Custom),
				pair("/usr/local/foo.txt", search::ParserToken::Path),
				pair("test/stest", search::ParserToken::Url),
				pair("12.34.56.78:1234", search::ParserToken::Url),
				pair("12.34.56.78", search::ParserToken::Version),
				pair("12.34.56.78@1234", search::ParserToken::Email),
				pair("12.34.56.78@test", search::ParserToken::Email),
				pair("12@test", search::ParserToken::Email),
				pair("12.34@test", search::ParserToken::Email),
				pair("John.Doe@example.com", search::ParserToken::Email),
				pair("mailto:John.Doe@example.com", search::ParserToken::Url),
				pair("up-to-date", search::ParserToken::AsciiHyphenatedWord),
				pair("up", search::ParserToken::HyphenatedWord_AsciiPart),
				pair("to", search::ParserToken::HyphenatedWord_AsciiPart),
				pair("date", search::ParserToken::HyphenatedWord_AsciiPart),
				pair("https://sbkarr@127.0.0.1:1234", search::ParserToken::Url),
				pair("postgresql-beta1", search::ParserToken::NumHyphenatedWord),
				pair("postgresql", search::ParserToken::HyphenatedWord_AsciiPart),
				pair("beta1", search::ParserToken::HyphenatedWord_NumPart),
				pair("123", search::ParserToken::Integer),
				pair("test", search::ParserToken::AsciiWord),
				pair("-1.234e56", search::ParserToken::ScientificFloat),
				pair("-1.234", search::ParserToken::Float),
				pair("-1234", search::ParserToken::Integer),
				pair("1234", search::ParserToken::Integer),
				pair("8.3.0", search::ParserToken::Version),
				pair("&amp;", search::ParserToken::XMLEntity),
				pair("a_nichkov@mail.ru", search::ParserToken::Email),
			};

			bool success = true;
			size_t idx = 0;
			search::parsePhrase(str, [&] (StringView str, search::ParserToken tok) {
				switch (tok) {
				case search::ParserToken::Blank: break;
				default:
					if (vec[idx].first != str || vec[idx].second != tok) {
						success = false;
					}
					++ idx;
					break;
				}
				return search::ParserStatus::Continue;
			});
			return success;
		});

		_desc = stream.str();

		return count == passed;
	}
} _SearchTest;

}

#endif
