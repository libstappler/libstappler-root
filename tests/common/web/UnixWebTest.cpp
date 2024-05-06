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
#include "SPNetworkHandle.h"
#include "Test.h"

#if MODULE_STAPPLER_WEBSERVER_UNIX

#include "SPWebUnixRoot.h"
#include "SPWebUnixWebsocket.h"

#include "UnixWebTestWebsocket.cc"
#include "UnixWebTestComponent.cc"

#include <unistd.h>

namespace STAPPLER_VERSIONIZED stappler::app::test {

struct FileData {
	StringView name;
	StringView contentType;
	StringView path;
};

static StringView TEST_DATA(R"({"arr":["value1 test",["value2 test", "value3"]],"array space":["value6", "value7"],"dict":{"key space":"value5"},"text":"text"})");

static StringView TEST_URLENCODED("dict[key%20space]=value4&arr[0]=value1%20test&arr[1][]=value2%20test&arr[1][]=value3"
		"&dict[key%20space]=value5&array%20space[]=value6&array%20space[]=value7&text=text");

static StringView DATA_INDEX(R"(<!DOCTYPE html>
<html>
  <head>
    <meta charset="utf-8">
    <title>Header</title>
  </head>
  <body>
    Hello world
  </body>
</html>
)");

static StringView s_AuthName;
static StringView s_AuthPassword;

static Bytes makeMultipartRequest(const Value &val, const SpanView<FileData> &files, StringView boundary) {
	Bytes data;

	auto writer = [&] (StringView str) {
		data.insert(data.end(), (const uint8_t *)str.data(), (const uint8_t *)str.data() + str.size());
	};

	Callback<void(StringView)> stream(writer);

	for (auto &it : val.asDict()) {
		if (it.second.isArray()) {
			for (auto &iit : it.second.asArray()) {
				stream << "--" << boundary << "\r\n";
				stream << "Content-Disposition: form-data; name=" << it.first << "[]\r\n\r\n";
				stream << iit.asString() << "\r\n";
			}

			size_t i = 0;
			for (auto &iit : it.second.asArray()) {
				stream << "--" << boundary << "\r\n";
				stream << "Content-Disposition: form-data; name=" << it.first << "[" << i << "]\r\n\r\n";
				stream << iit.asString() << "\r\n";
				++ i;
			}
		} else if (it.second.isDictionary()) {
			for (auto &iit : it.second.asDict()) {
				stream << "--" << boundary << "\r\n";
				stream << "Content-Disposition: form-data; name=" << it.first << "[" << iit.first << "]\r\n\r\n";
				stream << iit.second.asString() << "\r\n";
			}
		} else {
			stream << "--" << boundary << "\r\n";
			stream << "Content-Disposition: form-data; name=" << it.first << "\r\n\r\n";
			stream << it.second.asString() << "\r\n";
		}
	}

	for (auto &it : files) {
		auto filedata = filesystem::readIntoMemory<Interface>(it.path);

		if (!filedata.empty()) {
			stream << "--" << boundary << "\r\n";
			stream << "Content-Disposition: form-data; name=\"" << it.name << "\"; filename=\"" << filepath::lastComponent(it.path) << "\"; "
					"size=" << filedata.size() << "; other=dummy\r\n";
			stream << "Content-Type: " << it.contentType << "\r\n";
			stream << "Content-Transfer-Encoding: binary\r\n";
			stream << "Content-Length: " << filedata.size() << "\r\n\r\n";
			data.insert(data.end(), filedata.begin(), filedata.end());
			stream << "\r\n";
		}
	}

	stream << "--" << boundary << "--\r\n";

	return data;
}

static Bytes makeFileMultipartRequest(StringView boundary) {
	Bytes data;

	auto writer = [&] (StringView str) {
		data.insert(data.end(), (const uint8_t *)str.data(), (const uint8_t *)str.data() + str.size());
	};

	Callback<void(StringView)> stream(writer);

	auto cborFile = filesystem::currentDir<Interface>("data/app.cbor");
	auto jsonFile = filesystem::currentDir<Interface>("data/app.json");

	stream << "--" << boundary << "\r\n";
	stream << "Content-Disposition: form-data; name=text\r\n\r\n";
	stream << valid::generatePassword<Interface>(16) << "\r\n";

	stream << "--" << boundary << "\r\n" << "Content-Disposition: form-data; name=arr[0]; dummy=\"dummy\"; val=\"test\r\n\r\nvalue1\r\n";
	stream << "--" << boundary << "\r\n" << "Content-Disposition: form-data; name=arr[1][]\r\n\r\nvalue2\r\n";
	stream << "--" << boundary << "\r\n" << "Content-Disposition: form-data; name=arr[1][]\r\n\r\nvalue3\r\n";
	stream << "--" << boundary << "\r\n" << "Content-Disposition: form-data; name=dict[key]\r\n\r\nvalue4\r\n";
	stream << "--" << boundary << "\r\n" << "Content-Disposition: form-data; name=dict[key]\r\n\r\nvalue5\r\n";

	auto cborData = filesystem::readIntoMemory<Interface>(cborFile);
	auto jsonData = filesystem::readIntoMemory<Interface>(jsonFile);

	stream << "--" << boundary << "\r\n";
	stream << "Content-Disposition: form-data; name=\"data\"; filename=\"" << filepath::lastComponent(cborFile) << "\"; "
			"size=" << cborData.size() << "\r\n";
	stream << "Content-Type: application/cbor\r\n";
	stream << "Content-Transfer-Encoding: binary\r\n\r\n";
	data.insert(data.end(), cborData.begin(), cborData.end());
	stream << "\r\n";

	stream << "--" << boundary << "\r\n";
	stream << "Content-Disposition: form-data; name=\"file\"; filename=\"" << filepath::lastComponent(jsonFile) << "\"; "
			"size=" << jsonData.size() << "\r\n";
	stream << "Content-Type: application/json\r\n";
	stream << "Content-Transfer-Encoding: binary\r\n\r\n";
	data.insert(data.end(), jsonData.begin(), jsonData.end());
	stream << "\r\n";

	stream << "--" << boundary << "--\r\n";

	return data;
}
static Bytes makeFileMultipartRequest2(StringView boundary) {
	Bytes data;

	auto writer = [&] (StringView str) {
		data.insert(data.end(), (const uint8_t *)str.data(), (const uint8_t *)str.data() + str.size());
	};

	Callback<void(StringView)> stream(writer);

	auto cborFile = filesystem::currentDir<Interface>("data/app.cbor");
	auto jsonFile = filesystem::currentDir<Interface>("data/app.json");
	auto cborData = filesystem::readIntoMemory<Interface>(cborFile);
	auto jsonData = filesystem::readIntoMemory<Interface>(jsonFile);

	stream << "--" << boundary << "\r\n";
	stream << "Content-Disposition: form-data; name=\"binaryFile\"; filename=\"" << filepath::lastComponent(cborFile) << "\"; "
			"size=" << cborData.size() << "\r\n";
	stream << "Content-Type: application/cbor\r\n";
	stream << "Content-Transfer-Encoding: binary\r\n\r\n";
	data.insert(data.end(), cborData.begin(), cborData.end());
	stream << "\r\n";

	stream << "--" << boundary << "\r\n";
	stream << "Content-Disposition: form-data; name=\"textFile\"; filename=\"" << filepath::lastComponent(jsonFile) << "\"; "
			"size=" << jsonData.size() << "\r\n";
	stream << "Content-Type: application/json\r\n";
	stream << "Content-Transfer-Encoding: binary\r\n\r\n";
	data.insert(data.end(), jsonData.begin(), jsonData.end());
	stream << "\r\n";

	stream << "--" << boundary << "--\r\n";

	return data;
}

static Bytes performFileQuery(bool auth, NetworkHandle::Method m, StringView url, BytesView data = BytesView(), StringView contentType = StringView()) {
	Bytes out;

	NetworkHandle h;
	h.init(m, url);
	if (!data.empty()) {
		h.setSendData(data, contentType);
	}

	h.setReceiveCallback([&] (char *data, size_t size) {
		auto sourceSize = out.size();
		out.resize(sourceSize + size);
		memcpy(out.data() + sourceSize, data, size);
		return size;
	});

	auto cookieFile = filesystem::currentDir<Interface>("web/root/cookie.cookie");
	h.setCookieFile(cookieFile);

	if (auth && !s_AuthName.empty()) {
		h.setAuthority(s_AuthName, s_AuthPassword);
	}
	h.perform();

	return out;
}

static Bytes performFileQuery(NetworkHandle::Method m, StringView url, const Value & data) {
	Bytes out;

	NetworkHandle h;
	h.init(m, url);
	if (!data.empty()) {
		h.setSendData(data);
	}

	h.setCookieFile(filesystem::currentDir<Interface>("web/root/cookie.cookie"));

	h.setReceiveCallback([&] (char *data, size_t size) {
		auto sourceSize = out.size();
		out.resize(sourceSize + size);
		memcpy(out.data() + sourceSize, data, size);
		return size;
	});
	h.perform();

	return out;
}

static Value performQuery(NetworkHandle::Method m, StringView url, BytesView data = BytesView(), StringView contentType = StringView()) {
	StringStream out;

	NetworkHandle h;
	h.init(m, url);
	if (!data.empty()) {
		h.setSendData(data, contentType);
	}

	h.setReceiveCallback([&] (char *data, size_t size) {
		out << StringView(data, size);
		return size;
	});
	h.perform();

	return data::read<Interface>(out.str());
}

static Value performQuery(NetworkHandle::Method m, StringView url, const Value &data) {
	StringStream out;

	NetworkHandle h;
	h.init(m, url);
	if (!data.empty()) {
		h.setSendData(data);
	}

	h.setReceiveCallback([&] (char *data, size_t size) {
		out << StringView(data, size);
		return size;
	});
	h.perform();

	return data::read<Interface>(out.str());
}

struct UnixWebTest : Test {
	UnixWebTest() : Test("UnixWebTest") { }

