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

#include "SPWebOutput.h"
#include "SPDbFile.h"

namespace STAPPLER_VERSIONIZED stappler::web::output {

constexpr static const char * HTML_LOAD_BEGIN =
R"Html(<!doctype html>
<html><head><title>Serenity Pretty Data Dump</title>
	<link rel="stylesheet" href="/__server/virtual/css/style.css" />
	<link rel="stylesheet" href="/__server/virtual/css/kawaiJson.css" />
	<script src="/__server/virtual/js/kawaiJson.js"></script>
<script>function load(j) { KawaiJson(document.getElementById("content"), j); }
function init() {load()Html";

constexpr static const char * HTML_LOAD_END =
R"Html()}</script>
</head>)Html";

constexpr static const char * HTML_PRETTY =
R"Html(<body onload="init();">
	<div id="content" class="content"></div>
</body></html>)Html";

struct HtmlJsonEncoder {
	HtmlJsonEncoder(std::basic_ostream<char> &s, bool trackActions = false)
	: stream(&s), trackActions(trackActions) { }

	~HtmlJsonEncoder() { }

	void writeString(const String &str) {
		(*stream) << "<span class=\"quote\">\"</span>";
		for (auto i : str) {
			if (i == '\n') {
				(*stream) << "\\n";
			} else if (i == '\r') {
				(*stream) << "\\r";
			} else if (i == '\t') {
				(*stream) << "\\t";
			} else if (i == '\f') {
				(*stream) << "\\f";
			} else if (i == '\b') {
				(*stream) << "\\b";
			} else if (i == '\\') {
				(*stream) << "\\\\";
			} else if (i == '\"') {
				(*stream) << "\\\"";
			} else if (i == ' ') {
				(*stream) << ' ';
			} else if (i >= 0 && i <= 0x20) {
				(*stream) << "\\u";
				(*stream).fill('0');
				(*stream).width(4);
				(*stream) << std::hex << (int)i << std::dec;
				(*stream).width(1);
				(*stream).fill(' ');
			} else {
				(*stream) << i;
			}
		}
		(*stream) << "<span class=\"quote\">\"</span>";
		offsetted = false;
	}

	void write(nullptr_t) {
		(*stream) << "<span class=\"null\">null</span>";
		offsetted = false;
	}

	void write(bool value) {
		(*stream) << "<span class=\"bool\">" << ((value)?"true":"false") << "</span>";
		offsetted = false;
	}

	void write(int64_t value) {
		(*stream) << "<span class=\"integer\">" << value << "</span>";
		offsetted = false;
	}

	void write(double value) {
		(*stream) << "<span class=\"float\">" << value << "</span>";
		offsetted = false;
	}

	void write(const String &str) {
		if (actionsState == Dict) {
			auto sep = str.find("|");
			if (sep == String::npos) {
				if (action == Delete) {
					(*stream) << " <a class=\"delete\" href=\"" << str << "\">Remove</a> ";
				} else {
					(*stream) << " <a class=\"edit\" href=\"" << str << "\">Edit</a> ";
				}
			} else {
				if (action == Delete) {
					(*stream) << " <a class=\"delete\" href=\"" << str.substr(sep + 1) << "\">Remove: " << str.substr(0, sep) << "</a> ";
				} else {
					(*stream) << " <a class=\"edit\" href=\"" << str.substr(sep + 1) << "\">Edit: " << str.substr(0, sep) << "</a> ";
				}
			}
		} else if (str.size() > 6 && str.compare(0, 2, "~~") == 0) {
			auto sep = str.find("|");
			if (sep == String::npos) {
				(*stream) << "<span class=\"string\">";
				writeString(str);
				(*stream) << "</span>";
			} else {
				(*stream) << "<a class=\"file\" target=\"_blank\" href=\"" << str.substr(sep + 1) << "\">file:" << str.substr(2, sep - 2) << "</a>";
			}
		} else {
			(*stream) << "<span class=\"string\">";
			writeString(str);
			(*stream) << "</span>";
		}
	}

	void write(const Bytes &data) {
		(*stream) << "<span class=\"bytes\">\"" << "BASE64:" << base64::encode<Interface>(data) << "\"</span>";
		offsetted = false;
	}

	bool isObjectArray(const Array &arr) {
		for (auto &it : arr) {
			if (!it.isDictionary()) {
				return false;
			}
		}
		return true;
	}

	void onBeginArray(const Array &arr) {
		(*stream) << '[';
		if (!isObjectArray(arr)) {
			++ depth;
			bstack.push_back(false);
			offsetted = false;
		} else {
			bstack.push_back(true);
		}
	}

	void onEndArray(const Array &arr) {
		if (!bstack.empty()) {
			if (!bstack.back()) {
				-- depth;
				(*stream) << '\n';
				for (size_t i = 0; i < depth; i++) {
					(*stream) << '\t';
				}
			}
			bstack.pop_back();
		} else {
			-- depth;
			(*stream) << '\n';
			for (size_t i = 0; i < depth; i++) {
				(*stream) << '\t';
			}
		}
		(*stream) << ']';
		popComplex = true;
	}

