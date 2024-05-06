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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "stappler.h"

#include <uchar.h>
#include <string>

static void debug_print(const char *str) {
	stappler_string_t s;
	s.ptr = (uint8_t *)str;
	s.len = strlen(str);

	stappler_wasm_wasm_debug_print(&s);
}

static void debug_print(const std::string &str) {
	stappler_string_t s;
	s.ptr = (uint8_t *)str.data();
	s.len = str.size();

	stappler_wasm_wasm_debug_print(&s);
}

struct Data {
	const char *str8 = nullptr;
	const char16_t *str16 = nullptr;

	Data() {
		debug_print("TEST CTOR");
		str8 = "тЕСТ тЕСТ тЕСТ тЕСТ";
		str16 = u"тЕСТ тЕСТ тЕСТ тЕСТ";
	}

	~Data() {
		debug_print("TEST DTOR");
	}

	void print(char *ptr, size_t size) {
		stappler_string_t s;
		s.ptr = (uint8_t *)ptr;
		s.len = size;

		stappler_wasm_wasm_debug_print(&s);
	}
};

static const char *s_JsonData = R"({
	"cbor": "pWFhYUFhYmFCYWNhQ2FkYURhZWFF",
	"decoded": {
		"a": "A",
		"b": "B",
		"c": "C",
		"d": "D",
		"e": "E"
	},
	"hex": "a56161614161626142616361436164614461656145",
	"roundtrip": true
})";

static Data s_data;

using foreach_dict_fn = uint32_t (*) (void *ud, char *ptr, size_t size, stappler_wasm_data_borrow_value_t val);

static void foreach_dict(stappler_wasm_data_borrow_value_t val, foreach_dict_fn fn, void *arg) {
	stappler_wasm_data_method_value_foreach_dict(val, (uintptr_t)fn, (uintptr_t)arg);
}

template <typename T>
struct HandleTraits;

template <>
struct HandleTraits<stappler_wasm_data_own_value_t> {
	using Owned = stappler_wasm_data_own_value_t;
	using Borrowed = stappler_wasm_data_borrow_value_t;

	static constexpr auto Destructor = stappler_wasm_data_value_drop_own;
	static constexpr auto Borrowing = stappler_wasm_data_borrow_value;
};

template <typename T, typename Traits = HandleTraits<T>>
struct HandleBorrowed {
	HandleBorrowed(typename Traits::Borrowed &&t)
	: value(std::move(t)) { }

	typename Traits::Borrowed value;
};

template <typename T, typename Traits = HandleTraits<T>>
struct HandleOwned {
	~HandleOwned() {
		if (value.__handle > 0) {
			Traits::Destructor(value);
		}
	}

	HandleOwned(T &&t)
	: value(std::move(t)) { }

	HandleBorrowed<T> borrow() const {
		return HandleBorrowed<T>(Traits::Borrowing(value));
	}

	T value = T{-1};
};

using ValueOwned = HandleOwned<stappler_wasm_data_own_value_t>;

static void testJson() {
	stappler_list_u8_t buf { (uint8_t *)s_JsonData, strlen(s_JsonData) };
	stappler_string_t key { nullptr, 0 };

	auto handle = ValueOwned(stappler_wasm_data_read(&buf, &key));

	stappler_string_t val_set { (uint8_t *)"NewString", strlen("NewString") };
	stappler_string_t key_set { (uint8_t *)"string", strlen("string") };

	stappler_wasm_data_method_value_set_string_for_key(handle.borrow().value, &val_set, &key_set);

	foreach_dict(handle.borrow().value, [] (void *ud, char *ptr, size_t size, stappler_wasm_data_borrow_value_t val) -> uint32_t {
		stappler_string_t result { nullptr, 0 };

		stappler_wasm_data_method_value_to_string(val, 0, &result);

		debug_print("Foreach: " + std::string(ptr, size) + ": " + std::string((char *)result.ptr, result.len));

		stappler_string_free(&result);
		return 0;
	}, nullptr);
}

static std::string timeToHttp(uint64_t t) {
	char fmtBuf[30];
	stappler_string_t strFmt;
	strFmt.ptr = (uint8_t *)fmtBuf;
	strFmt.len = 30;
	strFmt.len = stappler_wasm_core_time_to_http(t, &strFmt);

	return std::string(fmtBuf, strFmt.len);
}

static std::string intToString(int64_t i) {
	auto len = stappler_wasm_core_itoa_len(i);
	char buf[len];

	stappler_string_t str;
	str.ptr = (uint8_t *)buf;
	str.len = len;

	str.len = stappler_wasm_core_itoa_u8(i, &str);
	return std::string((char *)str.ptr, str.len);
}

using ftw_fn = void (*) (void *ud, char *ptr, size_t size, bool isFile);

static void ftw(stappler_string_t *str, ftw_fn fn, void *arg, int32_t depth, bool dir_first) {
	stappler_wasm_filesystem_ftw(str, (uintptr_t)fn, (uintptr_t)arg, depth, dir_first);
}