	bool perfromIndexTest(StringView rootPath) {
		auto d = base16::encode<Interface>(valid::makeRandomBytes<Interface>(8));

		StringStream out;

		NetworkHandle h;
		h.init(NetworkHandle::Method::Get, toString("http://localhost:23001/index.html?data=", d));
		h.setReceiveCallback([&] (char *data, size_t size) {
			out << StringView(data, size);
			return size;
		});
		h.perform();

		auto fileData = filesystem::readIntoMemory<Interface>(filepath::merge<Interface>(rootPath, "index.html"));

		if (BytesView(fileData) == BytesView(out.str())) {
			return true;
		}

		return false;
	}

	Value performMapTest() {
		StringStream out;
		auto boundary = toString("---------------", base64::encode<Interface>(valid::makeRandomBytes<Interface>(16)));
		auto data = makeFileMultipartRequest(boundary);

		return performQuery(NetworkHandle::Method::Post, toString("http://localhost:23001/map/123/page?intValue=1234"),
				data, toString("multipart/form-data; boundary=", boundary));
	}

	Value performFileTest() {
		return performQuery(NetworkHandle::Method::Post, toString("http://localhost:23001/map/files"),
				BytesView(DATA_INDEX), "text/html");
	}

	Value performUrlencodedTest() {
		return performQuery(NetworkHandle::Method::Post, toString("http://localhost:23001/map/urlencoded"),
				BytesView(TEST_URLENCODED), "application/x-www-form-urlencoded");
	}

