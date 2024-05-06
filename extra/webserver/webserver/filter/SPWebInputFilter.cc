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

#include "SPWebInputFilter.h"
#include "SPWebHostController.h"
#include "SPWebRequest.h"
#include "SPWebRequestController.h"
#include "SPWebRoot.h"
#include "SPWebMultipartParser.h"
#include "SPDbFile.h"

namespace STAPPLER_VERSIONIZED stappler::web {

class FileParser : public InputParser {
public:
	FileParser(const db::InputConfig &c, const StringView &ct, const StringView &name, size_t cl)
	: InputParser(c, cl) {
		if (cl < getConfig().maxFileSize) {
			db::InputFile fileObj(name.str<Interface>(), ct.str<Interface>(), String(), String(), cl, files.size());
			files.emplace_back(move(fileObj));
			file = &files.back();
		}
	}

	SP_COVERAGE_TRIVIAL
	virtual ~FileParser() { }

	virtual bool run(BytesView data) override {
		if (file && !skip) {
			if (file->writeSize + data.size() < getConfig().maxFileSize) {
				file->write((const char *)data.data(), data.size());
			} else {
				file->close();
				skip = true;
				return false;
			}
		}
		return true;
	}
	virtual void finalize() override { }

protected:
	bool skip = false;
	db::InputFile *file = nullptr;
};

class DataParser : public InputParser {
public:
	DataParser(const db::InputConfig &c, size_t s) : InputParser(c, s) { }

	SP_COVERAGE_TRIVIAL
	virtual ~DataParser() { }

	virtual bool run(BytesView data) override {
		stream.write((const char *)data.data(), data.size());
		return true;
	}
	virtual void finalize() override {
		root = data::read<Interface>(stream.weak());
	}

protected:
	StringStream stream;
};

class UrlEncodeParser : public InputParser {
public:
	using Reader = StringView;

	UrlEncodeParser(const db::InputConfig &cfg, size_t len)
	: InputParser(cfg, len) { }

	SP_COVERAGE_TRIVIAL
	virtual ~UrlEncodeParser() { }