static void testFilesystem() {
	stappler_string_t inStr { 0, 0 };
	stappler_string_t outStr { 0, 0 };

	stappler_wasm_filesystem_get_current_work_dir(&inStr, false, &outStr);
	stappler_wasm_wasm_debug_print(&outStr);

	ftw(&outStr, ftw_fn([] (void *ud, char *ptr, size_t size, bool isFile) {
		stappler_string_t path;
		path.ptr = (uint8_t *)ptr;
		path.len = size;

		Data *d = (Data *)ud;
		if (isFile) {
			auto str = "File: " + std::string(ptr, size);
			d->print(str.data(), str.size());

			stappler_wasm_filesystem_stat_rec_t stat;

			if (stappler_wasm_filesystem_stat(&path, &stat)) {
				str = "Ctime: " + timeToHttp(stat.ctime) + " Size: " + intToString(stat.size);
				d->print(str.data(), str.size());
			}
		} else {
			auto str = "Dir: " + std::string(ptr, size);
			d->print(str.data(), str.size());
		}
	}), &s_data, 1, true);

	stappler_string_free(&outStr);

	inStr.ptr = (uint8_t *)"app.wit";
	inStr.len = strlen("app.wit");

	stappler_wasm_filesystem_get_current_work_dir(&inStr, false, &outStr);
	stappler_wasm_wasm_debug_print(&outStr);

	auto file = stappler_wasm_filesystem_open(&outStr);
	if (file.__handle != -1) {
		auto borrow = stappler_wasm_filesystem_borrow_file(file);
		auto size = stappler_wasm_filesystem_method_file_size(borrow);

		std::string out = "File size: " + std::to_string(size);
		debug_print(out);

		stappler_list_u8_t buf;
		buf.ptr = new uint8_t[size];
		buf.len = size;

		if (stappler_wasm_filesystem_method_file_read(borrow, &buf) == size) {
			out.clear();
			out = "File content:\n" + std::string((const char *)buf.ptr, size) + "\n";
			debug_print(out);
		}

		stappler_wasm_filesystem_file_drop_own(file);
	}

	stappler_string_free(&outStr);
}

static void testTime() {
	auto time = stappler_wasm_core_time_now();
	auto timeLen = stappler_wasm_core_itoa_len(time);

	stappler_string_t str;
	char buf[timeLen];
	str.ptr = (uint8_t *)buf;
	str.len = timeLen;

	str.len = stappler_wasm_core_itoa_u8(time, &str);
	stappler_wasm_wasm_debug_print(&str);

	char fmtBuf[30];
	stappler_string_t strFmt;
	strFmt.ptr = (uint8_t *)fmtBuf;
	strFmt.len = 30;
	strFmt.len = stappler_wasm_core_time_to_http(time, &strFmt);

	stappler_wasm_wasm_debug_print(&strFmt);
}

static void testToUpper() {
	stappler_string_t inStr8View{ (uint8_t *)s_data.str8, strlen(s_data.str8) };
	stappler_string_t outStr8View;

	stappler_wasm_core_toupper_u8(&inStr8View, &outStr8View);

	std::string out = "toUpper: " + std::string((char *)outStr8View.ptr, outStr8View.len);
	stappler_string_free(&outStr8View);

	outStr8View.ptr = (uint8_t *)out.data();
	outStr8View.len = out.size();

	stappler_wasm_wasm_debug_print(&outStr8View);
}

static void testToLower() {
	stappler_string_t inStr8View{ (uint8_t *)s_data.str8, strlen(s_data.str8) };
	stappler_string_t outStr8View;

	stappler_wasm_core_tolower_u8(&inStr8View, &outStr8View);

	std::string out = "toLower: " + std::string((char *)outStr8View.ptr, outStr8View.len);
	stappler_string_free(&outStr8View);

	outStr8View.ptr = (uint8_t *)out.data();
	outStr8View.len = out.size();

	stappler_wasm_wasm_debug_print(&outStr8View);
}

static void testToTitle() {
	stappler_string_t inStr8View{ (uint8_t *)s_data.str8, strlen(s_data.str8) };
	stappler_string_t outStr8View;

	stappler_wasm_core_totitle_u8(&inStr8View, &outStr8View);

	std::string out = "toTitle: " + std::string((char *)outStr8View.ptr, outStr8View.len);
	stappler_string_free(&outStr8View);

	outStr8View.ptr = (uint8_t *)out.data();
	outStr8View.len = out.size();

	stappler_wasm_wasm_debug_print(&outStr8View);
}

extern "C"
bool exports_stappler_wasm_app_run(void) {
	//debug_print("Run");

	testJson();
	testFilesystem();
	testTime();

	testToUpper();
	testToLower();
	testToTitle();

	std::string inStr8 = s_data.str8;
	std::u16string inStr16 = s_data.str16;
	std::string outStr8;
	std::u16string outStr16;

	stappler_string_t inStr8View{ (uint8_t *)inStr8.data(), inStr8.size() };
	stappler_wasm_core_wide_string_t inStr16View{ (uint16_t *)inStr16.data(), inStr16.size() };

	stappler_wasm_core_wide_string_t outStr16View;
	stappler_string_t outStr8View;

	stappler_wasm_core_to_utf16(&inStr8View, &outStr16View);
	stappler_wasm_core_to_utf8(&inStr16View, &outStr8View);

	outStr8 = std::string((char *)outStr8View.ptr, outStr8View.len);
	outStr16 = std::u16string((char16_t *)outStr16View.ptr, outStr16View.len);

	stappler_wasm_core_wide_string_free(&outStr16View);
	stappler_string_free(&outStr8View);

	if (inStr8 == outStr8 && inStr16 == outStr16) {
		debug_print("Success");
		return true;
	}
	return false;
}
