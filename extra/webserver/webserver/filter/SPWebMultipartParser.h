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

#ifndef EXTRA_WEBSERVER_WEBSERVER_FILTER_SPWEBMULTIPARTPARSER_H_
#define EXTRA_WEBSERVER_WEBSERVER_FILTER_SPWEBMULTIPARTPARSER_H_

#include "SPWebInputFilter.h"

namespace stappler::web {

class MultipartParser : public InputParser {
public:
	using Reader = StringView;

	MultipartParser(const db::InputConfig &, size_t, const StringView &);

	virtual void run(StringView data) override;
	virtual void finalize() override;

protected:
	enum class State {
		Begin,
		BeginBlock,
		HeaderLine,
		Data,
		End,
	};

	enum class Data {
		Var,
		File,
		FileAsData,
		Skip,
	};

	enum class Header {
		Begin,
		ContentDisposition,
		ContentDispositionParams,
		ContentDispositionName,
		ContentDispositionFileName,
		ContentDispositionSize,
		ContentDispositionUnknown,
		ContentType,
		ContentEncoding,
		Unknown,
	};

	enum class Literal {
		None,
		Plain,
		Quoted,
	};

	enum class VarState {
		Key,
		SubKey,
		SubKeyEnd,
		Value,
		End,
	};

	Value * flushVarName(Reader &r);
	void flushLiteral(Reader &r, bool quoted);
	void flushData(Reader &r);

	void readBegin(Reader &r);
	void readBlock(Reader &r);
	void readHeaderBegin(Reader &r);
	void readHeaderContentDisposition(Reader &r);
	void readHeaderContentDispositionParam(Reader &r);
	void readHeaderValue(Reader &r);
	void readHeaderDummy(Reader &r);

	void readPlainLiteral(Reader &r);
	void readQuotedLiteral(Reader &r);
	void readHeaderContentDispositionValue(Reader &r);
	void readHeaderContentDispositionDummy(Reader &r);
	void readHeader(Reader &r);
	void readData(Reader &r);

	auto flushString(StringView &r, Value *cur, VarState varState) -> Value *;

	String name;
	String file;
	String type;
	String encoding;
	size_t size = 0;

	State state = State::Begin;
	Header header = Header::Begin;
	Literal literal = Literal::None;
	Data data = Data::Skip;
	size_t match = 0;
	String boundary;
	BufferTemplate<Interface> buf;
	StringStream streamBuf;

	Value *target = nullptr;
};

}

#endif /* EXTRA_WEBSERVER_WEBSERVER_FILTER_SPWEBMULTIPARTPARSER_H_ */
