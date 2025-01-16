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

namespace STAPPLER_VERSIONIZED stappler::app::test {

class WebsocketSim : public web::UnixWebsocketSim {
public:
	void sendCommand(StringView str) {
		size_t dataSize = str.size();
		Bytes dataToSend;

		dataToSend.resize(web::WebsocketFrameWriter::getFrameSize(dataSize, true));

		auto offset = web::WebsocketFrameWriter::makeHeader(dataToSend.data(), dataSize, web::WebsocketFrameType::Text, true);
		memcpy(dataToSend.data() + offset, str.data(), dataSize);
		write(dataToSend);
	}

	void process(web::WebsocketFrameType t, BytesView bytes) {
		switch (t) {
		case web::WebsocketFrameType::Text:
			if (bytes.toStringView().starts_with("Serenity")) {

				send(Value({
					pair("message", Value(true)),
					pair("data", Value({
						pair("source", Value("Database-Query")),
						pair("test", Value("test"))
					}))
				}));
				send(Value({
					pair("message", Value(true)),
					pair("data", Value({
						pair("source", Value("custom data")),
						pair("test", Value("test"))
					}))
				}));
				send(Value({
					pair("event", Value("enter")),
					pair("user", Value("userdata"))
				}));
				send(Value({
					pair("event", Value("message")),
					pair("user", Value("userdata")),
					pair("message", Value("message"))
				}));

				sendCommand("help meta");
				sendCommand("help handlers");
				sendCommand("help history");
				sendCommand("help delta");
				sendCommand("help get");
				sendCommand("help multi");
				sendCommand("help create");
				sendCommand("help update");
				sendCommand("help append");
				sendCommand("help upload");
				sendCommand("help delete");
				sendCommand("help search");
				sendCommand("help debug");
				sendCommand("help exit");
				sendCommand("help echo");
				sendCommand("help parse");
				sendCommand("help msg");
				sendCommand("help count");
				sendCommand("help help");
				sendCommand("help generate_password");
				sendCommand("help time");
				sendCommand("help test");
				sendCommand("test test");
				sendCommand("echo test");
				sendCommand("debug on");
				sendCommand("debug");
				sendCommand("debug off");
				sendCommand("debug");
				sendCommand("meta");
				sendCommand("meta all");
				sendCommand("meta __users");
				sendCommand("get objects /first/index");
				sendCommand("multi (objects/id1/refs(limit:1);refs/all(limit:1);delta:1)");
				sendCommand("create objects /id1/array (one:10;two:12)");
				sendCommand("update objects /id1/array (one:10;two:12)");
				sendCommand("append objects /id1/array (one:10;two:12)");
				sendCommand("upload objects /id1/file");
				sendCommand("delete objects /id1/array");
				sendCommand("search test /search/tsv (search:value15;headlines(tsvData.text:plain)");
				sendCommand("search test /search/tsv value15");
				sendCommand("history objects");
				sendCommand("history objects::refs::1");
				sendCommand("delta objects 1");
				sendCommand("delta objects::refs::1 1");
				sendCommand("handlers");
				sendCommand("msg message");
				sendCommand("count");
				sendCommand("parse (search:value15;headlines(tsvData.text:plain)");
				sendCommand("generate_password 10");
				sendCommand("time");
				sendCommand("time 10sec");
				sendCommand("time 10ms");
				sendCommand("time 10mcs");
				sendCommand("echo close");
			} else if (bytes.toStringView().starts_with("stappler@localhost")) {
			} else if (bytes.toStringView().starts_with("<p id=")) {
				auto v = bytes.toStringView();
				v.readUntilString("/__server/shell");
				if (v.starts_with("/__server/shell")) {
					auto val = v.readUntil<StringView::Chars<'\''>>();
					auto cborFile = filesystem::currentDir<Interface>("data/app.cbor");
					auto cborData = filesystem::readIntoMemory<Interface>(cborFile);

					NetworkHandle h;
					h.init(NetworkHandle::Method::Post, toString("http://localhost:23001", val));
					h.setSendData(cborData, "application/cbor");
					h.setAuthority("stappler", "stappler");
					h.perform();
				}

				std::cout << "Upload: " << bytes.toStringView() << "\n";

			} else if (bytes.toStringView().starts_with("close")) {
				Bytes dataToSend;
				dataToSend.resize(web::WebsocketFrameWriter::getFrameSize(0, true));
				web::WebsocketFrameWriter::makeHeader(dataToSend.data(), 0, web::WebsocketFrameType::Close, true);
				write(dataToSend);
			} else {
				std::cout << "Read: " << bytes.toStringView() << "\n";
			}
			break;
		default:
			break;
		}
	}

	void wait() {
		_cancel.test_and_set();

		while (_cancel.test_and_set()) {
			std::unique_lock lock(_mutex);

			if (_bytes.empty()) {
				_cond.wait(lock);
			}

			auto bytes = sp::move(_bytes);
			_bytes.clear();
			_mutex.unlock();

			for (auto &it : bytes) {
				process(it.first, it.second);
			}
		}
	}

	virtual bool read(web::WebsocketFrameType t, const uint8_t *bytes, size_t count) {
		std::unique_lock lock(_mutex);
		_bytes.emplace_back(t, BytesView(bytes, count).bytes<Interface>());
		_cond.notify_one();
		return true;
	}

	virtual void onStarted() {
		std::cout << "onStarted\n";
	}

	virtual void onEnded() {
		_cancel.clear();
		_cond.notify_one();
		std::cout << "onEnded\n";
	}

protected:
	std::atomic_flag _cancel;
	std::atomic<size_t> _counter = 0;
	std::mutex _mutex;
	std::condition_variable _cond;
	Vector<Pair<web::WebsocketFrameType, Bytes>> _bytes;
};

}

#endif
