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

#if MODULE_STAPPLER_DB

#include "SPSqlDriver.h"
#include "SPDbScheme.h"
#include "SPDbFieldExtensions.h"
#include "SPSqlHandle.h"

namespace STAPPLER_VERSIONIZED stappler::app::test {

constexpr auto TEST_STRING_1 = R"(Этапы принятия решений и их характеристики в этой простой модели таковы:

(1) Определение поля для анализа: принимая экономические решения, человек изучает только экономические данные.

(2) Сбор информации: человек способен собрать всю информацию, имеющую отношение к делу.

(3) Выработка решения: человек способен полностью обработать полученную информацию и принять наилучшее для себя решение.

(4) Реализация решения: человек способен вести себя в строгом соответствии с принятым решением. Его поведение предсказуемо
изменяет экономические данные.

Второй и третий этапы этой модели - сбор информации и выработка на ее основе решения - характеризуют способность человека
быть рациональным. Рациональность можно определить, как способность человека непротиворечиво ранжировать имеющиеся
альтернативы и выбирать из них наилучшую в соответствии с определенным критерием. Мы говорим о неограниченной рациональности,
если, как в этой простой модели, считаем возможным собрать и обработать всю относящуюся к делу информацию, а также принять
наилучшее решение (подробно эти вопросы изучают в разделе «Потребительский выбор» экономической теории).

Однако в экономике, как и в авиации, невозможно долго игнорировать человеческий фактор.

Простая модель затрещала по швам. Сначала Герберт Саймон ввел в экономическую теорию понятие ограниченной рациональности,
указав на то, что наши мыслительные способности, а также технические и финансовые возможности собирать и обрабатывать
информацию ограниченны. Затем и вовсе возник новый раздел экономической науки - поведенческая экономика. А простая модель
стала сложной.)";

constexpr auto TEST_STRING_2 = R"(
Под культурными нормами мы понимаем разделяемые группой людей ценности и убеждения. Говоря очень коротко, это представления
о том, что такое хорошо и что такое плохо, разделяемые большинством. Можно или нет списывать на уроках и экзаменах, стоит
ли откладывать средства на черный день, могут ли женщины управлять государством, должно ли государство помогать бедным…
Если решение, которое надо принять человеку, лежит в той же сфере, что и разделяемые им культурные ценности, то человек
зачастую принимает решение, исходя из этих ценностей, не подвергая их мыслительному анализу.

Формируются культурные нормы обычно как эффективный ответ на определенное состояние мира вокруг нас (например, более
высокое доверие в обществе может сформироваться в ответ на необходимость общинного страхования от неурожая в нестабильных
климатических условиях[1]). С помощью воспитания культурные нормы передаются из поколения в поколение. Если окружающий
мир меняется мало, то культурные нормы, разделяемые многими поколениями, приобретают особую устойчивость. Их стабильность
поддерживают организации, получающие выгоду из существующего положения вещей, а также формальные и неформальные законы и
нормы.

При быстрых изменениях мира вокруг нас (например, как в последние 100–150 лет) традиционные культурные ценности перестают
быть эффективными, но в силу приобретенной устойчивости продолжают влиять на наши чувства, мысли и решения.)";

struct DbTest : MemPoolTest {
	DbTest() : MemPoolTest("DbTest"), _search(search::Language::Russian) { }

