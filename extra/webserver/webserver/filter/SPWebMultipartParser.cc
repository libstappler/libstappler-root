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

#include "SPWebMultipartParser.h"
#include "SPValid.h"
#include "SPDbFile.h"

namespace STAPPLER_VERSIONIZED stappler::web {

MultipartParser::MultipartParser(const db::InputConfig &c, size_t s, const StringView &b)
: InputParser(c, s) {
	boundary.reserve(b.size() + 4);
	boundary.append("\r\n--");
	boundary.append(b.data(), b.size());
	match = 2;
}

Value * MultipartParser::flushVarName(StringView &r) {
	VarState cstate = VarState::Key;
	Value *current = nullptr;
	while (!r.empty()) {
		StringView str = r.readUntil<chars::Chars<char, '[', ']'>>();
		current = flushString(str, current, cstate);
		if (!current) {
			break;
		}
		if (!r.empty()) {
			switch (cstate) {
			case VarState::Key:
				switch (r[0]) {
				case '[': 			cstate = VarState::SubKey; break;
				default: 			cstate = VarState::End; break;
				}
				break;
			case VarState::SubKey:
				switch (r[0]) {
				case ']': 			cstate = VarState::SubKeyEnd; break;
				default: 			cstate = VarState::End; break;
				}
				break;
			case VarState::SubKeyEnd:
				switch (r[0]) {
				case '[': 			cstate = VarState::SubKey; break;
				default: 			cstate = VarState::End; break;
				}
				break;
			default:
				return nullptr;
				break;
			}
			++ r;
		}
	}
	return current;
}

void MultipartParser::flushLiteral(StringView &r, bool quoted) {
	auto tmp = r;
	if (!quoted) {
		tmp.trimChars<StringView::WhiteSpace>();
	}
	switch (header) {
	case Header::ContentDispositionFileName:
		file.assign(tmp.data(), tmp.size());
		break;
	case Header::ContentDispositionName:
		name.assign(tmp.data(), tmp.size());
		break;
	case Header::ContentDispositionSize:
		size = strtol(r.data(), nullptr, 10);
		break;
	case Header::ContentType:
		type.assign(tmp.data(), tmp.size());
		break;
	case Header::ContentEncoding:
		encoding.assign(tmp.data(), tmp.size());
		break;
	default:
		break;
	}
}

void MultipartParser::flushData(const BytesView &r) {
	switch (data) {
	case Data::File:
		if (r.size() + files.back().writeSize >= getConfig().maxFileSize) {
			files.back().close();
			files.pop_back();
			data = Data::Skip;
		} else {
			files.back().write((const char *)r.data(), r.size());
		}
		break;
	case Data::FileAsData:
		if (r.size() + streamBuf.size() >= getConfig().maxFileSize) {
			files.back().close();
			files.pop_back();
			data = Data::Skip;
		} else {
			streamBuf.write((const char *)r.data(), r.size());
		}
		break;
	case Data::Var:
		if (r.size() + buf.size() >= getConfig().maxVarSize) {
			buf.clear();
			data = Data::Skip;
		} else {
			buf.put(r.data(), r.size());
		}
		break;
	case Data::Skip:
		break;
	}
}

bool MultipartParser::readBegin(BytesView &r) {
	StringView tmp = r.toStringView();

	if (match == 0) {
		tmp.skipUntil<StringView::Chars<'-'>>();
	}
	while (match < boundary.length() && tmp.is(boundary.at(match))) {
		++ match; ++ tmp;
	}
	if (tmp.empty()) {
		r = BytesView((const uint8_t *)tmp.data(), r.size() - (tmp.data() - (const char *)r.data()));
		return true;
	} else if (match == boundary.length()) {
		state = State::BeginBlock;
		target = &root;
		match = 0;
	} else {
		match = 0;
		return false;
	}
	buf.clear();

	r = BytesView((const uint8_t *)tmp.data(), r.size() - (tmp.data() - (const char *)r.data()));
	return true;
}

void MultipartParser::readBlock(BytesView &r) {
	while (!r.empty()) {
		if (buf.size() == 0 && r.is('-')) {
			buf.putc(char(r[0]));
		} else if (buf.size() == 1 && buf.get().is('-') && r.is('-')) {
			state = State::End;
			return;
		} else {
			if (r.is('\n')) {
				state = State::HeaderLine;
				header = Header::Begin;
				name.clear();
				type.clear();
				encoding.clear();
				file.clear();
				size = 0;
				buf.clear();
				++ r;
				return;
			} else {
				++ r;
			}
		}
	}
}

void MultipartParser::readHeaderBegin(StringView &r) {
	StringView str = r.readUntil<StringView::Chars<'\n', ':'>>();
	if (r.is(':')) {
		StringView tmp;
		if (buf.empty()) {
			tmp = str;
		} else {
			buf.put(str.data(), str.size());
			tmp = buf.get();
		}

		tmp.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
		if (strncasecmp(tmp.data(), "Content-Disposition", "Content-Disposition"_len) == 0) {
			header = Header::ContentDisposition;
		} else if (strncasecmp(tmp.data(), "Content-Type", "Content-Type"_len) == 0) {
			header = Header::ContentType;
		} else if (strncasecmp(tmp.data(), "Content-Transfer-Encoding", "Content-Transfer-Encoding"_len) == 0) {
			header = Header::ContentEncoding;
		} else {
			header = Header::Unknown;
		}

		buf.clear();
		r ++;
	} else if (r.empty()) {
		buf.put(str.data(), str.size());
	} else if (r.is('\n')) {
		auto tmp = buf.get();
		str.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
		tmp.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
		if ((tmp.empty() || tmp.is('\r')) && (str.empty() || str.is('\r'))) {
			state = State::Data;
			if (!file.empty() || !type.empty()) {
				if ((config.required & db::InputConfig::Require::FilesAsData) != db::InputConfig::Require::None
						&& (size == 0 || size < getConfig().maxFileSize)
						&& db::InputConfig::isFileAsDataSupportedForType(type)) {
					streamBuf.clear();
					data = Data::FileAsData;
				} else if ((config.required & db::InputConfig::Require::Files) != 0
						&& (size == 0 || size < getConfig().maxFileSize)) {
					files.emplace_back(sp::move(name), sp::move(type), sp::move(encoding), sp::move(file), size, files.size());
					data = Data::File;
				} else {
					data = Data::Skip;
				}
			} else {
				if ((config.required & db::InputConfig::Require::Data) != 0 &&
						(size == 0 || size < getConfig().maxVarSize)) {
					data = Data::Var;
				} else {
					data = Data::Skip;
				}
			}

			name.empty();
			type.empty();
			encoding.empty();
			file.empty();
			size = 0;
		}
		header = Header::Begin; // next header
		buf.clear();
		r ++;
	}
}

void MultipartParser::readHeaderContentDisposition(StringView &r) {
	StringView str = r.readUntil<StringView::Chars<'\n', ';'>>();
	if (r.is(';')) {
		StringView tmp;
		if (buf.empty()) {
			tmp = str;
		} else {
			buf.put(str.data(), str.size());
			tmp = buf.get();
		}

		tmp.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
		if (strncasecmp(tmp.data(), "form-data", "form-data"_len) == 0) {
			header = Header::ContentDispositionParams;
		} else {
			header = Header::Unknown;
		}

		buf.clear();
		r ++;
	} else if (r.empty()) {
		buf.put(str.data(), str.size());
	} else if (r.is('\n')) {
		header = Header::Begin; // next header
		buf.clear();
		r ++;
	}
}

void MultipartParser::readHeaderContentDispositionParam(StringView &r) {
	if (buf.empty()) {
		r.skipChars<StringView::Chars<';', ' '>>();
	}
	StringView str = r.readUntil<StringView::Chars<'\n', '='>>();
	if (r.is('=')) {
		StringView tmp;
		if (buf.empty()) {
			tmp = str;
		} else {
			buf.put(str.data(), str.size());
			tmp = buf.get();
		}

		if (strncasecmp(tmp.data(), "name", "name"_len) == 0) {
			header = Header::ContentDispositionName;
			literal = Literal::None;
		} else if (strncasecmp(tmp.data(), "filename", "filename"_len) == 0) {
			header = Header::ContentDispositionFileName;
			literal = Literal::None;
		} else if (strncasecmp(tmp.data(), "size", "size"_len) == 0) {
			header = Header::ContentDispositionSize;
			literal = Literal::None;
		} else {
			header = Header::ContentDispositionUnknown;
		}

		buf.clear();
		r ++;
	} else if (r.empty()) {
		buf.put(str.data(), str.size());
	} else if (r.is('\n')) {
		header = Header::Begin; // next header
		buf.clear();
		r ++;
	}
}

void MultipartParser::readHeaderValue(StringView &r) {
	auto &max = getConfig().maxVarSize;
	StringView str = r.readUntil<StringView::Chars<'\n'>>();
	if (r.empty()) {
		if (buf.size() + str.size() < max) {
			buf.put(str.data(), str.size());
		} else {
			header = Header::Unknown; // skip processing
		}
	} else if (r.is('\n')) {
		StringView tmp;
		if (buf.empty()) {
			if (str.size() < max) {
				tmp = str;
			}
		} else {
			if (str.size() + buf.size() < max) {
				buf.put(str.data(), str.size());
				tmp = buf.get();
			}
		}

		if (!tmp.empty()) {
			flushLiteral(tmp, false);
		}

		header = Header::Begin; // next header
		literal = Literal::None;
		buf.clear();
		r ++;
	}
}

void MultipartParser::readHeaderDummy(StringView &r) {
	r.skipUntil<StringView::Chars<'\n'>>();
	if (r.is('\n')) {
		header = Header::Begin; // next header
		literal = Literal::None;
		buf.clear();
		r ++;
	}
}

void MultipartParser::readPlainLiteral(StringView &r) {
	auto &max = getConfig().maxVarSize;
	StringView str = r.readUntil<StringView::Chars<'\n', ';'>>();
	if (r.is(';') || r.is('\n')) {
		StringView tmp;
		if (buf.empty()) {
			if (str.size() < max) {
				tmp = str;
			} else {
				header = Header::ContentDispositionUnknown;
			}
		} else {
			if (str.size() + buf.size() < max) {
				buf.put(str.data(), str.size());
				tmp = buf.get();
			} else {
				header = Header::ContentDispositionUnknown;
			}
		}

		if (!tmp.empty()) {
			flushLiteral(tmp, false);
		}

		header = r.is(';') ? Header::ContentDispositionParams : Header::Begin;
		literal = Literal::None;
		buf.clear();
		r ++;
	} else if (r.empty()) {
		if (str.size() + buf.size() < max) {
			buf.put(str.data(), str.size());
		} else {
			header = Header::ContentDispositionUnknown;
		}
	}
}

void MultipartParser::readQuotedLiteral(StringView &r) {
	auto &max = getConfig().maxVarSize;
	StringView str = r.readUntil<StringView::Chars<'\n', '"'>>();
	if (r.is('"')) {
		StringView tmp;
		if (buf.empty()) {
			if (str.size() < max) {
				tmp = str;
			} else {
				header = Header::ContentDispositionUnknown;
			}
		} else {
			if (buf.size() + str.size() < max) {
				buf.put(str.data(), str.size());
				tmp = buf.get();
			} else {
				header = Header::ContentDispositionUnknown;
			}
		}

		flushLiteral(tmp, true);
		buf.clear();
		r ++;
		header = Header::ContentDispositionParams;
		literal = Literal::None;
	} else if (r.empty()) {
		if (buf.size() + str.size() < max) {
			buf.put(str.data(), str.size());
		} else {
			header = Header::ContentDispositionUnknown;
		}
	} else if (r.is('\n')) {
		header = Header::Begin; // next header
		literal = Literal::None;
		buf.clear();
		r ++;
	}
}

void MultipartParser::readHeaderContentDispositionValue(StringView &r) {
	switch (literal) {
	case Literal::None:
		if (r.is('"')) {
			literal = Literal::Quoted;
			r ++;
		} else if (r.is('\n')) {
			header = Header::Begin; // next header
			buf.clear();
			r ++;
		} else if (!r.is<StringView::CharGroup<CharGroupId::WhiteSpace>>()) {
			literal = Literal::Plain;
		} else {
			header = Header::ContentDispositionParams;
		}
		break;
	case Literal::Plain:
		readPlainLiteral(r);
		break;
	case Literal::Quoted:
		readQuotedLiteral(r);
		break;
	}
}

void MultipartParser::readHeaderContentDispositionDummy(StringView &r) {
	r.skipUntil<StringView::Chars<'\n', ';'>>();
	if (r.is(';')) {
		header = Header::ContentDispositionParams;
		literal = Literal::None;
		buf.clear();
		r ++;
	} else if (r.is('\n')) {
		header = Header::Begin; // next header
		literal = Literal::None;
		buf.clear();
		r ++;
	}
}

void MultipartParser::readHeader(BytesView &r) {
	StringView tmp = r.toStringView();

	switch (header) {
	case Header::Begin:
		readHeaderBegin(tmp);
		break;
	case Header::ContentDisposition:
		readHeaderContentDisposition(tmp);
		break;
	case Header::ContentDispositionParams:
		readHeaderContentDispositionParam(tmp);
		break;
	case Header::ContentDispositionName:
	case Header::ContentDispositionFileName:
	case Header::ContentDispositionSize:
		readHeaderContentDispositionValue(tmp);
		break;
	case Header::ContentDispositionUnknown:
		readHeaderContentDispositionDummy(tmp);
		break;
	case Header::ContentType:
	case Header::ContentEncoding:
		readHeaderValue(tmp);
		break;
	case Header::Unknown:
		readHeaderDummy(tmp);
		break;
	}

	r = BytesView((const uint8_t *)tmp.data(), r.size() - (tmp.data() - (const char *)r.data()));
}

void MultipartParser::readData(BytesView &r) {
	if (match == 0) {
		flushData(r.readUntil<uint8_t('\r')>());
		if (r.empty()) {
			return;
		} else {
			match = 1;
			r ++;
		}
	} else {
		while (!r.empty() && r[0] == boundary[match] && match < boundary.length()) {
			match ++;
			r ++;
		}

		if (match == boundary.length()) {
			state = State::BeginBlock;
			target = &root;
			if (data == Data::Var) {
				StringView tmp(name);
				auto current = flushVarName(tmp);
				if (current) {
					current->setString(buf.str());
				}
				buf.clear();
			} else if (data == Data::FileAsData) {
				root.setValue(data::read<Interface>(streamBuf.weak()), sp::move(name));
				streamBuf.clear();
			}
			match = 0;
		} else if (!r.empty() && r[0] != boundary[match]) {
			BytesView tmp((const uint8_t *)boundary.data(), match);
			flushData(tmp);
			match = 0;
		}
	}
}

bool MultipartParser::run(BytesView r) {
	while (!r.empty()) {
		switch (state) {
		case State::Begin: // skip preambula
			if (!readBegin(r)) {
				return false;
			}
			break;
		case State::BeginBlock: // wait for CRLF then headers or "--" then EOF
			readBlock(r);
			break;
		case State::HeaderLine:
			readHeader(r);
			break;
		case State::Data:
			readData(r);
			break;
		case State::End:
			return true;
			break;
		}
	}
	return true;
}

void MultipartParser::finalize() {

}

auto MultipartParser::flushString(StringView &r, Value *cur, VarState varState) -> Value * {
	auto str = string::urldecode<Interface>(r);

	switch (varState) {
	case VarState::Key:
		if (!str.empty()) {
			if (target->hasValue(str)) {
				cur = &target->getValue(str);
			} else {
				cur = &target->setValue(Value(true), str);
			}
		}
		break;
	case VarState::SubKey:
		if (cur) {
			if (!str.empty() && valid::validateNumber(str)) {
				auto num = StringView(str).readInteger().get();
				if (cur->isArray()) {
					if (num < int64_t(cur->size())) {
						cur = &cur->getValue(num);
						return cur;
					} else if (num == int64_t(cur->size())) {
						cur = &cur->addValue(Value(true));
						return cur;
					}
				} else if (!cur->isDictionary() && num == 0) {
					cur->setArray(typename Value::ArrayType());
					cur = &cur->addValue(Value(true));
					return cur;
				}
			}
			if (str.empty()) {
				if (!cur->isArray()) {
					cur->setArray(typename Value::ArrayType());
				}
				cur = &cur->addValue(Value(true));
			} else {
				if (!cur->isDictionary()) {
					cur->setDict(typename Value::DictionaryType());
				}
				if (cur->hasValue(str)) {
					cur = &cur->getValue(str);
				} else {
					cur = &cur->setValue(Value(true), str);
				}
			}
		}
		break;
	case VarState::Value:
	case VarState::End:
		if (cur) {
			if (!str.empty()) {
				cur->setString(str);
			}
			cur = nullptr;
		}
		break;
	default:
		break;
	}

	return cur;
}

}