	Value performSerenityTest() {
		return performQuery(NetworkHandle::Method::Post, toString("http://localhost:23001/map/urlencoded"),
				BytesView("(dict(key%20space:value5);arr:value1%20test,~(value2%20test,value3);text:text;array%20space:value6,value7)"),
				"application/x-serenity-urlencoded");
	}

	Value createRef(const Value &val) {
		StringStream out;

		auto boundary = toString("---------------", base64::encode<Interface>(valid::makeRandomBytes<Interface>(16)));
		auto path = filesystem::currentDir<Interface>("resources/xenolith-2-480.png");

		FileData file {
			StringView("cover"),
			StringView("image/png"),
			StringView(path)
		};

		auto data = makeMultipartRequest(val, makeSpanView(&file, 1), boundary);
		auto ref = performQuery(NetworkHandle::Method::Post, toString("http://localhost:23001/refs"),
				data, toString("multipart/form-data; boundary=", boundary)).getValue("result");

		auto imageData = filesystem::readIntoMemory<Interface>(path);
		auto fileData = performFileQuery(false, NetworkHandle::Method::Get, toString("http://localhost:23001/refs/id", ref.getInteger("__oid"), "/cover"));

		if (imageData != fileData) {
			return Value(false);
		}

		return ref;
	}

	Value createRef2(const Value &val) {
		auto ref = performQuery(NetworkHandle::Method::Post, toString("http://localhost:23001/refs"), val).getValue("result");

		Value subobject({
			pair("text", Value("Sub1")),
			pair("index", Value(1))
		});

		Value subobject2({
			pair("text", Value("Sub2")),
			pair("index", Value(2))
		});

		Value subobject3({
			pair("text", Value("Sub2")),
			pair("index", Value(2))
		});

		auto url = toString("http://localhost:23001/refs/id", ref.getInteger("__oid"), "/subobject");

		auto obj = performQuery(NetworkHandle::Method::Post, url, subobject).getValue("result");
		obj = performQuery(NetworkHandle::Method::Get, url);
		obj = performQuery(NetworkHandle::Method::Post, url, subobject2).getValue("result");
		obj = performQuery(NetworkHandle::Method::Put, url, subobject3).getValue("result");

		std::cout << data::EncodeFormat::Pretty << obj << "\n";

		performQuery(NetworkHandle::Method::Delete, url);

		return ref;
	}

