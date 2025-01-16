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

#include "SPWebTools.h"
#include "SPWebRoot.h"
#include "SPDbUser.h"
#include "SPDbContinueToken.h"
#include "SPDbQuery.h"
#include "SPSqlHandle.h"

namespace STAPPLER_VERSIONIZED stappler::web::tools {

static Status makeUnavailablePage(Request &req) {
	req.runPug("virtual://html/errors_unauthorized.pug", [&] (pug::Context &exec, const pug::Template &) -> bool {
		exec.set("version", Value(config::getWebserverVersionString()));
		return true;
	});
	return HTTP_UNAUTHORIZED;
}

Status ErrorsGui::onTranslateName(Request &req) {
	req.setResponseHeader("WWW-Authenticate", req.host().getHostInfo().hostname);

	auto t = req.acquireDbTransaction();
	if (!t) {
		return HTTP_INTERNAL_SERVER_ERROR;
	}

	auto u = req.getAuthorizedUser();
	if (u && u->isAdmin()) {
		auto errorScheme = req.host().getErrorScheme();
		auto &d = req.getInfo().queryData;
		if (d.hasValue("delete")) {
			if (auto id = StringView(d.getString("delete")).readInteger().get(0)) {
				u = req.getAuthorizedUser();
				if (u && u->isAdmin()) {
					if (errorScheme) {
						errorScheme->remove(t, id);
					}
				}
			}

			auto c = d.getString("c");
			auto tag = d.getString("tag");

			StringStream url;
			url << req.getInfo().url.path;

			if (!c.empty()) {
				auto token = db::ContinueToken(d.getString("c"));
				db::Query q;
				if (!tag.empty()) {
					q.select("tags", db::Comparation::Includes, Value(tag));
				}
				token.refresh(*errorScheme, t, q);

				url << "?c=" << token.encode();
			}
			if (!tag.empty()) {
				if (c.empty()) {
					url << "?tag=" << tag;
				} else {
					url << "&tag=" << tag;
				}
			}
			return req.redirectTo(url.str());
		}

		String selectedTag;
		if (d.isString("tag")) {
			selectedTag = d.getString("tag");
		}

		Value errorsData;
		auto token = d.isString("c") ? db::ContinueToken(d.getString("c")) : db::ContinueToken("__oid", 25, true);

		if (errorScheme) {
			db::Query q;
			if (!selectedTag.empty()) {
				q.select("tags", db::Comparation::Includes, Value(selectedTag));
			}
			errorsData = token.perform(*errorScheme, t, q);
		}

		req.runPug("virtual://html/errors.pug", [&] (pug::Context &exec, const pug::Template &tpl) -> bool {
			ServerGui::defineBasics(exec, req, u);

			if (!selectedTag.empty()) {
				exec.set("selectedTag", Value(selectedTag));
			}

			if (auto iface = dynamic_cast<db::sql::SqlHandle *>(t.getAdapter().getBackendInterface())) {
				Value ret;

				auto driver = iface->getDriver();

				auto tagUnwrapQuery = (driver->getDriverName() == "sqlite")
						? toString("SELECT __oid, __unwrap_value as tag FROM ", errorScheme->getName(), ", sp_unwrap(tags) as unwrap")
						: toString("SELECT __oid, unnest(tags) as tag FROM ", errorScheme->getName());

				auto query = toString("SELECT tag, COUNT(*) FROM (", tagUnwrapQuery, ") s GROUP BY tag;;");
				iface->performSimpleSelect(query, [&] (db::Result &res) {
					for (auto it : res) {
						ret.addValue(Value({
							pair("tag", Value(it.toString(0))),
							pair("count", Value(it.toInteger(1))),
							pair("selected", Value(it.toString(0) == selectedTag))
						}));
					}
				});

				exec.set("tags", move(ret));
			}

			if (errorsData.size() > 0) {
				exec.set("errors", true, &errorsData);
			}

			auto current = token.encode();

			Value cursor({
				pair("start", Value(token.getStart())),
				pair("end", Value(token.getEnd())),
				pair("current", Value(current)),
				pair("total", Value(token.getTotal())),
			});

			if (token.hasNext()) {
				cursor.setString(token.encodeNext(), "next");
			}

			if (token.hasPrev()) {
				cursor.setString(token.encodePrev(), "prev");
			}

			exec.set("cursor", move(cursor));
			return true;
		});
		return DONE;
	} else {
		return makeUnavailablePage(req);
	}
}

Status HandlersGui::onTranslateName(Request &req) {
	req.setResponseHeader("WWW-Authenticate", req.host().getHostInfo().hostname);

	auto u = req.getAuthorizedUser();
	if (u && u->isAdmin()) {
		auto &hdl = req.host().getRequestHandlers();

		Value servh;
		Value ret;
		for (auto &it : hdl) {
			auto &v = (it.second.component == "root") ? servh.emplace() : ret.emplace();
			v.setValue(it.first, "name");
			if (!it.second.data.isNull()) {
				v.setValue(it.second.data, "data");
			}
			if (it.second.scheme) {
				v.setString(it.second.scheme->getName(), "scheme");
			}
			if (!it.second.component.empty()) {
				if (it.second.component == "root") {
					v.setBool(true, "server");
				}
				v.setString(it.second.component, "component");
			}
			if (it.first.back() == '/') {
				v.setBool(true, "forSubPaths");
			}
			if (it.second.map) {
				auto base = StringView(it.first);
				if (base.ends_with("/")) {
					base = StringView(base, base.size() - 1);
				}
				auto &m = v.emplace("map");
				for (auto &iit : it.second.map->getHandlers()) {
					auto &mVal = m.emplace();
					mVal.setString(iit.getName(), "name");
					mVal.setString(toString(base, iit.getPattern()), "pattern");

					switch (iit.getMethod()) {
					case RequestMethod::Get: mVal.setString("GET", "method"); break;
					case RequestMethod::Put: mVal.setString("PUT", "method"); break;
					case RequestMethod::Post: mVal.setString("POST", "method"); break;
					case RequestMethod::Delete: mVal.setString("DELETE", "method"); break;
					case RequestMethod::Connect: mVal.setString("CONNECT", "method"); break;
					case RequestMethod::Options: mVal.setString("OPTIONS", "method"); break;
					case RequestMethod::Trace: mVal.setString("TRACE", "method"); break;
					case RequestMethod::Patch: mVal.setString("PATCH", "method"); break;
					default: break;
					}

					auto &qScheme = iit.getQueryScheme().getFields();
					if (!qScheme.empty()) {
						auto &qVal = mVal.emplace("query");
						for (auto &it : qScheme) {
							auto &v = qVal.addValue(it.second.getTypeDesc());
							v.setString(it.first, "name");
						}
					}

					auto &iScheme = iit.getInputScheme().getFields();
					if (!iScheme.empty()) {
						auto &qVal = mVal.emplace("input");
						for (auto &it : iScheme) {
							auto &v = qVal.addValue(it.second.getTypeDesc());
							v.setString(it.first, "name");
						}
					}
				}
			}
		}

		for (auto &it : servh.asArray()) {
			ret.addValue(move(it));
		}

		req.runPug("virtual://html/handlers.pug", [&] (pug::Context &exec, const pug::Template &tpl) -> bool {
			ServerGui::defineBasics(exec, req, u);
			exec.set("handlers", sp::move(ret));
			return true;
		});
		return DONE;
	} else {
		return makeUnavailablePage(req);
	}
}

Status ReportsGui::onTranslateName(Request &req) {
	req.setResponseHeader("WWW-Authenticate", req.host().getHostInfo().hostname);

	auto u = req.getAuthorizedUser();
	if (u && u->isAdmin()) {
		auto reportsAddress = req.host().getDocumentRootPath(".reports");

		auto readTime = [&] (StringView name) {
			if (name.starts_with("crash.")) {
				name.skipUntil<StringView::CharGroup<CharGroupId::Numbers>>();
				return Time::microseconds(name.readInteger(10).get());
			} else {
				name.skipUntil<StringView::CharGroup<CharGroupId::Numbers>>();
				return Time::milliseconds(name.readInteger(10).get());
			}
		};

		Value paths;
		Value file;
		if (_subPath.empty() || _subPath == "/") {
			filesystem::ftw(reportsAddress, [&] (StringView path, bool isFile) {
				if (isFile) {
					auto name = filepath::lastComponent(path);
					if (name.starts_with("crash.") || name.starts_with("update.")) {
						auto &info = paths.emplace();
						info.setString(name, "name");
						if (auto t = readTime(name)) {
							info.setInteger(t.toMicros(), "time");
							info.setString(t.toHttp<Interface>(), "date");
						}
					}
				}
			}, 1);

			if (paths) {
				std::sort(paths.asArray().begin(), paths.asArray().end(), [&] (const Value &l, const Value &r) {
					return l.getInteger("time") > r.getInteger("time");
				});
			}
		} else if (_subPathVec.size() == 1) {
			auto name = _subPathVec.front();

			auto reportsAddress = filepath::merge<Interface>(req.host().getDocumentRootPath(".reports"), name);

			filesystem::Stat stat;
			auto exists = filesystem::stat(reportsAddress, stat);
			if (exists && !stat.isDir) {
				if (req.getInfo().queryData.getBool("remove")) {
					filesystem::remove(reportsAddress);
					return req.redirectTo(StringView(_originPath, _originPath.size() - _subPath.size()));
				}
				auto data = filesystem::readTextFile<Interface>(reportsAddress);
				if (!data.empty()) {
					file.setString(move(data), "data");
					auto name = filepath::lastComponent(reportsAddress);
					file.setString(name, "name");
					if (auto t = readTime(name)) {
						file.setInteger(t.toMicros(), "time");
						file.setString(t.toHttp<Interface>(), "date");
					}
				}
			}
		}

		req.runPug("virtual://html/reports.pug", [&] (pug::Context &exec, const pug::Template &tpl) -> bool {
			ServerGui::defineBasics(exec, req, u);
			if (paths) {
				exec.set("files", move(paths));
			} else if (file) {
				exec.set("file", move(file));
			}
			return true;
		});

		return DONE;
	} else {
		return makeUnavailablePage(req);
	}
}

}