	virtual bool run(pool_t *pool) {
		StringStream stream;
		bool success = true;
		stream << "\n";

		// open db driver interface
		// driver sqlite3 build with static library
		// for pqsql, you can manually specify path to libpq with pqsql:<path>
		auto driver = db::sql::Driver::open(pool, nullptr, "sqlite3");
		if (!driver) {
			return false;
		}

		// connection parameters is backend-dependent
		// create in-memory temporary database with sqlite
		auto driverHandle = driver->connect(mem_pool::Map<StringView, StringView>{
			pair("dbname", "db.sqlite"),
		});

		if (!driverHandle.get()) {
			return false;
		}

		db::Scheme dataScheme("data_scheme");

		dataScheme.define({
			db::Field::Integer("number"),
			db::Field::Integer("ctime", db::Flags::AutoCTime),
			db::Field::Integer("mtime", db::Flags::AutoCTime),
			db::Field::Text("name", db::Flags::Indexed | db::Flags::PatternIndexed),
			db::Field::Password("password"),
			db::Field::Float("value"),
			db::Field::Custom(new db::FieldTextArray("textArray", db::Flags::Indexed)),
			db::Field::Text("title", db::Flags::Indexed | db::Flags::PatternIndexed),
			db::Field::Text("desc", db::MaxLength(10_MiB)),
			db::Field::FullTextView("tsv", db::FullTextViewFn([this] (const db::Scheme &scheme, const db::Value &obj) -> db::FullTextVector {
				size_t count = 0;
				db::FullTextVector vec;

				count = _search.makeSearchVector(vec, obj.getString("name"), db::FullTextRank::A, count);
				count = _search.makeSearchVector(vec, obj.getString("desc"), db::FullTextRank::B, count);

				return vec;
			}), /* db::FullTextQueryFn([this] (const db::Value &data) -> db::FullTextQuery {
				return _search.parseQuery(data.getString());
			}), */ _search, Vector<String>({"title", "desc"})),
		});

		// we need BackendInterface to make queries, or an Adapter wrapper
		driver->performWithStorage(driverHandle, [&] (const db::Adapter &adapter) {
			db::BackendInterface::Config cfg;
			cfg.name = StringView("temporary_db");

			mem_pool::Map<StringView, const db::Scheme *> schemes({
				pair(dataScheme.getName(), &dataScheme)
			});

			if (!adapter.init(cfg, schemes)) {
				success = false;
				return;
			}

			adapter.performWithTransaction([&] (const db::Transaction &t) {
				auto d = dataScheme.select(t, db::Query().include("name"));
				for (auto &it : d.asArray()) {
					dataScheme.remove(t, it);
				}
				return true;
			});

			adapter.performWithTransaction([&] (const db::Transaction &t) {
				auto v = dataScheme.create(t, mem_pool::Value({
					pair("number", db::Value(42)),
					pair("name", db::Value("testName")),
					pair("password", db::Value("Pas$w0rd")),
					pair("value", db::Value(37.72)),
					pair("textArray", db::Value({
						db::Value("text string 1"),
						db::Value("text string 2"),
					})),
					pair("title", db::Value("1.1.1 Модель принятия экономических решений")),
					pair("desc", db::Value(TEST_STRING_1)),
				}));

				auto v2 = dataScheme.create(t, mem_pool::Value({
					pair("number", db::Value(42)),
					pair("name", db::Value("testName2")),
					pair("password", db::Value("Pas$w0rd")),
					pair("value", db::Value(37.72)),
					pair("textArray", db::Value({
						db::Value("text string 3"),
						db::Value("text string 4"),
					})),
					pair("title", db::Value("1.1.2. Проблемы восприятия данных")),
					pair("desc", db::Value(TEST_STRING_2)),
				}));

				if (!v) {
					success = false;
					return false;
				}

				auto tsvResult = dataScheme.select(t, db::Query().select("tsv", Value("культурные нормы")));

				auto val = dataScheme.select(t, db::Query().select("textArray", Value("text string 4")));
				auto v1 = dataScheme.get(t, v.getInteger("__oid"));

				//SELECT *, ts_query_rank(1) as rank FROM ts_query_ids(1) left join on table(__oid=__ts_id) WHERE ts_query_valid(1) ORDER BY rank;

				if (auto h = (db::sql::SqlHandle *)adapter.getBackendInterface()) {
					h->performSimpleSelect("SELECT * FROM data_scheme, sp_unwrap(data_scheme.textArray) as unwrap "
							"WHERE unwrap.__unwrap_value = 'text string 4';",
							[&] (db::sql::Result &res) {
						for (auto it : res) {
							std::cout << data::EncodeFormat::Pretty << it.encode() << "\n";
						}
					});
				}

				v.erase("password");
				v1.erase("password");
				v.erase("tsv");
				v1.erase("tsv");

				if (v != v1) {
					success = false;
					return false;
				}
				return true;
			});
		});

		driver->finish(driverHandle);

		_desc = stream.str();

		return success;
	}

protected:
	search::Configuration _search;
} _DbTest;

}

#endif