	void deleteRef(const Value &val) {
		NetworkHandle h;
		h.init(NetworkHandle::Method::Delete, toString("http://localhost:23001/refs/id", val.getInteger("__oid")));
		h.perform();
	}

	Value createObject(const Value &val) {
		auto ret = performQuery(NetworkHandle::Method::Post, toString("http://localhost:23001/objects"), val);
		auto obj = ret.getValue("result");
		auto objId = obj.getInteger("__oid");

		auto boundary = toString("---------------", base64::encode<Interface>(valid::makeRandomBytes<Interface>(16)));
		auto data = makeFileMultipartRequest2(boundary);

		performQuery(NetworkHandle::Method::Put, toString("http://localhost:23001/objects/id", objId),
				data, toString("multipart/form-data; boundary=", boundary));

		Value subobject({
			pair("text", Value("Sub1")),
			pair("index", Value(1))
		});

		performQuery(NetworkHandle::Method::Post, toString("http://localhost:23001/objects/id", objId, "/subobjects"),
				Value({ Value(subobject) }));
		//performQuery(NetworkHandle::Method::Put, toString("http://localhost:23001/objects/id", objId, "/subobjects"),
		//		Value({ Value(subobject) }));
		performQuery(NetworkHandle::Method::Post, toString("http://localhost:23001/objects/id", objId, "/subobjects?METHOD=PATCH"),
				Value({ Value(subobject) }));

		NetworkHandle h1;
		h1.init(NetworkHandle::Method::Head, toString("http://localhost:23001/objects/id", objId));
		h1.perform();

		NetworkHandle h3;
		h3.init(NetworkHandle::Method::Get, toString("http://localhost:23001/objects/id", obj.getInteger("__oid")));
		h3.addHeader("if-modified-since", Time::now().toHttp<Interface>());
		h3.perform();

		NetworkHandle h4;
		h4.init(NetworkHandle::Method::Get, "http://localhost:23001/objects");
		h4.addHeader("if-modified-since", Time::now().toHttp<Interface>());
		h4.perform();

		auto cborFile = filesystem::currentDir<Interface>("data/app.cbor");
		auto cborData = filesystem::readIntoMemory<Interface>(cborFile);

		performQuery(NetworkHandle::Method::Put, toString("http://localhost:23001/objects/id", obj.getInteger("__oid"), "/file"),
				cborData, "application/cbor");

		performQuery(NetworkHandle::Method::Post, toString("http://localhost:23001/objects/id", obj.getInteger("__oid"), "/file"),
				cborData, "application/cbor");

		performQuery(NetworkHandle::Method::Delete, toString("http://localhost:23001/objects/id", obj.getInteger("__oid"), "/file"));

		return obj;
	}

