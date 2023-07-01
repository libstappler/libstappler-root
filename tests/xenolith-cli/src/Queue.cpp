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

#include "../../xenolith-cli/src/Queue.h"

#include "../../xenolith-cli/stappler-build/host/gcc/debug/include/XLCommon.h"
#include "XLVkAttachment.h"
#include "XLVkRenderPass.h"
#include "XLVkPipeline.h"
#include "XLVkBuffer.h"
#include "XLCoreFrameQueue.h"

namespace stappler::xenolith::test {

extern SpanView<uint32_t> NoiseComp;

class NoiseDataAttachment : public vk::BufferAttachment {
public:
	virtual ~NoiseDataAttachment();

	virtual bool init(AttachmentBuilder &);

	virtual bool validateInput(const Rc<core::AttachmentInputData> &) const override;

protected:
	using BufferAttachment::init;

	virtual Rc<AttachmentHandle> makeFrameHandle(const FrameQueue &) override;
};

class NoiseDataAttachmentHandle : public vk::BufferAttachmentHandle {
public:
	virtual ~NoiseDataAttachmentHandle();

	virtual void submitInput(FrameQueue &, Rc<core::AttachmentInputData> &&, Function<void(bool)> &&) override;

	virtual bool isDescriptorDirty(const PassHandle &, const vk::PipelineDescriptor &,
			uint32_t, bool isExternal) const override;

	virtual bool writeDescriptor(const core::QueuePassHandle &, vk::DescriptorBufferInfo &) override;

	const Rc<vk::DeviceBuffer> &getBuffer() const { return _data; }

protected:
	void loadData(vk::DeviceFrameHandle *devFrame);

	Rc<vk::DeviceBuffer> _data;
	Rc<NoiseDataInput> _input;
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
	virtual Vector<const vk::CommandBuffer *> doPrepareCommands(FrameHandle &);

	const vk::ImageAttachmentHandle *_image = nullptr;
};


NoiseQueue::~NoiseQueue() { }

bool NoiseQueue::init() {
	using namespace core;
	Queue::Builder builder("Noise");

	auto dataAttachment = builder.addAttachemnt("NoiseDataAttachment", [&] (AttachmentBuilder &attachmentBuilder) -> Rc<Attachment> {
		attachmentBuilder.defineAsInput();
		return Rc<NoiseDataAttachment>::create(attachmentBuilder);
	});

	auto imageAttachment = builder.addAttachemnt("NoiseImageAttachment", [&] (AttachmentBuilder &attachmentBuilder) -> Rc<Attachment> {
		attachmentBuilder.defineAsOutput();
		return Rc<vk::ImageAttachment>::create(attachmentBuilder,
			ImageInfo(Extent2(1024, 768), ImageUsage::Storage | ImageUsage::TransferSrc, ImageTiling::Optimal, ImageFormat::R8G8B8A8_UNORM),
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

NoiseDataAttachment::~NoiseDataAttachment() { }

bool NoiseDataAttachment::init(AttachmentBuilder &builder) {
	return vk::BufferAttachment::init(builder, core::BufferInfo(
		core::BufferUsage::UniformBuffer, sizeof(NoiseData)
	));
}

bool NoiseDataAttachment::validateInput(const Rc<core::AttachmentInputData> &data) const {
	if (dynamic_cast<NoiseDataInput *>(data.get())) {
		return true;
	}
	return false;
}

auto NoiseDataAttachment::makeFrameHandle(const FrameQueue &handle) -> Rc<AttachmentHandle> {
	return Rc<NoiseDataAttachmentHandle>::create(this, handle);
}

NoiseDataAttachmentHandle::~NoiseDataAttachmentHandle() { }

void NoiseDataAttachmentHandle::submitInput(FrameQueue &q, Rc<core::AttachmentInputData> &&data, Function<void(bool)> &&cb) {
	auto d = data.cast<NoiseDataInput>();
	if (!d || q.isFinalized()) {
		cb(false);
		return;
	}

	q.getFrame()->waitForDependencies(data->waitDependencies, [this, d = move(d), cb = move(cb)] (FrameHandle &handle, bool success) mutable {
		if (!success || !handle.isValidFlag()) {
			cb(false);
			return;
		}

		auto devFrame = (vk::DeviceFrameHandle *)(&handle);

		_input = move(d);
		_data = devFrame->getMemPool(devFrame)->spawn(vk::AllocationUsage::DeviceLocalHostVisible,
					core::BufferInfo(((vk::BufferAttachment *)_attachment.get())->getInfo()));

		loadData(devFrame);

		cb(true);
	});
}

bool NoiseDataAttachmentHandle::isDescriptorDirty(const PassHandle &, const vk::PipelineDescriptor &,
		uint32_t, bool isExternal) const {
	return _data;
}

bool NoiseDataAttachmentHandle::writeDescriptor(const core::QueuePassHandle &, vk::DescriptorBufferInfo &info) {
	info.buffer = _data;
	info.offset = 0;
	info.range = _data->getSize();
	return true;
}

void NoiseDataAttachmentHandle::loadData(vk::DeviceFrameHandle *devFrame) {
	NoiseData *data = nullptr;
	vk::DeviceBuffer::MappedRegion mapped;
	if (devFrame->isPersistentMapping()) {
		mapped = _data->map();
		data = (NoiseData *)mapped.ptr;
	} else {
		data = new NoiseData;
	}

	memcpy(data, &_input->data, sizeof(NoiseData));

	if (devFrame->isPersistentMapping()) {
		_data->unmap(mapped, true);
	} else {
		_data->setData(BytesView((const uint8_t *)data, sizeof(NoiseData)));
		delete data;
	}
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
	auto pass = (NoisePass *)_renderPass.get();

	if (auto imageAttachment = q.getAttachment(pass->getImageAttachment())) {
		_image = (const vk::ImageAttachmentHandle *)imageAttachment->handle.get();
	}

	return vk::QueuePassHandle::prepare(q, move(cb));
}

Vector<const vk::CommandBuffer *> NoisePassHandle::doPrepareCommands(FrameHandle &handle) {
	auto buf = _pool->recordBuffer(*_device, [&] (vk::CommandBuffer &buf) {
		auto pass = _data->impl.cast<vk::RenderPass>().get();
		pass->perform(*this, buf, [&] {
			auto sdfImage = (vk::Image *)_image->getImage()->getImage().get();
			auto extent = handle.getFrameConstraints().extent;

			vk::ImageMemoryBarrier inImageBarriers[] = {
				vk::ImageMemoryBarrier(sdfImage, 0, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL)
			};

			buf.cmdPipelineBarrier(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0,
					inImageBarriers);

			buf.cmdBindDescriptorSets(pass, 0);

			auto pipeline = (vk::ComputePipeline *)_data->subpasses[0]->computePipelines.get("NoisePipeline")->pipeline.get();

			buf.cmdBindPipeline(pipeline);

			buf.cmdDispatch((extent.width - 1) / pipeline->getLocalX() + 1, (extent.height - 1) / pipeline->getLocalY() + 1);
		});
		return true;
	});
	return Vector<const vk::CommandBuffer *>{buf};
}

}
