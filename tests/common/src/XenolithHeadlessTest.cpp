/**
 Copyright (c) 2023 Stappler LLC <admin@stappler.dev>

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
#include "Test.h"

#if MODULE_XENOLITH_APPLICATION && MODULE_XENOLITH_BACKEND_VK

#include "SPBitmap.h"
#include "XLApplication.h"
#include "XLVkPlatform.h"
#include "XLVkLoop.h"
#include "XLVkAttachment.h"
#include "XLVkRenderPass.h"
#include "XLVkQueuePass.h"
#include "XLVkPipeline.h"
#include "XLCoreAttachment.h"
#include "XLCoreFrameQueue.h"
#include "XLCoreFrameRequest.h"

#include "XLSnnGenTest.h"
#include "XLSnnInputTest.h"
#include "XLSnnModelTest.h"
#include "XLSnnModelProcessor.h"

namespace STAPPLER_VERSIONIZED stappler::xenolith::test::detail {

#include "noise.comp.h"

SpanView<uint32_t> NoiseComp(noise_comp);

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
	virtual ~NoiseQueue();

	bool init();

	const AttachmentData *getDataAttachment() const { return _dataAttachment; }
	const AttachmentData *getImageAttachment() const { return _imageAttachment; }

	void run(Application *);

protected:
	using core::Queue::init;

	const AttachmentData *_dataAttachment = nullptr;
	const AttachmentData *_imageAttachment = nullptr;
};

class NoisePass : public vk::QueuePass {
public:
	virtual ~NoisePass();

	virtual bool init(Queue::Builder &queueBuilder, QueuePassBuilder &, const AttachmentData *, const AttachmentData *);

	virtual Rc<QueuePassHandle> makeFrameHandle(const FrameQueue &) override;

	const AttachmentData *getDataAttachment() const { return _dataAttachment; }
	const AttachmentData *getImageAttachment() const { return _imageAttachment; }

protected:
	using QueuePass::init;

	const AttachmentData *_dataAttachment = nullptr;
	const AttachmentData *_imageAttachment = nullptr;
};

class NoisePassHandle : public vk::QueuePassHandle {
public:
	virtual ~NoisePassHandle() { }

	virtual bool prepare(FrameQueue &q, Function<void(bool)> &&cb) override;

protected:
	virtual Vector<const core::CommandBuffer *> doPrepareCommands(FrameHandle &) override;

	const vk::ImageAttachmentHandle *_image = nullptr;
};

NoiseQueue::~NoiseQueue() { }

bool NoiseQueue::init() {
	using namespace core;
	Queue::Builder builder("Noise");

	auto dataAttachment = builder.addAttachemnt("NoiseDataAttachment", [&] (AttachmentBuilder &attachmentBuilder) -> Rc<Attachment> {
		attachmentBuilder.defineAsInput();
		auto a = Rc<vk::BufferAttachment>::create(attachmentBuilder, core::BufferInfo(
			core::BufferUsage::UniformBuffer, sizeof(NoiseData)
		));

		a->setValidateInputCallback([] (const Attachment &, const Rc<AttachmentInputData> &data) {
			return dynamic_cast<NoiseDataInput *>(data.get()) != nullptr;
		});

		a->setFrameHandleCallback([] (Attachment &a, const FrameQueue &queue) {
			auto h = Rc<vk::BufferAttachmentHandle>::create(a, queue);
			h->setInputCallback([] (AttachmentHandle &handle, FrameQueue &queue, AttachmentInputData *input, Function<void(bool)> &&cb) {
				auto a = static_cast<vk::BufferAttachment *>(handle.getAttachment().get());
				auto d = static_cast<NoiseDataInput *>(input);
				auto devFrame = static_cast<vk::DeviceFrameHandle *>(queue.getFrame().get());
				auto buf = devFrame->getMemPool(devFrame)->spawn(vk::AllocationUsage::DeviceLocalHostVisible, a->getInfo());
				auto b = static_cast<vk::BufferAttachmentHandle *>(&handle);

				buf->map([&] (uint8_t *buf, VkDeviceSize) {
					memcpy(buf, &d->data, sizeof(NoiseData));
				});

				b->addBufferView(move(buf));

				cb(true);
			});
			return h;
		});
		return a;
	});

	auto imageAttachment = builder.addAttachemnt("NoiseImageAttachment", [&] (AttachmentBuilder &attachmentBuilder) -> Rc<Attachment> {
		attachmentBuilder.defineAsOutput();
		return Rc<vk::ImageAttachment>::create(attachmentBuilder,
			ImageInfo(Extent2(64, 64), ImageUsage::Storage | ImageUsage::TransferSrc, ImageTiling::Optimal, ImageFormat::R8G8B8A8_UNORM),
			ImageAttachment::AttachmentInfo{
				.initialLayout = AttachmentLayout::General,
				.finalLayout = AttachmentLayout::General,
				.clearOnLoad = true,
				.clearColor = Color4F(0.0f, 0.0f, 0.0f, 0.0f)}
		);
	});

	builder.addPass("NoisePass", PassType::Compute, RenderOrdering(0), [&] (QueuePassBuilder &passBuilder) -> Rc<core::QueuePass> {
		return Rc<NoisePass>::create(builder, passBuilder, dataAttachment, imageAttachment);
	});

	if (core::Queue::init(move(builder))) {
		_dataAttachment = dataAttachment;
		_imageAttachment = imageAttachment;
		return true;
	}
	return false;
}

static void runBitmapTestsPool(bitmap::BitmapTemplate<memory::PoolInterface> &bmp) {
	auto path = filesystem::currentDir<Interface>(toString(Time::now().toMicros()));

	bmp.save(bitmap::FileFormat::Png, path);
	filesystem::remove(path);

	bmp.save(bitmap::FileFormat::WebpLossless, path);
	filesystem::remove(path);

	bmp.save(bitmap::FileFormat::WebpLossy, path);
	filesystem::remove(path);

	bmp.truncate(bitmap::PixelFormat::RGB888);
	bmp.save(bitmap::FileFormat::Jpeg, path);
	filesystem::remove(path);
}

static void readBitmap(BytesView data) {
	mem_pool::perform_temporary([&] {
		bitmap::BitmapTemplate<memory::PoolInterface> d(data);
		runBitmapTestsPool(d);
	});
}

static void runBitmapTests(Bitmap &bmp) {
	auto path = filesystem::currentDir<Interface>(toString(Time::now().toMicros()));
	bmp.save(bitmap::FileFormat::Png, path);
	filesystem::remove(path);

	bmp.save(bitmap::FileFormat::WebpLossless, path);
	filesystem::remove(path);

	bmp.save(bitmap::FileFormat::WebpLossy, path);
	filesystem::remove(path);

	auto upscale = bmp.resample(96, 96);
	auto downscale = bmp.resample(48, 48);

	readBitmap(upscale.write(bitmap::FileFormat::Png));
	readBitmap(downscale.write(bitmap::FileFormat::WebpLossless));
	readBitmap(downscale.write(bitmap::FileFormat::WebpLossy));

	bmp.truncate(bitmap::PixelFormat::RGB888);
	bmp.save(bitmap::FileFormat::Jpeg, path);
	filesystem::remove(path);

	upscale.truncate(bitmap::PixelFormat::RGB888);
	readBitmap(upscale.write(bitmap::FileFormat::Jpeg));

	mem_pool::perform_temporary([&] {
		bitmap::BitmapTemplate<memory::PoolInterface> d(upscale.write(bitmap::FileFormat::Png));

		for (int i = 0; i <= toInt(bitmap::ResampleFilter::QuadMix); ++ i) {
			auto path = filesystem::currentDir<Interface>(toString(Time::now().toMicros()));
			auto tmp = d.resample(bitmap::ResampleFilter(i), 96, 96);
			tmp.save(bitmap::FileFormat::Png, path);
			filesystem::remove(path);
		}

		for (int i = 0; i <= toInt(bitmap::ResampleFilter::QuadMix); ++ i) {
			auto path = filesystem::currentDir<Interface>(toString(Time::now().toMicros()));
			auto tmp = d.resample(bitmap::ResampleFilter(i), 48, 48);
			tmp.save(bitmap::FileFormat::Png, path);
			filesystem::remove(path);
		}
	});

}

void NoiseQueue::run(Application *app) {
	auto req = Rc<core::FrameRequest>::create(Rc<core::Queue>(this), core::FrameConstraints{Extent2(64, 64)});

	auto inputData = Rc<NoiseDataInput>::alloc();
	inputData->data = NoiseData{0, 0, 0.0f, 0.0f};

	req->addInput(getDataAttachment(), move(inputData));
	req->setOutput(getImageAttachment(), [app] (core::FrameAttachmentData &data, bool success, Ref *) {
		app->getGlLoop()->captureImage([app] (core::ImageInfoData info, BytesView view) {
			if (!view.empty()) {
				auto fmt = core::getImagePixelFormat(info.format);
				bitmap::PixelFormat pixelFormat = bitmap::PixelFormat::Auto;
				switch (fmt) {
				case core::PixelFormat::A: pixelFormat = bitmap::PixelFormat::A8; break;
				case core::PixelFormat::IA: pixelFormat = bitmap::PixelFormat::IA88; break;
				case core::PixelFormat::RGB: pixelFormat = bitmap::PixelFormat::RGB888; break;
				case core::PixelFormat::RGBA: pixelFormat = bitmap::PixelFormat::RGBA8888; break;
				default: break;
				}
				if (pixelFormat != bitmap::PixelFormat::Auto) {
					Bitmap bmp(view.data(), info.extent.width, info.extent.height, pixelFormat);
					runBitmapTests(bmp);
				}
			}
			//app->end();
		}, data.image->getImage(), core::AttachmentLayout::General);
		return true;
	});

	app->getGlLoop()->runRenderQueue(move(req), 0);
}

NoisePass::~NoisePass() { }

bool NoisePass::init(Queue::Builder &queueBuilder, QueuePassBuilder &builder, const AttachmentData *data, const AttachmentData *image) {
	using namespace core;

	auto passImage = builder.addAttachment(image, [] (AttachmentPassBuilder &builder) {
		builder.setDependency(AttachmentDependencyInfo{
			PipelineStage::ComputeShader, AccessType::ShaderWrite,
			PipelineStage::ComputeShader, AccessType::ShaderWrite,
			FrameRenderPassState::Submitted,
		});
	});

	auto layout = builder.addDescriptorLayout([&] (PipelineLayoutBuilder &layoutBuilder) {
		layoutBuilder.addSet([&] (DescriptorSetBuilder &setBuilder) {
			setBuilder.addDescriptor(builder.addAttachment(data));
			setBuilder.addDescriptor(passImage, DescriptorType::StorageImage, AttachmentLayout::General);
		});
	});

	Shader::inspectShader(NoiseComp);

	builder.addSubpass([&] (SubpassBuilder &subpassBuilder) {
		subpassBuilder.addComputePipeline("NoisePipeline", layout,
				queueBuilder.addProgramByRef("NoisePipelineComp", NoiseComp));
	});

	_dataAttachment = data;
	_imageAttachment = image;

	return QueuePass::init(builder);
}

auto NoisePass::makeFrameHandle(const FrameQueue &queue) -> Rc<QueuePassHandle> {
	return Rc<NoisePassHandle>::create(*this, queue);
}

bool NoisePassHandle::prepare(FrameQueue &q, Function<void(bool)> &&cb) {
	auto pass = (NoisePass *)_queuePass.get();

	if (auto imageAttachment = q.getAttachment(pass->getImageAttachment())) {
		_image = (const vk::ImageAttachmentHandle *)imageAttachment->handle.get();
	}

	return vk::QueuePassHandle::prepare(q, sp::move(cb));
}

Vector<const core::CommandBuffer *> NoisePassHandle::doPrepareCommands(FrameHandle &handle) {
	auto buf = _pool->recordBuffer(*_device, Vector<Rc<vk::DescriptorPool>>(_descriptors), [&, this] (vk::CommandBuffer &buf) {
		auto pass = _data->impl.cast<vk::RenderPass>().get();
		pass->perform(*this, buf, [&, this] {
			auto extent = handle.getFrameConstraints().extent;

			buf.cmdBindDescriptorSets(pass, 0);

			auto pipeline = (vk::ComputePipeline *)_data->subpasses[0]->computePipelines.get("NoisePipeline")->pipeline.get();

			buf.cmdBindPipeline(pipeline);

			buf.cmdDispatch((extent.width - 1) / pipeline->getLocalX() + 1, (extent.height - 1) / pipeline->getLocalY() + 1);
		}, true);
		return true;
	});
	return Vector<const core::CommandBuffer *>{buf};
}

static void runTests() {
	ApplicationInfo commonInfo;
	commonInfo.bundleName = String("org.stappler.xenolith.cli");
	commonInfo.applicationName = String("xenolith-cli");
	commonInfo.applicationVersion = String("0.1.0");
	commonInfo.updateCallback = [] (const PlatformApplication &app, const UpdateTime &time) {
		if (time.app == 0) {
			auto noiseQueue = Rc<NoiseQueue>::create();

			// then compile it on graphics device
			app.getGlLoop()->compileQueue(noiseQueue, [app = &app, noiseQueue] (bool success) {
				Application::getInstance()->performOnAppThread([app, noiseQueue] {
					noiseQueue->run((Application *)app);
				}, nullptr);
			});

			auto modelPath = filesystem::currentDir<Interface>("resources/mnist.json");
			auto inputPath = toString("mnist:", filesystem::currentDir<Interface>("resources/mnist"));

			auto modelQueue = Rc<shadernn::ModelQueue>::create(modelPath, shadernn::ModelFlags::None, inputPath);
			if (modelQueue) {
				modelQueue->retain();
				app.getGlLoop()->compileQueue(modelQueue, [app = &app, modelQueue] (bool success) {
					Application::getInstance()->performOnAppThread([app, modelQueue] {
						modelQueue->run((Application *)app);
					}, nullptr);
				});
			}
		}
	};
	commonInfo.appThreadsCount = 2;
	commonInfo.updateInterval = TimeInterval::microseconds(500000);

	// define device selector/initializer
	auto data = Rc<vk::LoopData>::alloc();
	data->deviceSupportCallback = [] (const vk::DeviceInfo &dev) {
		return dev.requiredExtensionsExists && dev.requiredFeaturesExists;
	};

	commonInfo.loopInfo.platformData = data;

	auto instance = vk::platform::createInstance([&] (vk::platform::VulkanInstanceData &data, const vk::platform::VulkanInstanceInfo &info) {
		data.applicationName = commonInfo.applicationName;
		data.applicationVersion = commonInfo.applicationVersion;
		return true;
	});

	// create main looper
	auto app = Rc<Application>::create(move(commonInfo), move(instance));

	// run main loop with 2 additional threads and 0.5sec update interval
	app->run();
	app->waitStopped();
}

}

namespace STAPPLER_VERSIONIZED stappler::app::test {

struct XenolithHeadlessTest : Test {
	XenolithHeadlessTest() : Test("XenolithHeadlessTest") { }

	virtual bool run() override {
		using namespace stappler::xenolith::test::detail;

		auto mempool = memory::pool::create();
		memory::pool::push(mempool);

		runTests();

		memory::pool::pop();

		return true;
	}

} XenolithHeadlessTest;

}

#endif