	void deleteObject(const Value &val) {
		NetworkHandle h;
		h.init(NetworkHandle::Method::Get, toString("http://localhost:23001/objects/id", val.getInteger("__oid"), "?METHOD=DELETE"));
		h.perform();
	}

	Value updateObject(const Value &orig, const Value &data) {
		return performQuery(NetworkHandle::Method::Put, toString("http://localhost:23001/objects/id", orig.getInteger("__oid")), data);
	}

	bool testResourceObjects() {
		auto obj = performQuery(NetworkHandle::Method::Get, "http://localhost:23001/categories/select/id/10/all_pages/limit/1/root").getValue("result");
		auto d = performQuery(NetworkHandle::Method::Get, "http://localhost:23001/categories/all/select/id/10").getValue("result").getValue(0);

		auto pages1 = performQuery(NetworkHandle::Method::Get, "http://localhost:23001/categories/select/id/10/all_pages").getValue("result");
		auto pages2 = performQuery(NetworkHandle::Method::Get, "http://localhost:23001/categories/select/id/10/pages").getValue("result");

		auto target = performQuery(NetworkHandle::Method::Get, "http://localhost:23001/objects/all/limit/1").getValue("result");
		performQuery(NetworkHandle::Method::Get, "http://localhost:23001/objects/all/limit/1/array").getValue("result");

		auto url = toString("http://localhost:23001/objects/id", target.getValue(0).getInteger("__oid"), "/array");

		Value arr({
			Value({
				pair("one", Value(1)),
				pair("two", Value(2)),
			}),
			Value({
				pair("one", Value(3)),
				pair("two", Value(4)),
			})
		});

		Value val({
			pair("one", Value(1)),
			pair("two", Value(2)),
		});

		performQuery(NetworkHandle::Method::Post, url, arr);
		performQuery(NetworkHandle::Method::Post, url, val);
		performQuery(NetworkHandle::Method::Put, url, arr);

		pages1.erase(2);

		return pages2 == pages1 && obj.getValue(0) == d;
	}

	bool testResourceAcquire() {
		auto testToken = [] (StringView beginStmt) {
			auto d = performQuery(NetworkHandle::Method::Get, toString("http://localhost:23001/test?(fields:key,index,time,value,secret;begin:", beginStmt, ")"));
			size_t results = d.getValue("result").size();
			while (d.getValue("cursor").hasValue("next")) {
				d = performQuery(NetworkHandle::Method::Get, toString("http://localhost:23001/test?(fields:key,index,time,value,secret;continue:", d.getValue("cursor").getString("next"), ")"));
				results += d.getValue("result").size();
			}
			return results;
		};

		auto testSelect = [] (StringView selString) {
			auto d = performQuery(NetworkHandle::Method::Get, toString("http://localhost:23001/test", selString));
			return d.getValue("result").size();
		};

		auto testHeadlines = [] (StringView config) {
			auto d = performQuery(NetworkHandle::Method::Get, toString("http://localhost:23001/test/search/tsv"
					"?(search:value15;headlines(", config, "))"));
			return d.getValue("result");
		};

		auto testObject = [] (StringView selString) {
			auto d = performQuery(NetworkHandle::Method::Get, toString("http://localhost:23001", selString));
			//std::cout << data::EncodeFormat::Pretty << d << "\n";
			return d.getValue("result").size();
		};

		size_t obj = testObject("/objects_root") + testObject("/objects_indexed")
				+ testObject("/objects_first") + testObject("/objects_named");

		if (obj != 4) {
			return false;
		}

		size_t sel = testSelect("/search/tsv?(search:value15)")
				+ testSelect("/id11")
				+ testSelect("/named-key10")
				+ testSelect("/select/key/key10")
				+ testSelect("/:select:key:key10")
				+ testSelect("/select/key/eq/key10/order/key/asc/10")
				+ testSelect("/select/index/20")
				+ testSelect("/select/flag/1/limit/1")
				+ testSelect("/select/flag/t/limit/1")
				+ testSelect("/select/flag/f/limit/1")
				+ testSelect("/select/index/lt/20/limit/2")
				+ testSelect("/select/index/gt/20/limit/2/offset/2")
				+ testSelect("/select/index/bw/20/30/order/key/desc")
				+ testSelect("/first/index/2")
				+ testSelect("/last/index/2")
				+ testSelect("/first/index")
				+ testSelect("/last/index")
				+ testSelect("/+index/2")
				+ testSelect("/-index/1/offset/2");

		if (sel != 32) {
			return false;
		}

		size_t results = testToken("10")
			+ testToken("__oid")
			+ testToken("index")
			+ testToken("10,desc")
			+ testToken("index,10,desc");
		if (results != 156) {
			return false;
		}

		testHeadlines("tsvData.html:html");
		testHeadlines("tsvData.html(type:html)");
		testHeadlines("tsvData.html(type:html;fragments:3;start:%3ci%3e;end:%3c/i%3e;fragStart:%3ci%3e;fragStop:%3c/i%3e;)");

		testHeadlines("tsvData.text:plain");
		testHeadlines("tsvData.text(type:plain)");
		testHeadlines("tsvData.text(type:plain;start:%3ci%3e;end:%3c/i%3e)");

		return true;
	}