	void onBeginDict(const Dictionary &dict) {
		if (trackActions && actionsState == Key) {
			actionsState = Dict;
			(*stream) << "<span class=\"actions\">";
		} else {
			(*stream) << '{';
			++ depth;
		}
	}

	void onEndDict(const Dictionary &dict) {
		if (actionsState == Dict) {
			actionsState = None;
			(*stream) << "</span>";
			for (size_t i = 0; i < depth; i++) {
				(*stream) << '\t';
			}
		} else {
			-- depth;
			(*stream) << '\n';
			for (size_t i = 0; i < depth; i++) {
				(*stream) << '\t';
			}
			(*stream) << '}';
			popComplex = true;
		}
	}

	void onKey(const String &str) {
		if (actionsState == Dict) {
			if (str == "remove") {
				action = Delete;
			} else if (str == "edit") {
				action = Edit;
			}
		} else {
			(*stream) << '\n';
			for (size_t i = 0; i < depth; i++) {
				(*stream) << '\t';
			}
			if (trackActions && str == "~ACTIONS~") {
				actionsState = Key;
				//(*stream) << "<span class=\"key\">\"ACTIONS\"</span>";
			} else {
				(*stream) << "<span class=\"key\">";
				writeString(str);
				(*stream) << "</span>";
				(*stream) << ':';
				(*stream) << ' ';
			}
			offsetted = true;
		}

	}

	void onNextValue() {
		(*stream) << ',';
	}

	void onValue(const Value &val) {
		if (depth > 0) {
			if (popComplex && (val.isArray() || val.isDictionary())) {
				(*stream) << ' ';
			} else {
				if (!offsetted) {
					(*stream) << '\n';
					for (size_t i = 0; i < depth; i++) {
						(*stream) << '\t';
					}
					offsetted = true;
				}
			}
			popComplex = false;
		}
	}

	size_t depth = 0;
	bool popComplex = false;
	bool offsetted = false;
	Vector<bool> bstack;
	std::basic_ostream<char> *stream;
	bool trackActions = false;
	enum {
		None,
		Key,
		Dict
	} actionsState = None;
	enum {
		Delete,
		Edit,
	} action = Delete;
};

void formatJsonAsHtml(OutputStream &stream, const Value &data, bool actionHandling) {
	HtmlJsonEncoder enc(stream, actionHandling);
	data.encode(enc);
}

static void writeToRequest(Request &rctx, std::basic_ostream<char> &stream, const Value &data, bool trackActions) {
	stream << HTML_LOAD_BEGIN;
	data::write(stream, data, data::EncodeFormat::Json);
	stream << HTML_LOAD_END;
	if (!trackActions) {
		stream << HTML_PRETTY;
	} else if (trackActions) {
		auto host = rctx.host();
		auto &res = host.getResources();
		stream << "<body class=\"api\" onload=\"init();\">";
		if (!res.empty()) {
			stream << "<div class=\"sidebar\"><h3>Resources</h3><ul>";
			for (auto & it : res) {
				stream << "<li><a href=\"" << it.second.path << "?pretty=api\">" << it.first->getName() << "</a></li>";
			}
			stream << "</ul></div>";
		}

		auto &info = rctx.getInfo();
		stream << "<div class=\"content\"><h3>" << info.url.path;
		if (info.status >= 400) {
			stream << " <span class=\"error\">" << info.statusLine << "</span>";
		}
		stream << "</h3><p id=\"content\"></p></div>";

	}
}

void writeData(Request &rctx, const Value &data, bool allowJsonP) {
	Request r = rctx;
	writeData(rctx, rctx, [&] (const String &ct) {
		r.setContentType(String(ct));
	}, data, allowJsonP);
}

void writeData(Request &rctx, std::basic_ostream<char> &stream, const Function<void(const String &)> &ct,
		const Value &data, bool allowJsonP) {

	auto &info = rctx.getInfo();
	bool allowCbor = rctx.getController()->isAcceptable("application/cbor") > 0.0f;
	auto pretty = info.queryData.getValue("pretty");

	if (allowCbor) {
		ct("application/cbor"_weak);
		stream << data::EncodeFormat::Cbor << data;
		stream.flush();
		return;
	}

	if (allowJsonP) {
		if (!info.queryData.empty()) {
			String obj;
			if (info.queryData.isString("callback")) {
				obj = info.queryData.getString("callback");
			} else if (info.queryData.isString("jsonp")) {
				obj = info.queryData.getString("jsonp");
			}
			if (!obj.empty()) {
				ct("application/javascript;charset=UTF-8");
				stream << obj <<  "(" << (pretty?data::EncodeFormat::Pretty:data::EncodeFormat::Json) << data << ");\r\n";
				stream.flush();
				return;
			}
		}
	}

	if (pretty.isString() && (pretty.getString() == "html" || pretty.getString() == "api")) {
		ct("text/html;charset=UTF-8");
		writeToRequest(rctx, stream, data, pretty.getString() == "api");
	} else {
		if (pretty.isString() && pretty.getString() == "time") {
			ct("application/json;charset=UTF-8");
			stream << data::EncodeFormat::PrettyTime << data << "\r\n";
		} else {
			ct("application/json;charset=UTF-8");
			stream << (pretty.asBool()?data::EncodeFormat::Pretty:data::EncodeFormat::Json) << data << "\r\n";
		}
	}
	stream.flush();
}

