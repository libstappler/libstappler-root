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

#ifndef EXTRA_WEBSERVER_WEBSERVER_FILTER_SPWEBINPUTFILTER_H_
#define EXTRA_WEBSERVER_WEBSERVER_FILTER_SPWEBINPUTFILTER_H_

#include "SPWebRequest.h"

namespace STAPPLER_VERSIONIZED stappler::web {

class SP_PUBLIC InputParser : public AllocBase {
public:
	InputParser(const db::InputConfig &, size_t);

	InputParser(const InputParser &) noexcept = delete;
	InputParser(InputParser &&) noexcept = delete;

	SP_COVERAGE_TRIVIAL
	virtual ~InputParser() { }

	virtual bool run(BytesView) = 0;
	virtual void finalize() = 0;
	virtual void cleanup();

	virtual Value &getData() {
		return root;
	}

	virtual Vector<db::InputFile> &getFiles() {
		return files;
	}

	const db::InputConfig &getConfig() const;

protected:
	db::InputConfig config;
	size_t length;
	Value root;
	//UrlencodeParser basicParser;
	Vector<db::InputFile> files;
};

class SP_PUBLIC InputFilter : public AllocBase {
public:
	using FilterFunc = Function<void(InputFilter *filter)>;
	using Accept = InputFilterAccept;

	enum class Exception {
		None,
		TooLarge,
		Unrecognized,
	};

	static Status getStatusForException(Exception);

	static Exception insert(const Request &);

	/* file index can be coded as ordered index or negative id
	 * negative_id = - index - 1
	 *
	 * return nullptr if there is no such file */
	static db::InputFile *getFileFromContext(int64_t);

	InputFilter(const Request &, Accept a);

	Status init();

	bool step(BytesView);
	void finalize();

	StringView getContentType() const;

	size_t getContentLength() const;
	size_t getBytesRead() const;
	size_t getBytesReadSinceUpdate() const;

	Time getStartTime() const;
	TimeInterval getElapsedTime() const;
	TimeInterval getElapsedTimeSinceUpdate() const;

	size_t getFileBufferSize() const;
	void setFileBufferSize(size_t size);

	bool isFileUploadAllowed() const;
	bool isDataParsingAllowed() const;
	bool isBodySavingAllowed() const;

	bool isCompleted() const;

	const StringStream & getBody() const;
	Value & getData();
	Vector<db::InputFile> &getFiles();

	db::InputFile * getInputFile(int64_t) const;

	const db::InputConfig & getConfig() const;

	Request getRequest() const;
	memory::pool_t *getPool() const;

protected:
	Accept _accept = Accept::None;

	Time _time;
	Time _startTime;
	TimeInterval _timer;

	size_t _unupdated = 0;
	size_t _read = 0;
	size_t _contentLength = 0;

	StringStream _body;

	InputParser *_parser = nullptr;
	Request _request;

	bool _eos = false, _isCompleted = false, _isStarted = false;
};

}

#endif /* EXTRA_WEBSERVER_WEBSERVER_FILTER_SPWEBINPUTFILTER_H_ */
