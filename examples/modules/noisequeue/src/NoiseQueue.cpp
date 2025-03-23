/**
 Copyright (c) 2024-2025 Stappler LLC <admin@stappler.dev>

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

// Проход рендеринга шума
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
	// Устанавливаем параметры приложения
	ApplicationInfo commonInfo;
	commonInfo.bundleName = String("org.stappler.examples.noisegen");
	commonInfo.applicationName = String("stappler-noisegen");
	commonInfo.applicationVersion = String("0.1.0");
	commonInfo.mainThreadsCount = 2;
	commonInfo.appThreadsCount = 2;
	commonInfo.updateInterval = TimeInterval::microseconds(500000);
	commonInfo.updateCallback = [target, noiseData, extent] (const PlatformApplication &app, const UpdateTime &time) {
		// Рабочий цикл приложения
		if (time.app == 0) {
			// Вызывается при запуске (нулевое время)
			auto queue = Rc<NoiseQueue>::create();

			// собираем очередь на устройстве
			app.getGlLoop()->compileQueue(queue, [app = &app, queue, extent, target, noiseData] (bool success) {
				Application::getInstance()->performOnAppThread([app, queue, target, noiseData, extent] {
					// Запускаем исполнение очереди
					queue->run(const_cast<PlatformApplication *>(app), noiseData, extent, target);
				}, nullptr);
			});
		}
	};

	// Создаём графический цикл и находим подходящее устройство исполнения
	auto data = Rc<vk::LoopData>::alloc();
	data->deviceSupportCallback = [] (const vk::DeviceInfo &dev) {
		return dev.requiredExtensionsExists && dev.requiredFeaturesExists;
	};

	commonInfo.loopInfo.platformData = data;

	// Загружаем графический API
	auto instance = vk::platform::createInstance([&] (vk::platform::VulkanInstanceData &data, const vk::platform::VulkanInstanceInfo &info) {
		data.applicationName = commonInfo.applicationName;
		data.applicationVersion = commonInfo.applicationVersion;
		return true;
	});

	// Создаём главный цикл приложения
	auto app = Rc<Application>::create(move(commonInfo), move(instance));

	if (app) {
		// запускаем основной цикл
		app->run();
		app->waitStopped();
		return true;
	}
	return false;
}

NoiseQueue::~NoiseQueue() { }

bool NoiseQueue::init() {
	using namespace core;
	Queue::Builder builder("Noise");

	// создаём входящее вложение
	auto dataAttachment = builder.addAttachemnt("NoiseDataAttachment", [&] (AttachmentBuilder &attachmentBuilder) -> Rc<Attachment> {
		// определяем как входящее
		attachmentBuilder.defineAsInput();

		// Создаём вложение типа буфер в Vulkan
		auto a = Rc<vk::BufferAttachment>::create(attachmentBuilder, core::BufferInfo(
			core::BufferUsage::UniformBuffer, sizeof(NoiseData)
		));

		// функция проверки входящих данных
		a->setValidateInputCallback([] (const Attachment &, const Rc<AttachmentInputData> &data) {
			return dynamic_cast<NoiseDataInput *>(data.get()) != nullptr;
		});

		// функция создания вложения для кадра
		a->setFrameHandleCallback([] (Attachment &a, const FrameQueue &queue) {
			// создаём вложение для кадра
			auto h = Rc<vk::BufferAttachmentHandle>::create(a, queue);

			// функция заполнения входящих данных
			h->setInputCallback([] (AttachmentHandle &handle, FrameQueue &queue, Rc<AttachmentInputData> &&input, Function<void(bool)> &&cb) {
				// получаем данные из кадра
				auto a = static_cast<vk::BufferAttachment *>(handle.getAttachment().get());
				auto d = static_cast<NoiseDataInput *>(input.get());
				auto devFrame = static_cast<vk::DeviceFrameHandle *>(queue.getFrame().get());
				auto b = static_cast<vk::BufferAttachmentHandle *>(&handle);

				// создаём временный буфер, используя пул памяти кадра
				auto buf = devFrame->getMemPool(devFrame)->spawn(vk::AllocationUsage::DeviceLocalHostVisible, a->getInfo());

				// заполняем буфер
				buf->map([&] (uint8_t *data, VkDeviceSize) {
					memcpy(data, &d->data, sizeof(NoiseData));
				});

				// соединяем буфер с вложением
				b->addBufferView(buf);

				// информируем об успешном завершении
				cb(true);
			});
			return h;
		});

		// Возвращаем созданное вложение
		return a;
	});

	// создаём исходящее вложение
	auto imageAttachment = builder.addAttachemnt("NoiseImageAttachment", [&] (AttachmentBuilder &attachmentBuilder) -> Rc<Attachment> {
		// определяем как исходящее
		attachmentBuilder.defineAsOutput();

		// создаём вложение типа изображения в Vulkan
		return Rc<vk::ImageAttachment>::create(attachmentBuilder,
			ImageInfo(Extent2(1024, 768), ImageUsage::Storage | ImageUsage::TransferSrc, ImageTiling::Optimal, PassType::Compute, ImageFormat::R8G8B8A8_UNORM),
			ImageAttachment::AttachmentInfo{
				.initialLayout = AttachmentLayout::Undefined,
				.finalLayout = AttachmentLayout::General,
				.clearOnLoad = true,
				.clearColor = Color4F(0.0f, 0.0f, 0.0f, 0.0f)}
		);
	});

	// создаём проход рендеринга в очереди
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

void NoiseQueue::run(PlatformApplication *app, NoiseData data, Extent2 extent, StringView target) {
	// запускаем очередь для записи в файл
	run(app->getGlLoop(), data, extent, [app, target = target.str<Interface>()] (core::ImageInfoData info, BytesView view) {
		if (!view.empty()) {
			// сохраняем данные в файл
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
				std::cout << "Written: " << target << "\n";
			}
		}

		// завершаем работу приложения
		app->end();
	});
}

void NoiseQueue::run(core::Loop *loop, NoiseData data, Extent2 extent,
		Function<void(core::ImageInfoData info, BytesView view)> &&cb) {
	// создаём запрос на кадр
	auto req = Rc<core::FrameRequest>::create(Rc<core::Queue>(this), core::FrameConstraints{extent});

	// создаём входящие данные
	auto inputData = Rc<NoiseDataInput>::alloc();
	inputData->data = data;

	// устанавливаем входящие данные для вложения
	req->addInput(getDataAttachment(), move(inputData));

	// устанавливаем функцию для получения результата
	req->setOutput(getImageAttachment(), [loop, cb = sp::move(cb)] (core::FrameAttachmentData &data, bool success, Ref *) mutable {
		loop->captureImage([cb = sp::move(cb)] (core::ImageInfoData info, BytesView view) {
			cb(info, view);
		}, data.image->getImage(), core::AttachmentLayout::General);
		return true;
	});

	// запускаем запрос на кадр
	loop->runRenderQueue(move(req), 0);
}

NoisePass::~NoisePass() { }

bool NoisePass::init(Queue::Builder &queueBuilder, QueuePassBuilder &builder, const AttachmentData *data, const AttachmentData *image) {
	using namespace core;

	// добавляем исходящее вложение к проходу
	auto passImage = builder.addAttachment(image, [] (AttachmentPassBuilder &builder) {
		builder.setDependency(AttachmentDependencyInfo{
			PipelineStage::ComputeShader, AccessType::ShaderWrite,
			PipelineStage::ComputeShader, AccessType::ShaderWrite,
			FrameRenderPassState::Complete,
		});
	});

	// добавляем укладку дескрипторов для шейдера
	auto layout = builder.addDescriptorLayout([&] (PipelineLayoutBuilder &layoutBuilder) {
		layoutBuilder.addSet([&] (DescriptorSetBuilder &setBuilder) {
			setBuilder.addDescriptor(builder.addAttachment(data));
			setBuilder.addDescriptor(passImage, DescriptorType::StorageImage, AttachmentLayout::General);
		});
	});

	// добавляем рабочий подпроход
	builder.addSubpass([&] (SubpassBuilder &subpassBuilder) {
		// добавляем шейдер
		subpassBuilder.addComputePipeline("NoisePipeline", layout,
				queueBuilder.addProgramByRef("NoisePipelineComp", NoiseComp));

		// добавляем функцию командного буфера
		subpassBuilder.setCommandsCallback([&] (const SubpassData &subpass, FrameQueue &frame, CommandBuffer &commands) {
			auto &buf = static_cast<vk::CommandBuffer &>(commands);
			auto imageAttachment = static_cast<vk::ImageAttachmentHandle *>(frame.getAttachment(_imageAttachment)->handle.get());
			auto image = (vk::Image *)imageAttachment->getImage()->getImage().get();
			auto extent = frame.getFrame()->getFrameConstraints().extent;

			// Выполняем перевод изображения к оптимальной укладке
			vk::ImageMemoryBarrier inImageBarriers[] = {
				vk::ImageMemoryBarrier(image, 0, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL)
			};

			buf.cmdPipelineBarrier(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0,
					inImageBarriers);

			// Связываем дескрипторы с укладкой
			buf.cmdBindDescriptorSets(static_cast<vk::RenderPass *>(subpass.pass->impl.get()), 0);

			// Запрашиваем поток исполнения
			auto pipeline = (vk::ComputePipeline *)subpass.computePipelines.get("NoisePipeline")->pipeline.get();

			// Приивязываем поток исполнения
			buf.cmdBindPipeline(pipeline);

			// Запускаем шейдер
			buf.cmdDispatch((extent.width - 1) / pipeline->getLocalX() + 1, (extent.height - 1) / pipeline->getLocalY() + 1);
		});
	});

	_dataAttachment = data;
	_imageAttachment = image;

	return QueuePass::init(builder);
}

}