Status writeResourceFileData(Request &rctx, Value &&result) {
	Value file(result.isArray()?move(result.getValue(0)):move(result));
	auto path = db::File::getFilesystemPath(rctx.host().getRoot(), uint64_t(file.getInteger("__oid")));

	auto &info = rctx.getInfo();
	if (info.queryData.getBool("stat")) {
		file.setBool(filesystem::exists(path), "exists");
		return writeResourceData(rctx, move(file), Value());
	}

	auto &loc = file.getString("location");
	if (filesystem::exists(path) && loc.empty()) {
		if (!output::writeFileHeaders(rctx, file)) {
			return HTTP_NOT_MODIFIED;
		}

		rctx.setFilename(std::move(path));
		return OK;
	}

	if (!loc.empty()) {
		rctx.setFilename(nullptr);
		return rctx.redirectTo(std::move(loc));
	}

	return HTTP_NOT_FOUND;
}

Status writeResourceData(Request &rctx, Value &&result, Value && origin) {
	Value data(move(origin));

	data.setInteger(Time::now().toMicros(), "date");
#if DEBUG
	auto &debug = rctx.getDebugMessages();
	if (!debug.empty()) {
		data.setArray(debug, "debug");
	}
#endif
	auto &error = rctx.getErrorMessages();
	if (!error.empty()) {
		data.setArray(error, "errors");
	}

	data.setValue(move(result), "result");
	data.setBool(true, "OK");
	rctx.writeData(data, true);

	return DONE;
}

Status writeResourceFileHeader(Request &rctx, const Value &result) {
	Value file(result.isArray()?std::move(result.getValue(0)):std::move(result));

	if (!file) {
		return HTTP_NOT_FOUND;
	}

	auto path = db::File::getFilesystemPath(rctx.host().getRoot(), uint64_t(file.getInteger("__oid")));
	auto &loc = file.getString("location");

	if (!filesystem::exists(path) && loc.empty()) {
		return HTTP_NOT_FOUND;
	}

	if (!writeFileHeaders(rctx, file)) {
		return HTTP_NOT_MODIFIED;
	}

	return DONE;
}

bool writeFileHeaders(Request &rctx, const Value &file, const String &convertType) {
	auto path = db::File::getFilesystemPath(rctx.host().getRoot(), file.getInteger("__oid"));

	rctx.setFilename(path, true, file.getInteger("mtime"));

	auto &info = rctx.getInfo();
	auto mtime = info.stat.mtime;

	auto tag = rctx.getResponseHeader("etag");

	auto match = rctx.getRequestHeader("if-none-match");
	auto modified = rctx.getRequestHeader("if-modified-since");
	if (!match.empty() && !modified.empty()) {
		if (tag == match && Time::fromHttp(modified).toSeconds() >= mtime.toSeconds()) {
			return false;
		}
	} else if (!match.empty() && tag == match) {
		return false;
	} else if (!modified.empty() && Time::fromHttp(modified).toSeconds() >= mtime.toSeconds()) {
		return false;
	}

	rctx.setResponseHeader("X-FileModificationTime", toString(mtime.toMicros()));

	if (file.isString("location")) {
		rctx.setResponseHeader("X-FileLocation", file.getString("location"));
	}
	if (!convertType.empty()) {
		rctx.setContentType(String(convertType));
	} else {
		rctx.setResponseHeader("X-FileSize", toString(file.getInteger("size")));
		if (info.headerRequest) {
			rctx.setResponseHeader("Content-Length", toString(info.stat.size));
		}
		rctx.setContentType(String(file.getString("type")));
	}
	return true;

}

String makeEtag(uint32_t idHash, Time mtime) {
	auto time = mtime.toMicroseconds();
	Bytes etagData; etagData.resize(12);
	memcpy(etagData.data(), (const void *)&idHash, sizeof(uint32_t));
	memcpy(etagData.data() + 4, (const void *)&time, sizeof(int64_t));

	return toString('"', base64::encode<Interface>(etagData), '"');
}

bool checkCacheHeaders(Request &rctx, Time t, const StringView &etag) {
	rctx.setResponseHeader("ETag", etag.str<Interface>());
	rctx.setResponseHeader("Last-Modified", t.toHttp<Interface>());

	auto match = rctx.getRequestHeader("if-none-match");
	auto modified = rctx.getRequestHeader("if-modified-since");
	if (!match.empty() && !modified.empty()) {
		if (etag == match && Time::fromHttp(modified).toSeconds() >= t.toSeconds()) {
			return true;
		}
	} else if (!match.empty() && etag == match) {
		return true;
	} else if (!modified.empty() && Time::fromHttp(modified).toSeconds() >= t.toSeconds()) {
		return true;
	}

	return false;
}

bool checkCacheHeaders(Request &rctx, Time t, uint32_t idHash) {
	return checkCacheHeaders(rctx, t, makeEtag(idHash, t));
}

}
