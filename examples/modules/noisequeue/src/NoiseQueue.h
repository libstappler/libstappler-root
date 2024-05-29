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

#ifndef EXAMPLES_VK_HEADLESS_SRC_NOISEQUEUE_H_
#define EXAMPLES_VK_HEADLESS_SRC_NOISEQUEUE_H_

#include "XLVkQueuePass.h"
#include "XLCoreAttachment.h"
#include "XLApplication.h"

namespace stappler::xenolith::app {

struct NoiseData {
	uint32_t seedX;
	uint32_t seedY;
	float densityX;
	float densityY;
};

struct NoiseDataInput : core::AttachmentInputData {
	NoiseData data;
};

class NoiseQueue : public core::Queue {
public:
	static bool run(StringView target, NoiseData data, Extent2);

	virtual ~NoiseQueue();

	bool init();

	const AttachmentData *getDataAttachment() const { return _dataAttachment; }
	const AttachmentData *getImageAttachment() const { return _imageAttachment; }

	void run(Application *, NoiseData data, Extent2 extent, StringView target);

	void run(core::Loop *, NoiseData data, Extent2 extent,
			Function<void(core::ImageInfoData info, BytesView view)> &&);

protected:
	using core::Queue::init;

	const AttachmentData *_dataAttachment = nullptr;
	const AttachmentData *_imageAttachment = nullptr;
};

}

#endif /* EXAMPLES_VK_HEADLESS_SRC_NOISEQUEUE_H_ */
