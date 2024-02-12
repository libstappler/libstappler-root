/**
Copyright (c) 2017-2022 Roman Katuntsev <sbkarr@stappler.org>
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

#if MODULE_STAPPLER_SQL

#include "SPSql.h"

namespace STAPPLER_VERSIONIZED stappler::app::test {

constexpr auto SqlString1 =
R"(SELECT "field", "field" AS "alias", "database" AS "alias" FROM table, table WHERE("alias"=value)AND("field"=1234)OR("field"!=false)OR(("time">1234 AND "time"<123400)) ORDER BY "field" DESC LIMIT 12 OFFSET 16;)";

constexpr auto SqlString2 =
R"(WITH query AS (SELECT * FROM sqtable), query AS (SELECT * FROM sqtable)SELECT "field1", "field2" FROM table WHERE("alias"=value)AND(("field"=value)OR("field"=value));)";

constexpr auto SqlString3 =
R"(INSERT INTO table("field1", "field2")VALUES(test1,test2),(test3,test4);)";

constexpr auto SqlString4 =
R"(INSERT INTO table("field1", "field2")VALUES(test1,test2)ON CONFLICT DO NOTHING RETURNING *;)";

constexpr auto SqlString5 =
R"(INSERT INTO table("field1", "field2", "field3")VALUES(test1,test2,test3)ON CONFLICT("field1") DO UPDATE SET "field2"=EXCLUDED."field2", "field3"=DEFAULT WHERE("field1"=alias) RETURNING *;)";

constexpr auto SqlString6 =
R"(UPDATE table AS alias SET "field1"=value1, "field1"=value2 WHERE("field3"!=false) RETURNING *;)";

constexpr auto SqlString7 =
R"(DELETE FROM table AS alias WHERE("field3"!=false) RETURNING *;)";

struct SqlTest : Test {
	SqlTest() : Test("SqlTest") { }

	virtual bool run() override {
		using namespace sql;

		using BinderType = SimpleBinder<memory::StandartInterface>;
		using QueryType = Query<BinderType, memory::StandartInterface>;

		using Field = QueryType::Field;

		StringStream stream;
		QueryType query;
		query.select()
				.fields("field", Field("field").as("alias")).field(Field("database", "field").as("alias"))
				.from("table").from("table")
				.where("alias", sql::Comparation::Equal, "value")
				.where(sql::Operator::And, "field", sql::Comparation::Equal, Value(1234))
				.where(sql::Operator::Or, "field", sql::Comparation::NotEqual, Value(false))
				.where(sql::Operator::Or, "time", sql::Comparation::BetweenValues, Value(1234), Value(123400))
				.order(sql::Ordering::Descending, "field")
				.limit(12, 16)
				.finalize();
		auto test1 = (query.getStream().str() == StringView(SqlString1));

		query = QueryType();
		query.with("query", [] (QueryType::GenericQuery &q) {
			q.select().all().from("sqtable");
		}).with("query", [] (QueryType::GenericQuery &q) {
			q.select().all().from("sqtable");
		}).select().fields("field1", "field2").from("table")
			.where("alias", sql::Comparation::Equal, "value")
			.parenthesis(sql::Operator::And, [] (QueryType::WhereBegin &q) {
				q.where("field", sql::Comparation::Equal, "value")
						.where(sql::Operator::Or, "field", sql::Comparation::Equal, "value");
		}).finalize();
		auto test2 = (query.getStream().str() == StringView(SqlString2));

		QueryType insertQuery;
		insertQuery.insert("table")
				.field("field1").field("field2")
				.values().value("test1").value("test2")
				.values().value("test3").value("test4")
				.finalize();
		auto test3 = (insertQuery.getStream().str() == StringView(SqlString3));

		insertQuery = QueryType();
		insertQuery.insert("table")
				.field("field1").field("field2")
				.values().value("test1").value("test2")
				.onConflictDoNothing()
				.returning().all()
				.finalize();
		auto test4 = (insertQuery.getStream().str() == StringView(SqlString4));

		insertQuery = QueryType();
		insertQuery.insert("table")
				.field("field1").field("field2").field("field3")
				.values().value("test1").value("test2").value("test3")
				.onConflict("field1").doUpdate().excluded("field2").def("field3")
				.where("field1", sql::Comparation::Equal, "alias")
				.returning().all()
				.finalize();
		auto test5 = (insertQuery.getStream().str() == StringView(SqlString5));

		QueryType updateQuery;
		updateQuery.update("table", "alias")
				.set("field1", "value1").set("field1", "value2")
				.where("field3", sql::Comparation::NotEqual, Value(false))
				.returning().all()
				.finalize();
		auto test6 = (updateQuery.getStream().str() == StringView(SqlString6));

		QueryType deleteQuery;
		deleteQuery.remove("table", "alias")
				.where("field3", sql::Comparation::NotEqual, Value(false))
				.returning().all()
				.finalize();
		auto test7 = (deleteQuery.getStream().str() == StringView(SqlString7));

		_desc = stream.str();

		return test1 && test2 && test3 && test4 && test5 && test6 && test7;
	}

} _SqlTest;

}

#endif
