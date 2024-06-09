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

#include "XLCommon.h"

#include "SPBitmap.h"

#include "XLCoreFrameRequest.h"
#include "XLVkPlatform.h"
#include "XLVkAttachment.h"
#include "XLVkPipeline.h"
#include "XLVkRenderPass.h"

#include "NoiseQueue.h"

namespace stappler::xenolith::app {

extern SpanView<uint32_t> NoiseComp;

class NoisePass : public vk::QueuePass {
public:
	virtual ~NoisePass();

	virtual bool init(Queue::Builder &queueBuilder, QueuePassBuilder &, const AttachmentData *, const AttachmentData *);

	const AttachmentData *getDataAttachment() const { return _dataAttachment; }
	const AttachmentData *getImageAttachment() const { return _imageAttachment; }

protected:
	using QueuePass::init;

	const AttachmentData *_dataAttachment = nullptr;
	const AttachmentData *_imageAttachment = nullptr;
};

bool NoiseQueue::run(StringView target, NoiseData noiseData, Extent2 extent) {
	Application::CommonInfo commonInfo({
		.bundleName = String("org.stappler.examples.noisegen"),
		.applicationName = String("stappler-noisegen"),
		.applicationVersion = String("0.1.0")
	});

	auto instance = vk::platform::createInstance([&] (vk::platform::VulkanInstanceData &data, const vk::platform::VulkanInstanceInfo &info) {
		data.applicationName = commonInfo.applicationName;
		data.applicationVersion = commonInfo.applicationVersion;
		return true;
	});

	// create main looper
	auto app = Rc<Application>::create(move(commonInfo), move(instance));

	// define device selector/initializer
	auto data = Rc<vk::LoopData>::alloc();
	data->deviceSupportCallback = [] (const vk::DeviceInfo &dev) {
		return dev.requiredExtensionsExists && dev.requiredFeaturesExists;
	};

	Application::CallbackInfo callbackInfo({
		.updateCallback = [target, noiseData, extent] (const Application &app, const UpdateTime &time) {
			if (time.app == 0) {
				auto queue = Rc<NoiseQueue>::create();

				// then compile it on graphics device
				app.getGlLoop()->compileQueue(queue, [app = &app, queue, extent, target, noiseData] (bool success) {
					Application::getInstance()->performOnMainThread([app, queue, target, noiseData, extent] {
						queue->run(const_cast<Application *>(app), noiseData, extent, target);
					}, nullptr);
				});
			}
		}
	});

	core::LoopInfo info;
	info.platformData = data;

	// run main loop with 2 additional threads and 0.5sec update interval
	app->run(callbackInfo, move(info), 2, TimeInterval::microseconds(500000));

	return true;
}

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
			h->setInputCallback([] (AttachmentHandle &handle, FrameQueue &queue, Rc<AttachmentInputData> &&input, Function<void(bool)> &&cb) {
				auto a = static_cast<vk::BufferAttachment *>(handle.getAttachment().get());
				auto d = static_cast<NoiseDataInput *>(input.get());
				auto devFrame = static_cast<vk::DeviceFrameHandle *>(queue.getFrame().get());
				auto b = static_cast<vk::BufferAttachmentHandle *>(&handle);

				auto buf = devFrame->getMemPool(devFrame)->spawn(vk::AllocationUsage::DeviceLocalHostVisible, a->getInfo());

				buf->map([&] (uint8_t *data, VkDeviceSize) {
					memcpy(data, &d->data, sizeof(NoiseData));
				});

				b->addBufferView(buf);

				cb(true);
			});
			return h;
		});
		return a;
	});

	auto imageAttachment = builder.addAttachemnt("NoiseImageAttachment", [&] (AttachmentBuilder &attachmentBuilder) -> Rc<Attachment> {
		attachmentBuilder.defineAsOutput();
		return Rc<vk::ImageAttachment>::create(attachmentBuilder,
			ImageInfo(Extent2(1024, 768), ImageUsage::Storage | ImageUsage::TransferSrc, ImageTiling::Optimal, PassType::Compute, ImageFormat::R8G8B8A8_UNORM),
			ImageAttachment::AttachmentInfo{
				.initialLayout = AttachmentLayout::Undefined,
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

void NoiseQueue::run(Application *app, NoiseData data, Extent2 extent, StringView target) {
	run(app->getGlLoop(), data, extent, [app, target = target.str<Interface>()] (core::ImageInfoData info, BytesView view) {
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
				bmp.save(target);
			}
		}
		app->end();
	});
}

void NoiseQueue::run(core::Loop *loop, NoiseData data, Extent2 extent,
		Function<void(core::ImageInfoData info, BytesView view)> &&cb) {
	auto req = Rc<core::FrameRequest>::create(Rc<core::Queue>(this), core::FrameContraints{extent});

	auto inputData = Rc<NoiseDataInput>::alloc();
	inputData->data = data;

	req->addInput(getDataAttachment(), move(inputData));
	req->setOutput(getImageAttachment(), [loop, cb = move(cb)] (core::FrameAttachmentData &data, bool success, Ref *) mutable {
		loop->captureImage([cb = move(cb)] (core::ImageInfoData info, BytesView view) {
			cb(info, view);
		}, data.image->getImage(), core::AttachmentLayout::General);
		return true;
	});

	loop->runRenderQueue(move(req), 0);
}

NoisePass::~NoisePass() { }

bool NoisePass::init(Queue::Builder &queueBuilder, QueuePassBuilder &builder, const AttachmentData *data, const AttachmentData *image) {
	using namespace core;

	auto passImage = builder.addAttachment(image, [] (AttachmentPassBuilder &builder) {
		builder.setDependency(AttachmentDependencyInfo{
			PipelineStage::ComputeShader, AccessType::ShaderWrite,
			PipelineStage::ComputeShader, AccessType::ShaderWrite,
			FrameRenderPassState::Complete,
		});
	});

	auto layout = builder.addDescriptorLayout([&] (PipelineLayoutBuilder &layoutBuilder) {
		layoutBuilder.addSet([&] (DescriptorSetBuilder &setBuilder) {
			setBuilder.addDescriptor(builder.addAttachment(data));
			setBuilder.addDescriptor(passImage, DescriptorType::StorageImage, AttachmentLayout::General);
		});
	});

	builder.addSubpass([&] (SubpassBuilder &subpassBuilder) {
		subpassBuilder.addComputePipeline("NoisePipeline", layout,
				queueBuilder.addProgramByRef("NoisePipelineComp", NoiseComp));

		subpassBuilder.setCommandsCallback([&] (const SubpassData &subpass, FrameQueue &frame, CommandBuffer &commands) {
			auto &buf = static_cast<vk::CommandBuffer &>(commands);
			auto imageAttachment = static_cast<vk::ImageAttachmentHandle *>(frame.getAttachment(_imageAttachment)->handle.get());
			auto image = (vk::Image *)imageAttachment->getImage()->getImage().get();
			auto extent = frame.getFrame()->getFrameConstraints().extent;

			vk::ImageMemoryBarrier inImageBarriers[] = {
				vk::ImageMemoryBarrier(image, 0, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL)
			};

			buf.cmdPipelineBarrier(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0,
					inImageBarriers);

			buf.cmdBindDescriptorSets(static_cast<vk::RenderPass *>(subpass.pass->impl.get()), 0);

			auto pipeline = (vk::ComputePipeline *)subpass.computePipelines.get("NoisePipeline")->pipeline.get();

			buf.cmdBindPipeline(pipeline);

			buf.cmdDispatch((extent.width - 1) / pipeline->getLocalX() + 1, (extent.height - 1) / pipeline->getLocalY() + 1);
		});
	});

	_dataAttachment = data;
	_imageAttachment = image;

	return QueuePass::init(builder);
}

}