	bool testResourceHandlers() {
		Value res1({
			pair("text", Value("Text1")),
			pair("index", Value(1)),
			pair("strings", Value({ Value("String1"), Value("String2") })),
			pair("array", Value({
				Value({
					pair("one", Value(1)),
					pair("two", Value(2)),
				}),
				Value({
					pair("one", Value(3)),
					pair("two", Value(4)),
				})
			})),
		});

		Value upd({
			pair("alias", Value("alias"))
		});

		Value refData({
			pair("alias", Value("reference")),
			pair("index", Value(2)),
			pair("array", Value({ Value("String1"), Value("String2") })),
			pair("data", Value({
				pair("one", Value(1)),
				pair("two", Value(2)),
			}))
		});

		Value ref2Data({
			pair("alias", Value("reference2")),
			pair("index", Value(4)),
			pair("array", Value({ Value("String1"), Value("String2") })),
			pair("data", Value({
				pair("one", Value(1)),
				pair("two", Value(2)),
			}))
		});

		auto obj = createObject(res1);

		refData.setInteger(obj.getInteger("__oid"), "objectRef");

		auto ref = createRef(refData);
		auto ref2 = createRef2(ref2Data);

		auto features = performQuery(NetworkHandle::Method::Post, toString("http://localhost:23001/refs/id", ref.getInteger("__oid"), "/features"),
				Value({
					Value({
						pair("text", Value("Subobject1")),
						pair("index", Value(4)),
					}),
					Value({
						pair("text", Value("Subobject2")),
						pair("index", Value(5)),
					})
				})).getValue("result");

		auto optionals = performQuery(NetworkHandle::Method::Post, toString("http://localhost:23001/refs/id", ref.getInteger("__oid"), "/optionals"),
				Value({
					Value(features.getValue(0).getInteger("__oid")),
					Value(features.getValue(1).getInteger("__oid"))
				})).getValue("result");

		if (features != optionals) {
			return false;
		}

		optionals = performQuery(NetworkHandle::Method::Post, toString("http://localhost:23001/refs/id", ref.getInteger("__oid"), "/optionals"),
				Value(features.getValue(1).getInteger("__oid"))).getValue("result");

		features = performQuery(NetworkHandle::Method::Get, toString("http://localhost:23001/refs/id", ref.getInteger("__oid"), "/features"));

		features = performQuery(NetworkHandle::Method::Post, toString("http://localhost:23001/refs/id", ref.getInteger("__oid"), "/features?METHOD=PATCH"),
				Value({
					Value({
						pair("text", Value("Subobject1")),
						pair("index", Value(4)),
					}),
					Value({
						pair("text", Value("Subobject2")),
						pair("index", Value(5)),
					})
				})).getValue("result");

		features = performQuery(NetworkHandle::Method::Put, toString("http://localhost:23001/refs/id", ref.getInteger("__oid"), "/features?METHOD=PATCH"),
				Value({
					Value({
						pair("text", Value("Subobject1")),
						pair("index", Value(4)),
					}),
					Value({
						pair("text", Value("Subobject2")),
						pair("index", Value(5)),
					})
				})).getValue("result");

		performQuery(NetworkHandle::Method::Delete, toString("http://localhost:23001/refs/id", ref.getInteger("__oid"), "/features"));

		performQuery(NetworkHandle::Method::Get, toString("http://localhost:23001/refs/id", ref.getInteger("__oid"), "/objectRef"));
		//std::cout << data::EncodeFormat::Pretty << objRef << "\n";

		performQuery(NetworkHandle::Method::Get, toString("http://localhost:23001/refs/id", ref.getInteger("__oid"),
				"?(fields($basics;$files;data:two;features(text)optionals($ids)))"));

		auto objs = performQuery(NetworkHandle::Method::Get, toString("http://localhost:23001/multi?",
				"(objects/id", obj.getInteger("__oid"), "/refs(limit:1);refs/all(limit:1);delta:1)"));

		std::cout << data::EncodeFormat::Pretty << objs << "\n";

		obj = updateObject(obj, upd);

		deleteRef(ref);
		deleteRef(ref2);
		deleteObject(obj);

		return true;
	}