	virtual bool run(BytesView data) override {
		stream.write((const char *)data.data(), data.size());
		return true;
	}
	virtual void finalize() override {
		root = data::readUrlencoded<Interface>(stream.weak());
	}

protected:
	StringStream stream;
};

InputParser::InputParser(const db::InputConfig &cfg, size_t len)
: config(cfg), length(len) { }

void InputParser::cleanup() {
	for (auto &it : files) {
		it.close();
	}
	files.clear();
}

const db::InputConfig &InputParser::getConfig() const {
	return config;
}

static InputFilter::Accept getAcceptedData(const Request &req, InputFilter::Exception &e) {
	Request r = req;
	auto &cfg = r.getInputConfig();
	InputFilter::Accept ret = InputFilter::Accept::None;

	size_t cl = 0;
	StringView ct;

	auto &info = req.getInfo();

	auto reportError = [&] (InputFilter::Exception ex, StringView info) SP_COVERAGE_TRIVIAL -> InputFilter::Accept {
		switch (ex) {
		case InputFilter::Exception::Unrecognized:
			req.addError("InputFilter", "No data to process", Value{
				std::make_pair("content", Value(ct)),
				std::make_pair("available", Value(info)),
			});
			break;
		case InputFilter::Exception::TooLarge:
			req.addError("InputFilter", "Request size is out of limits", Value{
				std::make_pair("length", Value(int64_t(cl))),
				std::make_pair("local", Value(int64_t(cfg.maxRequestSize))),
				std::make_pair("global", Value(int64_t(config::MAX_INPUT_POST_SIZE))),
			});
			break;
		default:
			break;
		}
		e = ex;
		return ret;
	};

	if (info.method == RequestMethod::Post || info.method == RequestMethod::Put || info.method == RequestMethod::Patch) {
		ct = r.getRequestHeader("Content-Type");
		auto b = r.getRequestHeader("Content-Length");

		if (!b.empty()) {
			cl = b.readInteger(10).get(0);
		}

		if (cl > config::MAX_INPUT_POST_SIZE || cl > cfg.maxRequestSize) {
			return reportError(InputFilter::Exception::TooLarge, StringView());
		}

		if (!ct.empty() && cl != 0 && cl < config::MAX_INPUT_POST_SIZE) {
			if ((cfg.required & db::InputConfig::Require::Data) != db::InputConfig::Require::None
					|| (cfg.required & db::InputConfig::Require::FilesAsData) != db::InputConfig::Require::None) {
				if (ct.starts_with("multipart/form-data; boundary=")) {
					ret = InputFilter::Accept::Multipart;
				} else if (ct.starts_with(data::MIME_JSON) || ct.starts_with(data::MIME_CBOR) || ct.starts_with(data::MIME_SERENITY)) {
					ret = InputFilter::Accept::Json;
				} else if (ct.starts_with(data::MIME_URLENCODED)) {
					ret = InputFilter::Accept::Urlencoded;
				}
			}
			if (ret == InputFilter::Accept::None) {
				if ((cfg.required & db::InputConfig::Require::Files) != db::InputConfig::Require::None
						|| (cfg.required & db::InputConfig::Require::Body) != db::InputConfig::Require::None) {
					if (ct.starts_with("multipart/form-data; boundary=")) {
						ret = InputFilter::Accept::Multipart;
					} else {
						ret = InputFilter::Accept::Files;
					}
				}
			}
		} else {
			e = InputFilter::Exception::Unrecognized;
		}
	}
	if (ret == InputFilter::Accept::None) {
		return reportError(InputFilter::Exception::Unrecognized, StringView("data, body"));
	}
	return ret;
}

db::InputFile *InputFilter::getFileFromContext(int64_t id) {
	auto req = Request::getCurrent();
	if (req) {
		auto f = req.getInputFilter();
		if (f) {
			return f->getInputFile(id);
		}
	}
	return nullptr;
}

SP_COVERAGE_TRIVIAL
Status InputFilter::getStatusForException(Exception ex) {
	switch (ex) {
	case InputFilter::Exception::TooLarge:
		return HTTP_REQUEST_ENTITY_TOO_LARGE;
		break;
	case InputFilter::Exception::Unrecognized:
		return HTTP_UNSUPPORTED_MEDIA_TYPE;
		break;
	default:
		break;
	}
	return HTTP_BAD_REQUEST;
}

InputFilter::Exception InputFilter::insert(const Request &r) {
	return perform([&] () -> InputFilter::Exception {
		Exception e = Exception::None;
		auto accept = getAcceptedData(r, e);
		if (accept == Accept::None) {
			return e;
		}

		auto f = r.config()->makeInputFilter(accept);
		Request(r).setInputFilter(f);
		return e;
	}, r.pool(), config::TAG_REQUEST, r.config());
}

InputFilter::InputFilter(const Request &r, Accept a) : _body() {
	_accept = a;
	_request = r;

	_isStarted = false;
	_isCompleted = false;

	auto b = _request.getRequestHeader("Content-Length");

	if (!b.empty()) {
		_contentLength = b.readInteger(10).get(0);
	}
}

Status InputFilter::init() {
	_startTime =_time = Time::now();
	_isStarted = true;

	if (_accept == Accept::Multipart) {
		auto ct = _request.getRequestHeader("Content-Type");
		ct.skipUntilString("boundary=");
		if (ct.starts_with("boundary=")) {
			_parser = new MultipartParser(_request.getInputConfig(), _contentLength,
					ct.sub("boundary="_len));
		}
	} else if (_accept == Accept::Urlencoded) {
		_parser = new UrlEncodeParser(_request.getInputConfig(), _contentLength);
	} else if (_accept == Accept::Json) {
		_parser = new DataParser(_request.getInputConfig(), _contentLength);
	} else if (_accept == Accept::Files) {
		const auto &ct = _request.getRequestHeader("Content-Type");
		const auto &name = _request.getRequestHeader("X-File-Name");

		_parser = new FileParser(_request.getInputConfig(), ct, name, _contentLength);
	}

	if (isBodySavingAllowed()) {
		_body.reserve(_contentLength);
	}

	if (_parser) {
		_request.config()->getHost()->getRoot()->handleFilterInit(this);
	}

	return OK;
}

bool InputFilter::step(BytesView data) {
	if (getConfig().required == db::InputConfig::Require::None || _accept == Accept::None) {
		return false;
	}

	if (_read + data.size() > _contentLength) {
		_unupdated += _contentLength - _read;
		_read = _contentLength;
	} else {
		_read += data.size();
		_unupdated += data.size();
	}

	auto t = Time::now();
	_timer = t - _time;
	_time = t;

	if (_timer > getConfig().updateTime || _unupdated > (_contentLength * getConfig().updateFrequency)) {
		_request.config()->getHost()->getRoot()->handleFilterUpdate(this);
		_timer = TimeInterval();
		_unupdated = 0;
	}

	if (isBodySavingAllowed()) {
		_body.write((const char *)data.data(), data.size());
	}

	if (_parser && (
			(_accept == Accept::Urlencoded && isDataParsingAllowed()) ||
			(_accept == Accept::Multipart && (isFileUploadAllowed() || isDataParsingAllowed()) ) ||
			(_accept == Accept::Json && isDataParsingAllowed()) ||
			(_accept == Accept::Files && isFileUploadAllowed()))) {
		if (!_parser->run(data)) {
			return false;
		}
	}
	return true;
}

void InputFilter::finalize() {
	if (_parser && (
			(_accept == Accept::Urlencoded && isDataParsingAllowed()) ||
			(_accept == Accept::Multipart && (isFileUploadAllowed() || isDataParsingAllowed()) ) ||
			(_accept == Accept::Json && isDataParsingAllowed()) ||
			(_accept == Accept::Files && isFileUploadAllowed()))) {
		_parser->finalize();
	}
	_eos = true;
	_isCompleted = true;
	_request.config()->getHost()->getRoot()->handleFilterComplete(this);
	if (_parser) {
		_parser->cleanup();
	}
}

size_t InputFilter::getContentLength() const {
	return _contentLength;
}
size_t InputFilter::getBytesRead() const {
	return _read;
}
size_t InputFilter::getBytesReadSinceUpdate() const {
	return _unupdated;
}

Time InputFilter::getStartTime() const {
	return _startTime;
}
TimeInterval InputFilter::getElapsedTime() const {
	return Time::now() - _startTime;
}
TimeInterval InputFilter::getElapsedTimeSinceUpdate() const {
	return Time::now() - _time;
}

bool InputFilter::isFileUploadAllowed() const {
	return (getConfig().required & db::InputConfig::Require::Files) != db::InputConfig::Require::None;
}
bool InputFilter::isDataParsingAllowed() const {
	return (getConfig().required & db::InputConfig::Require::Data) != db::InputConfig::Require::None;
}
bool InputFilter::isBodySavingAllowed() const {
	return (getConfig().required & db::InputConfig::Require::Body) != db::InputConfig::Require::None;
}

bool InputFilter::isCompleted() const {
	return _isCompleted;
}

const StringStream & InputFilter::getBody() const {
	return _body;
}
Value & InputFilter::getData() {
	return _parser->getData();
}
Vector<db::InputFile> &InputFilter::getFiles() {
	return _parser->getFiles();
}

db::InputFile * InputFilter::getInputFile(int64_t idx) const {
	if (idx < 0) {
		idx = -(idx + 1);
	}

	auto &files = _parser->getFiles();
	if (size_t(idx) >= files.size()) {
		return nullptr;
	}
	return &files.at(size_t(idx));
}

const db::InputConfig & InputFilter::getConfig() const {
	return _request.getInputConfig();
}

Request InputFilter::getRequest() const {
	return _request;
}

memory::pool_t *InputFilter::getPool() const {
	return _request.pool();
}

}
