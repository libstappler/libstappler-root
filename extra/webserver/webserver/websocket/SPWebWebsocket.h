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

#ifndef EXTRA_WEBSERVER_WEBSERVER_WEBSOCKET_SPWEBWEBSOCKET_H_
#define EXTRA_WEBSERVER_WEBSERVER_WEBSOCKET_SPWEBWEBSOCKET_H_

#include "SPWeb.h"
#include "SPWebRequest.h"
#include "SPWebHost.h"
#include "SPBuffer.h"

namespace STAPPLER_VERSIONIZED stappler::web {

class WebsocketManager;
class WebsocketHandler;

enum class WebsocketFrameType : uint8_t {
	None,

	// User
	Text,
	Binary,

	// System
	Continue,
	Close,
	Ping,
	Pong,
};

enum class WebsocketStatusCode : uint16_t {
	None = 0,
	Auto = 1,
	Ok = 1000,
	Away = 1001,
	ProtocolError = 1002,
	NotAcceptable = 1003,
	ExpectStatus = 1005,
	AbnormalClose = 1006,
	NotConsistent = 1007,
	PolicyViolated = 1008,
	TooLarge = 1009,
	NotNegotiated = 1010,
	UnexceptedCondition = 1011,
	SSLError = 1015,
};


struct SP_PUBLIC WebsocketFrameReader : AllocBase {
	enum class Status : uint8_t {
		Head,
		Size16,
		Size64,
		Mask,
		Body,
		Control
	};

	enum class Error : uint8_t {
		None,
		NotInitialized, // error in reader initialization
		ExtraIsNotEmpty,// rsv 1-3 is not empty
		NotMasked,// input frame is not masked
		UnknownOpcode,// unknown opcode in frame
		InvalidSegment,// invalid FIN or OPCODE sequence in segmented frames
		InvalidSize,// frame (or sequence) is larger then max size
		InvalidAction,// Handler tries to perform invalid reading action
	};

	struct Frame {
		bool fin; // fin value inside current frame
		WebsocketFrameType type; // opcode from first frame
		Bytes buffer; // common data buffer
		size_t block; // size of completely written block when segmented
		size_t offset; // offset inside current frame
	};

	static WebsocketFrameType getTypeFromOpcode(uint8_t opcode);
	static bool isControlFrameType(WebsocketFrameType t);

	static void unmask(uint32_t mask, size_t offset, uint8_t *data, size_t nbytes);

	template <typename B>
	static size_t getBufferRequiredBytes(const B &buf, size_t maxSize) {
		return (buf.size() < maxSize) ? (maxSize - buf.size()) : 0;
	}

	bool fin = false;
	bool masked = false;

	Status status = Status::Head;
	Error error = Error::None;
	WebsocketFrameType type = WebsocketFrameType::None;
	uint8_t extra = 0;
	uint32_t mask = 0;
	size_t size = 0;
	size_t max = config::WEBSOCKET_DEFAULT_MAX_FRAME_SIZE; // absolute maximum (even for segmented frames)

	Frame frame;
	pool_t *pool = nullptr;
	Root * root = nullptr;
	StackBuffer<128> buffer;

	WebsocketFrameReader(Root *r, pool_t *p);

	explicit operator bool() const {  return error == Error::None; }

	size_t getRequiredBytes() const;
	uint8_t * prepare(size_t &len);
	bool save(uint8_t *, size_t nbytes);

	bool isFrameReady() const;
	bool isControlReady() const;
	void popFrame();
	void clear();

	bool updateState();
};

struct SP_PUBLIC WebsocketFrameWriter : AllocBase {
	static uint8_t getOpcodeFromType(WebsocketFrameType opcode);

	static size_t getFrameSize(size_t dataSize, bool masked = false);
	static size_t makeHeader(uint8_t *buf, size_t dataSize, WebsocketFrameType t, bool masked = false, uint32_t mask = 0);
	static void makeHeader(StackBuffer<32> &buf, size_t dataSize, WebsocketFrameType t, bool masked = false, uint32_t mask = 0);

	struct Slice {
		uint8_t *data;
		size_t size;
		Slice *next;
	};

	struct WriteSlot : AllocBase {
		pool_t *pool;
		size_t alloc = 0;
		size_t offset = 0;
		Slice *firstData = nullptr;
		Slice *lastData = nullptr;

		WriteSlot *next = nullptr;

		WriteSlot(pool_t *p);

		bool empty() const;

		void emplace(const uint8_t *data, size_t size);

		void pop(size_t size);

		uint8_t * getNextBytes() const;
		size_t getNextLength() const;
	};

	pool_t *pool = nullptr;
	WriteSlot *firstSlot = nullptr;
	WriteSlot *lastSlot = nullptr;

	WebsocketFrameWriter(pool_t *p);

	bool empty() const;

	WriteSlot *nextReadSlot() const;

	void popReadSlot();

	WriteSlot *nextEmplaceSlot(size_t sizeOfData);
};


}

#endif /* EXTRA_WEBSERVER_WEBSERVER_WEBSOCKET_SPWEBWEBSOCKET_H_ */