	virtual bool testTools() {
		auto content = performFileQuery(false, NetworkHandle::Method::Get, "http://localhost:23001/__server");
		performFileQuery(false, NetworkHandle::Method::Get, "http://localhost:23001/__server/virtual/css/style.css");
		performFileQuery(false, NetworkHandle::Method::Get, "http://localhost:23001/__server/virtual/js/shell.js");
		performFileQuery(false, NetworkHandle::Method::Get, "http://localhost:23001/__server/virtual/html/index.html");
		performFileQuery(false, NetworkHandle::Method::Get, "http://localhost:23001/__server/handlers");

		performFileQuery(NetworkHandle::Method::Post, "http://localhost:23001/__server", Value({
			pair("name", Value("stappler")),
			pair("passwd", Value("stappler")),
		}));

		s_AuthName = StringView("stappler");
		s_AuthPassword = StringView("stappler");

		performFileQuery(true, NetworkHandle::Method::Get, "http://localhost:23001/__server");
		performFileQuery(true, NetworkHandle::Method::Get, "http://localhost:23001/__server/shell");
		performFileQuery(true, NetworkHandle::Method::Get, "http://localhost:23001/__server/auth/basic?redirect=/");

		auto data = data::read<Interface>(performFileQuery(false, NetworkHandle::Method::Get,
				toString("http://localhost:23001/__server/auth/login?name=", s_AuthName, "&passwd=", s_AuthPassword, "&maxAge=1000&userdata=true")));

		performFileQuery(true, NetworkHandle::Method::Get, "http://localhost:23001/__server/auth/basic");

		auto token = data.getString("token");

		performFileQuery(false, NetworkHandle::Method::Get,
				toString("http://localhost:23001/__server/auth/touch?token=", token));
		data = data::read<Interface>(performFileQuery(false, NetworkHandle::Method::Get,
						toString("http://localhost:23001/__server/auth/update?token=", token, "&maxAge=1000&userdata=true")));
		token = data.getString("token");

		performFileQuery(false, NetworkHandle::Method::Get,
				toString("http://localhost:23001/__server/auth/cancel?token=", token));
		performFileQuery(true, NetworkHandle::Method::Delete,
				toString("http://localhost:23001/users/id", data.getInteger("userId")));

		performFileQuery(false, NetworkHandle::Method::Get,
				toString("http://localhost:23001/__server/auth/touch?token=notoken"));
		performFileQuery(false, NetworkHandle::Method::Get,
				toString("http://localhost:23001/__server/auth/login?name=noname&passwd=nopasswd"));

		performQuery(NetworkHandle::Method::Get, toString("http://localhost:23001/__server/auth/setup?name=", s_AuthName, "&passwd=", s_AuthPassword));

		performFileQuery(true, NetworkHandle::Method::Get, "http://localhost:23001/__server/errors");
		performFileQuery(true, NetworkHandle::Method::Get, "http://localhost:23001/__server/reports");
		performFileQuery(true, NetworkHandle::Method::Get, "http://localhost:23001/__server/handlers");

		Vector<String> paths;
		auto rootPath = filesystem::currentDir<Interface>("web/root/.reports");
		filesystem::ftw(rootPath, [&] (StringView path, bool isFile) {
			if (isFile) {
				performFileQuery(true, NetworkHandle::Method::Get, toString("http://localhost:23001/__server/reports/",
						filepath::lastComponent(path)));
				paths.emplace_back(path.str<Interface>());
			}
		});

		for (auto &it : paths) {
			performFileQuery(true, NetworkHandle::Method::Get, toString("http://localhost:23001/__server/reports/",
				filepath::lastComponent(it), "?remove=true"));
		}

		performFileQuery(true, NetworkHandle::Method::Get, toString("http://localhost:23001/__server/errors?delete=38&c=2dn3hmVfX29pZPYYGQACBw&tag=Auth"));

		return true;
	}

