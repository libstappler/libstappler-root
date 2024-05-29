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

#include "SPWebHostComponent.h"
#include "SPWebRequestHandler.h"
#include "SPBitmap.h"
#include "SPValid.h"

#include "XLApplication.h"
#include "XLVkPlatform.h"

#include "NoiseQueue.h"

namespace stappler::web::test {

class VkComponent;

class VkComponent : public HostComponent {
public:
	VkComponent(const Host &serv, const HostComponentInfo &info);
	virtual ~VkComponent();

	virtual void handleChildInit(const Host &) override;

	bool isQueueStarted() const { return _queueStarted; }
	xenolith::core::Loop *getLoop() const { return _loop; }
	xenolith::app::NoiseQueue *getQueue() const { return _queue; }

protected:
	String _appName;
	String _appVersion;

	std::atomic<bool> _queueStarted;

	Rc<xenolith::core::Instance> _instance;
	Rc<xenolith::core::Loop> _loop;
	Rc<xenolith::app::NoiseQueue> _queue;
};

class VkRequestHandler : public RequestHandler {
public:
	virtual bool isRequestPermitted(Request & rctx) override {
		return true;
	}

	virtual Status onTranslateName(Request &rctx) override {
		auto comp = rctx.host().getComponent<VkComponent>();
		if (!comp->isQueueStarted()) {
			return HTTP_BAD_REQUEST;
		}

		xenolith::app::NoiseData data{0, 0, 1.0f, 1.0f};
		xenolith::Extent2 extent(1024, 768);

		auto &queryData = rctx.getInfo().queryData;

		if (queryData.getBool("random")) {
			valid::makeRandomBytes((uint8_t *)&data, sizeof(uint32_t) * 2);
		}

		if (queryData.hasValue("dx")) {
			data.seedX = queryData.getInteger("dx");
		}

		if (queryData.hasValue("dx")) {
			data.seedY = queryData.getInteger("dx");
		}

		if (queryData.hasValue("width")) {
			extent.width = queryData.getInteger("width", 1024);
		}

		if (queryData.hasValue("height")) {
			extent.height = queryData.getInteger("height", 768);
		}

		std::unique_lock lock(_mutex);

		comp->getQueue()->run(comp->getLoop(), data, extent,
				[&] (xenolith::core::ImageInfoData info, BytesView view) {
			if (!view.empty()) {
				auto fmt = xenolith::core::getImagePixelFormat(info.format);
				bitmap::PixelFormat pixelFormat = bitmap::PixelFormat::Auto;
				switch (fmt) {
				case xenolith::core::PixelFormat::A: pixelFormat = bitmap::PixelFormat::A8; break;
				case xenolith::core::PixelFormat::IA: pixelFormat = bitmap::PixelFormat::IA88; break;
				case xenolith::core::PixelFormat::RGB: pixelFormat = bitmap::PixelFormat::RGB888; break;
				case xenolith::core::PixelFormat::RGBA: pixelFormat = bitmap::PixelFormat::RGBA8888; break;
				default: break;
				}
				if (pixelFormat != bitmap::PixelFormat::Auto) {
					_bitmap = bitmap::BitmapTemplate<memory::StandartInterface>(view.data(), info.extent.width, info.extent.height, pixelFormat);
				}
			}
			_var.notify_all();
		});

		_var.wait(lock);

		if (!_bitmap.empty()) {
			auto bytes = _bitmap.write(bitmap::FileFormat::Png);

			rctx.setContentType("image/png");
			rctx.write((const char *)bytes.data(), bytes.size());
			return DONE;
		}

		return HTTP_NOT_FOUND;
	}

protected:
	bitmap::BitmapTemplate<memory::StandartInterface> _bitmap;
	std::mutex _mutex;
	std::condition_variable _var;
};

VkComponent::~VkComponent() {

}

VkComponent::VkComponent(const Host &serv, const HostComponentInfo &info)
: HostComponent(serv, info) {
	using namespace xenolith;

	_appName = "stappler-web";
	_appVersion = "0.1.0";

	_instance = vk::platform::createInstance([&] (vk::platform::VulkanInstanceData &data, const vk::platform::VulkanInstanceInfo &info) {
		data.applicationName = _appName;
		data.applicationVersion = _appVersion;
		return true;
	});

	auto data = Rc<vk::LoopData>::alloc();
	data->deviceSupportCallback = [] (const vk::DeviceInfo &dev) {
		return dev.requiredExtensionsExists && dev.requiredFeaturesExists;
	};

	core::LoopInfo loopInfo;
	loopInfo.platformData = data;
	_loop = _instance->makeLoop(move(loopInfo));
	_loop->waitRinning();
}

void VkComponent::handleChildInit(const Host &serv) {
	_queue = Rc<xenolith::app::NoiseQueue>::create();

	// then compile it on graphics device
	_loop->compileQueue(_queue, [this] (bool success) {
		_queueStarted = success;
		log::verbose("VkComponent", "Queue compiled");
	});

	serv.addHandler("/vk", RequestHandler::Make<VkRequestHandler>());
}

SP_EXTERN_C HostComponent * CreateVkComponent(const Host &serv, const HostComponentInfo &info) {
	return new VkComponent(serv, info);
}

}
