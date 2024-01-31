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

#include "SPWebWebsocket.h"
#include "SPWebRoot.h"

namespace STAPPLER_VERSIONIZED stappler::web {

uint8_t WebsocketFrameWriter::getOpcodeFromType(WebsocketFrameType opcode) {
	switch (opcode) {
	case WebsocketFrameType::Continue: return 0x0; break;
	case WebsocketFrameType::Text: return 0x1; break;
	case WebsocketFrameType::Binary: return 0x2; break;
	case WebsocketFrameType::Close: return 0x8; break;
	case WebsocketFrameType::Ping: return 0x9; break;
	case WebsocketFrameType::Pong: return 0xA; break;
	default: break;
	}
	return 0;
}

WebsocketFrameType WebsocketFrameReader::getTypeFromOpcode(uint8_t opcode) {
	switch (opcode) {
	case 0x0: return WebsocketFrameType::Continue; break;
	case 0x1: return WebsocketFrameType::Text; break;
	case 0x2: return WebsocketFrameType::Binary; break;
	case 0x8: return WebsocketFrameType::Close; break;
	case 0x9: return WebsocketFrameType::Ping; break;
	case 0xA: return WebsocketFrameType::Pong; break;
	}
	return WebsocketFrameType::None;
}

bool WebsocketFrameReader::isControlFrameType(WebsocketFrameType t) {
	switch (t) {
	case WebsocketFrameType::Close:
	case WebsocketFrameType::Continue:
	case WebsocketFrameType::Ping:
	case WebsocketFrameType::Pong:
		return true;
		break;
	default:
		return false;
		break;
	}
	return false;
}

void WebsocketFrameWriter::makeHeader(StackBuffer<32> &buf, size_t dataSize, WebsocketFrameType t) {
	size_t sizeSize = (dataSize <= 125) ? 0 : ((dataSize > (size_t)maxOf<uint16_t>())? 8 : 2);
	size_t frameSize = 2 + sizeSize;

	buf.prepare(frameSize);

	buf[0] = ((uint8_t)0b10000000 | getOpcodeFromType(t));
	if (sizeSize == 0) {
		buf[1] = ((uint8_t)dataSize);
	} else if (sizeSize == 2) {
		buf[1] = ((uint8_t)126);
		uint16_t size = byteorder::HostToNetwork((uint16_t)dataSize);
		memcpy(buf.data() + 2, &size, sizeof(uint16_t));
	} else if (sizeSize == 8) {
		buf[1] = ((uint8_t)127);
		uint64_t size = byteorder::HostToNetwork((uint64_t)dataSize);
		memcpy(buf.data() + 2, &size, sizeof(uint64_t));
	}

	buf.save(nullptr, frameSize);
}

void WebsocketFrameReader::unmask(uint32_t mask, size_t offset, uint8_t *data, size_t nbytes) {
	uint8_t j = offset % 4;
	for (size_t i = 0; i < nbytes; ++i, ++j) {
		if (j >= 4) { j = 0; }
		data[i] ^= ((mask >> (j * 8)) & 0xFF);
	}
}

WebsocketFrameReader::WebsocketFrameReader(Root *r, pool_t *p)
: frame(Frame{false, WebsocketFrameType::None, Bytes(), 0, 0})
, pool(memory::pool::create(p)), root(r) {
	if (!pool) {
		error = Error::NotInitialized;
	} else {
		new (&frame.buffer) Bytes(pool); // switch allocator
	}
}

size_t WebsocketFrameReader::getRequiredBytes() const {
	switch (status) {
	case Status::Head: return getBufferRequiredBytes(buffer, 2); break;
	case Status::Size16: return getBufferRequiredBytes(buffer, 2); break;
	case Status::Size64: return getBufferRequiredBytes(buffer, 8); break;
	case Status::Mask: return getBufferRequiredBytes(buffer, 4); break;
	case Status::Body: return (frame.offset < size) ? (size - frame.offset) : 0; break;
	case Status::Control: return getBufferRequiredBytes(buffer, size); break;
	default: break;
	}
	return 0;
}

uint8_t * WebsocketFrameReader::prepare(size_t &len) {
	switch (status) {
	case Status::Head:
	case Status::Size16:
	case Status::Size64:
	case Status::Mask:
	case Status::Control:
		return buffer.prepare_preserve(len); break;
	case Status::Body:
		return frame.buffer.data() + frame.block + frame.offset; break;
	default: break;
	}
	return nullptr;
}

bool WebsocketFrameReader::save(uint8_t *b, size_t nbytes) {
	switch (status) {
	case Status::Head:
	case Status::Size16:
	case Status::Size64:
	case Status::Mask:
	case Status::Control:
		buffer.save(b, nbytes); break;
	case Status::Body:
		unmask(mask, frame.offset, b, nbytes);
		frame.offset += nbytes;
		break;
	default: break;
	}

	if (getRequiredBytes() == 0) {
		return updateState();
	}
	return true;
}

bool WebsocketFrameReader::updateState() {
	bool shouldPrepareBody = false;
	switch (status) {
	case Status::Head:
		size = 0;
		mask = 0;
		type = WebsocketFrameType::None;

		fin =		(buffer[0] & 0b10000000) != 0;
		extra =		(buffer[0] & 0b01110000);
		type = getTypeFromOpcode
					(buffer[0] & 0b00001111);
		masked =	(buffer[1] & 0b10000000) != 0;
		size =		(buffer[1] & 0b01111111);

		if (extra != 0 || !masked || type == WebsocketFrameType::None) {
			if (extra != 0) {
				error = Error::ExtraIsNotEmpty;
			} else if (!masked) {
				error = Error::NotMasked;
			} else {
				error = Error::UnknownOpcode;
			}
			root->error("Websocket", "Invalid control flow", Value(toInt(error)));
			return false;
		}

		if (!frame.buffer.empty()) {
			if (!isControlFrameType(type)) {
				error = Error::InvalidSegment;
				root->error("Websocket", "Invalid segment", Value(toInt(error)));
				return false;
			}
		}

		if (size > max) {
			error = Error::InvalidSize;
			root->error("Websocket", "Too large query", Value{{
				pair("size", Value(size)),
				pair("max", Value(max)),
			}});
			return false;
		}

		if (size == 126) {
			size = 0;
			status = Status::Size16;
		} else if (size == 127) {
			size = 0;
			status = Status::Size64;
		} else {
			status = Status::Mask;
		}

		buffer.clear();
		return true;
		break;
	case Status::Size16:
		size = buffer.get<BytesViewNetwork>().readUnsigned16();
		if (size > max) {
			error = Error::InvalidSize;
			root->error("Websocket", "Too large query", Value{{
				pair("size", Value(size)),
				pair("max", Value(max)),
			}});
			return false;
		}
		status = masked?Status::Mask:Status::Body;
		buffer.clear();
		shouldPrepareBody = true;
		break;
	case Status::Size64:
		size = buffer.get<BytesViewNetwork>().readUnsigned64();
		if (size > max) {
			error = Error::InvalidSize;
			root->error("Websocket", "Too large query", Value{{
				pair("size", Value(size)),
				pair("max", Value(max)),
			}});
			return false;
		}
		status = masked?Status::Mask:Status::Body;
		buffer.clear();
		shouldPrepareBody = true;
		break;
	case Status::Mask:
		mask = buffer.get<BytesView>().readUnsigned32();
		status = Status::Body;
		buffer.clear();
		shouldPrepareBody = true;
		break;
	case Status::Control:
		break;
	case Status::Body:
		frame.fin = fin;
		frame.block += size;
		if (type != WebsocketFrameType::Continue) {
			frame.type = type;
		}
		break;
	default:
		break;
	}

	if (shouldPrepareBody && status == Status::Body) {
		if (isControlFrameType(type)) {
			status = Status::Control;
		} else {
			if (size + frame.block > max) {
				error = Error::InvalidSize;
				root->error("Websocket", "Too large query", Value{{
					pair("size", Value(size + frame.block)),
					pair("max", Value(max)),
				}});
				return false;
			}
			frame.buffer.resize(size + frame.block);
		}
	}
	return true;
}

bool WebsocketFrameReader::isControlReady() const {
	if (status == Status::Control && getRequiredBytes() == 0) {
		return true;
	}
	return false;
}
bool WebsocketFrameReader::isFrameReady() const {
	if (status == Status::Body && getRequiredBytes() == 0 && frame.fin) {
		return true;
	}
	return false;
}
void WebsocketFrameReader::popFrame() {
	switch (status) {
	case Status::Control:
		buffer.clear();
		status = Status::Head;
		break;
	case Status::Body:
		clear();
		break;
	default:
		error = Error::InvalidAction;
		break;
	}
}

void WebsocketFrameReader::clear() {
	frame.buffer.force_clear();
	frame.buffer.clear();
	memory::pool::clear(pool); // clear frame-related data

	status = Status::Head;
	frame.block = 0;
	frame.offset = 0;
	frame.fin = true;
	frame.type = WebsocketFrameType::None;
}

WebsocketFrameWriter::WriteSlot::WriteSlot(pool_t *p) : pool(p) { }

bool WebsocketFrameWriter::WriteSlot::empty() const {
	return firstData == nullptr;
}

void WebsocketFrameWriter::WriteSlot::emplace(const uint8_t *data, size_t size) {
	auto mem = pool::palloc(pool, sizeof(Slice) + size);
	Slice *next = (Slice*) mem;
	next->data = (uint8_t*) mem + sizeof(Slice);
	next->size = size;
	next->next = nullptr;

	memcpy(next->data, data, size);

	if (lastData) {
		lastData->next = next;
		lastData = next;
	} else {
		firstData = next;
		lastData = next;
	}

	alloc += size + sizeof(Slice);
}

void WebsocketFrameWriter::WriteSlot::pop(size_t size) {
	if (size >= firstData->size - offset) {
		firstData = firstData->next;
		if (!firstData) {
			lastData = nullptr;
		}
		offset = 0;
	} else {
		offset += size;
	}
}

uint8_t* WebsocketFrameWriter::WriteSlot::getNextBytes() const {
	return firstData->data + offset;
}

size_t WebsocketFrameWriter::WriteSlot::getNextLength() const {
	return firstData->size - offset;
}

WebsocketFrameWriter::WebsocketFrameWriter(pool_t *p) : pool(p) { }

bool WebsocketFrameWriter::empty() const {
	return firstSlot == nullptr;
}

WebsocketFrameWriter::WriteSlot* WebsocketFrameWriter::nextReadSlot() const {
	return firstSlot;
}

void WebsocketFrameWriter::popReadSlot() {
	if (firstSlot->empty()) {
		memory::pool::destroy(firstSlot->pool);
		firstSlot = firstSlot->next;
		if (!firstSlot) {
			lastSlot = nullptr;
		}
	}
}

WebsocketFrameWriter::WriteSlot* WebsocketFrameWriter::nextEmplaceSlot(size_t sizeOfData) {
	if (!lastSlot || lastSlot->alloc + sizeOfData > 16_KiB) {
		auto p = memory::pool::create(pool);
		WriteSlot *slot = new (p) WriteSlot(p);
		if (lastSlot) {
			lastSlot->next = slot;
			lastSlot = slot;
		} else {
			firstSlot = slot;
			lastSlot = slot;
		}
	}
	return lastSlot;
}

}