	virtual void testSocket(web::UnixRoot *root) {
		WebsocketSim sim;
		root->simulateWebsocket(&sim, "localhost", "/__server/shell?name=stappler&passwd=stappler");
		sim.wait();
	}

	virtual bool run() override {
		auto rootPath = filesystem::currentDir<Interface>("web/root");
		auto indexPath = filesystem::currentDir<Interface>("web/root/index.html");
		filesystem::remove(rootPath, true, true);
		filesystem::mkdir(rootPath);
		filesystem::write(indexPath, DATA_INDEX);

		auto sqlitePath = filepath::merge<Interface>(rootPath, "db.sqlite");

		web::UnixRoot::Config cfg;

		cfg.listen = StringView("127.0.0.1:23001");

		cfg.db = mem_pool::Value({
			pair("host", mem_pool::Value("localhost")),
			pair("dbname", mem_pool::Value("postgres")),
			pair("user", mem_pool::Value("stappler")),
			pair("password", mem_pool::Value("stappler")),
			pair("databases", mem_pool::Value({
				mem_pool::Value("stappler")
			})),
		});

		cfg.hosts.emplace_back(web::UnixHostConfig{
			.hastname = "localhost",
			.admin = "admin@stappler.org",
			.root = StringView(rootPath),
			.components = web::Vector<web::HostComponentInfo>{
				web::HostComponentInfo{
					.name = "TestComonent",
					.version = "0.1",
					.file = StringView(),
					.symbol = "CreateTestComponent",
					.data = mem_pool::Value({
						pair("test", mem_pool::Value("test")),
					})
				}
			},
			.db = mem_pool::Value({
				pair("host", mem_pool::Value("localhost")),
				pair("dbname", mem_pool::Value(sqlitePath)),
				pair("driver", mem_pool::Value("sqlite")),
				pair("threading", mem_pool::Value("serialized")),
				pair("cache", mem_pool::Value("shared")),
				pair("journal", mem_pool::Value("wal"))
			})
		});

		auto root = web::UnixRoot::create(move(cfg));

		::sleep(1);

		bool success = true;

		auto objSource = data::read<Interface>(TEST_DATA);
		auto objSource2 = data::readUrlencoded<Interface>(TEST_URLENCODED, 256);
		auto objInput1 = performSerenityTest().getValue("input");
		auto objInput2 = performUrlencodedTest().getValue("input");
		if (objSource != objInput1 || objInput2 != objInput1 || objInput2 != objSource2) {
			success = false;
		}

		if (!testTools()) {
			success = false;
		}

		testSocket(root);

		if (!testResourceObjects()) {
			success = false;
		}

		if (!testResourceAcquire()) {
			success = false;
		}

		if (performFileTest().getString("body") != DATA_INDEX) {
			success = false;
		}

		performMapTest();

		testResourceHandlers();

		if (!perfromIndexTest(rootPath)) {
			success = false;
		}

		::sleep(1);

		root->cancel();
		root = nullptr;

		filesystem::remove(rootPath, true, true);

		return success;
	}
} _UnixWebTest;

}

#endif
