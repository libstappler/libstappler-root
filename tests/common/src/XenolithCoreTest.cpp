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

#if MODULE_XENOLITH_CORE && MODULE_XENOLITH_BACKEND_VK

#include "SPValid.h"
#include "SPBitmap.h"
#include "XLCore.h"
#include "XLCoreInfo.h"
#include "XLCoreInput.h"
#include "XLCoreAttachment.h"
#include "XLCoreQueue.h"
#include "XLCoreQueuePass.h"
#include "XLActionEase.h"
#include "XLVk.h"

#include "XLGestureRecognizer.h"
#include "XLInputDispatcher.h"
#include "XLFontLocale.h"
#include "XLFontLabelBase.h"

namespace STAPPLER_VERSIONIZED stappler::app::test {

class TextInterface : public xenolith::TextInputViewInterface {
public:
	virtual void updateTextCursor(uint32_t pos, uint32_t len) { }
	virtual void updateTextInput(WideStringView str, uint32_t pos, uint32_t len, xenolith::TextInputType) { }
	virtual void runTextInput(WideStringView str, uint32_t pos, uint32_t len, xenolith::TextInputType) { }
	virtual void cancelTextInput() { }
};

static bool XenolithCoreTest_core() {
	using namespace xenolith::core;
	StringStream out;

	xenolith::XL_MAKE_API_VERSION("");
	xenolith::XL_MAKE_API_VERSION("1");
	xenolith::XL_MAKE_API_VERSION("1.2");
	xenolith::XL_MAKE_API_VERSION("1.2.3");
	xenolith::XL_MAKE_API_VERSION("1.2.3.4");

	out << getBufferFlagsDescription(BufferFlags(0xFFFF));
	out << getBufferUsageDescription(BufferUsage(0xFFFF));
	out << getImageFlagsDescription(ImageFlags(0xFFFF));
	out << getSampleCountDescription(SampleCount(0xFFFF));
	out << getImageUsageDescription(ImageUsage(0xFFFF));
	out << getInputModifiersNames(InputModifier(0xFFFF));
	out << xenolith::vk::getQueueOperationsDesc(xenolith::vk::QueueOperations(0xFFFF));

	for (size_t i = 0; i < 3; ++ i) {
		out << getImageTypeName(ImageType(i));
	}
	for (size_t i = 0; i < 7; ++ i) {
		out << getImageViewTypeName(ImageViewType(i));
	}
	for (size_t i = 0; i < toInt(ImageFormat::ASTC_12x12_SRGB_BLOCK) + 1; ++ i) {
		out << getImageFormatName(ImageFormat(i)) << ":" << getFormatBlockSize(ImageFormat(i));
	}
	for (size_t i = toInt(ImageFormat::G8B8G8R8_422_UNORM); i < toInt(ImageFormat::A4B4G4R4_UNORM_PACK16_EXT) + 1; ++ i) {
		out << getImageFormatName(ImageFormat(i)) << ":" << getFormatBlockSize(ImageFormat(i));;
	}
	for (size_t i = toInt(ImageFormat::PVRTC1_2BPP_UNORM_BLOCK_IMG); i <= toInt(ImageFormat::PVRTC2_4BPP_SRGB_BLOCK_IMG) + 1; ++ i) {
		out << getImageFormatName(ImageFormat(i)) << ":" << getFormatBlockSize(ImageFormat(i));;
	}
	for (size_t i = toInt(ImageFormat::ASTC_4x4_SFLOAT_BLOCK_EXT); i <= toInt(ImageFormat::ASTC_12x12_SFLOAT_BLOCK_EXT) + 1; ++ i) {
		out << getImageFormatName(ImageFormat(i)) << ":" << getFormatBlockSize(ImageFormat(i));;
	}
	for (size_t i = 0; i < toInt(ImageTiling::Linear) + 1; ++ i) {
		out << getImageTilingName(ImageTiling(i));
	}
	for (size_t i = 0; i < toInt(ComponentMapping::A) + 1; ++ i) {
		out << getComponentMappingName(ComponentMapping(i));
	}
	out << getColorSpaceName(ColorSpace(0));
	for (size_t i = toInt(ColorSpace::DISPLAY_P3_NONLINEAR_EXT); i < toInt(ColorSpace::DISPLAY_NATIVE_AMD) + 1; ++ i) {
		out << getColorSpaceName(ColorSpace(i));
	}
	for (size_t i = 0; i < toInt(InputKeyCode::Max); ++ i) {
		out << getInputKeyCodeName(InputKeyCode(i));
	}
	for (size_t i = 0; i < toInt(InputKeyCode::Max); ++ i) {
		out << getInputKeyCodeKeyName(InputKeyCode(i));
	}
	for (size_t i = 0; i < toInt(InputEventName::Max); ++ i) {
		out << getInputEventName(InputEventName(i));
	}
	for (size_t i = 0; i < toInt(InputMouseButton::Max); ++ i) {
		out << getInputButtonName(InputMouseButton(i));
	}

	getPresentModeName(PresentMode::Mailbox);
	getPresentModeName(PresentMode::FifoRelaxed);
	getPresentModeName(PresentMode::Unsupported);

	BufferInfo bufferInfo1;
	bufferInfo1.flags = BufferFlags::Protected;
	bufferInfo1.persistent = true;
	bufferInfo1.description();
	BufferInfo bufferInfo2;
	bufferInfo2.flags = BufferFlags::None;
	bufferInfo2.usage = BufferUsage::None;
	bufferInfo2.persistent = false;
	bufferInfo2.description();

	BufferData bufferData;
	bufferData.size = 12;
	bufferData.writeData(nullptr, 10);

	Bytes data; data.resize(16);
	BufferData bufferData2;
	bufferData2.size = 16;
	bufferData2.data = data;
	bufferData2.writeData(data.data(), 16);
	bufferData2.data = BytesView();
	bufferData2.memCallback = [&] (uint8_t *, uint64_t, const BufferData::DataCallback &cb) { cb(BytesView(data.data(), 16)); };
	bufferData2.writeData(data.data(), 16);
	bufferData2.memCallback = nullptr;
	bufferData2.stdCallback = [&] (uint8_t *, uint64_t, const BufferData::DataCallback &cb) { cb(BytesView(data.data(), 16)); };
	bufferData2.writeData(data.data(), 16);

	ImageInfo imageInfo1;
	imageInfo1.flags = ImageFlags::None;
	imageInfo1.usage = ImageUsage::None;
	imageInfo1.description();
	ImageInfo imageInfo2;
	imageInfo2.flags = ImageFlags::Protected;
	imageInfo2.description();

	Bytes data2; data2.resize(64);
	ImageData imageData;
	imageData.format = ImageFormat::R8_UNORM;
	imageData.extent = geom::Extent3(4, 4, 4);
	imageData.data = data2;
	imageData.writeData(nullptr, 10);
	imageData.writeData(data2.data(), 64);
	imageData.data = BytesView();
	imageData.memCallback = [&] (uint8_t *, uint64_t, const BufferData::DataCallback &cb) { cb(BytesView(data2.data(), 64)); };
	imageData.writeData(data2.data(), 64);
	imageData.memCallback = nullptr;
	imageData.stdCallback = [&] (uint8_t *, uint64_t, const BufferData::DataCallback &cb) { cb(BytesView(data2.data(), 64)); };
	imageData.writeData(data2.data(), 64);

	ImageViewInfo imageViewInfo;
	imageViewInfo.layerCount = ArrayLayers(1);
	imageViewInfo.setup(imageData);
	imageViewInfo.format = ImageFormat::R8G8B8_UNORM;
	imageViewInfo.setup(ColorMode::SolidColor, true);
	imageViewInfo.setup(ImageType::Image1D, ArrayLayers(2));
	imageViewInfo.setup(ImageType::Image1D, ArrayLayers(1));
	imageViewInfo.setup(ImageType::Image2D, ArrayLayers(2));
	imageViewInfo.setup(ImageType::Image2D, ArrayLayers(1));
	imageViewInfo.setup(ImageType::Image3D, ArrayLayers(1));

	imageViewInfo.format = ImageFormat::ASTC_12x12_SRGB_BLOCK;
	imageViewInfo.getColorMode();
	imageViewInfo.format = ImageFormat::R8_UNORM;
	imageViewInfo.r = ComponentMapping::One;
	imageViewInfo.g = ComponentMapping::One;
	imageViewInfo.b = ComponentMapping::One;
	imageViewInfo.a = ComponentMapping::R;
	imageViewInfo.getColorMode();
	imageViewInfo.format = ImageFormat::R8G8_UNORM;
	imageViewInfo.r = ComponentMapping::R;
	imageViewInfo.g = ComponentMapping::R;
	imageViewInfo.b = ComponentMapping::R;
	imageViewInfo.a = ComponentMapping::G;
	imageViewInfo.getColorMode();
	imageViewInfo.format = ImageFormat::R8G8B8_UNORM;
	imageViewInfo.r = ComponentMapping::Identity;
	imageViewInfo.g = ComponentMapping::Identity;
	imageViewInfo.b = ComponentMapping::Identity;
	imageViewInfo.a = ComponentMapping::One;
	imageViewInfo.getColorMode();
	imageViewInfo.format = ImageFormat::R8G8B8A8_UNORM;
	imageViewInfo.r = ComponentMapping::Identity;
	imageViewInfo.g = ComponentMapping::Identity;
	imageViewInfo.b = ComponentMapping::Identity;
	imageViewInfo.a = ComponentMapping::Identity;
	imageViewInfo.getColorMode();

	imageData.format = ImageFormat::R8G8B8_UNORM;
	imageViewInfo.format = ImageFormat::R8G8B8A8_UNORM;
	imageViewInfo.isCompatible(imageData);

	imageData.format = ImageFormat::R8G8B8A8_UNORM;
	imageData.imageType = ImageType::Image1D;
	imageViewInfo.type = ImageViewType::ImageView1D;
	imageViewInfo.isCompatible(imageData);
	imageData.imageType = ImageType::Image2D;
	imageViewInfo.isCompatible(imageData);

	imageData.imageType = ImageType::Image1D;
	imageViewInfo.type = ImageViewType::ImageView1DArray;
	imageViewInfo.isCompatible(imageData);
	imageData.imageType = ImageType::Image2D;
	imageViewInfo.isCompatible(imageData);

	imageData.imageType = ImageType::Image1D;
	imageViewInfo.type = ImageViewType::ImageView2D;
	imageViewInfo.isCompatible(imageData);
	imageData.imageType = ImageType::Image2D;
	imageViewInfo.isCompatible(imageData);

	imageData.imageType = ImageType::Image1D;
	imageViewInfo.type = ImageViewType::ImageView2DArray;
	imageViewInfo.isCompatible(imageData);
	imageData.imageType = ImageType::Image2D;
	imageViewInfo.isCompatible(imageData);

	imageData.imageType = ImageType::Image1D;
	imageViewInfo.type = ImageViewType::ImageView3D;
	imageViewInfo.isCompatible(imageData);
	imageData.imageType = ImageType::Image3D;
	imageViewInfo.isCompatible(imageData);

	imageData.imageType = ImageType::Image1D;
	imageViewInfo.type = ImageViewType::ImageViewCube;
	imageViewInfo.isCompatible(imageData);
	imageData.imageType = ImageType::Image2D;
	imageViewInfo.isCompatible(imageData);

	imageData.imageType = ImageType::Image1D;
	imageViewInfo.type = ImageViewType::ImageViewCubeArray;
	imageViewInfo.isCompatible(imageData);
	imageData.imageType = ImageType::Image2D;
	imageViewInfo.isCompatible(imageData);

	imageData.arrayLayers = ArrayLayers(2);
	imageViewInfo.baseArrayLayer = BaseArrayLayer(2);
	imageViewInfo.isCompatible(imageData);

	imageViewInfo.baseArrayLayer = BaseArrayLayer(1);
	imageViewInfo.layerCount = ArrayLayers(2);
	imageViewInfo.isCompatible(imageData);

	imageViewInfo.baseArrayLayer = BaseArrayLayer(1);
	imageViewInfo.layerCount = ArrayLayers(1);
	imageViewInfo.isCompatible(imageData);

	imageViewInfo.description();

	SwapchainConfig cfg;
	SurfaceInfo info;

	cfg.presentMode = PresentMode::Mailbox;
	info.isSupported(cfg);

	info.presentModes.emplace_back(PresentMode::Mailbox);
	cfg.presentMode = PresentMode::Immediate;
	info.isSupported(cfg);

	info.presentModes.emplace_back(PresentMode::Immediate);
	cfg.imageFormat = ImageFormat::R8G8B8A8_UNORM;
	cfg.colorSpace = ColorSpace::SRGB_NONLINEAR_KHR;
	info.isSupported(cfg);

	info.formats.emplace_back(pair(ImageFormat::R8G8B8A8_UNORM, ColorSpace::SRGB_NONLINEAR_KHR));
	cfg.alpha = CompositeAlphaFlags::Premultiplied;

	info.isSupported(cfg);
	info.supportedCompositeAlpha = CompositeAlphaFlags::Premultiplied;
	cfg.transform = SurfaceTransformFlags::Rotate90;

	info.isSupported(cfg);
	info.supportedTransforms = SurfaceTransformFlags::Rotate90;
	info.minImageCount = 2;
	info.maxImageCount = 8;
	cfg.imageCount = 1;

	info.isSupported(cfg);
	cfg.imageCount = 2;
	cfg.extent = geom::Extent2(768, 768);

	info.minImageExtent = geom::Extent2(1024, 1024);
	info.maxImageExtent = geom::Extent2(1024, 1024);
	info.isSupported(cfg);

	cfg.extent = geom::Extent2(1024, 1024);
	cfg.transfer = true;
	info.isSupported(cfg);
	info.supportedUsageFlags = ImageUsage::TransferDst;
	info.isSupported(cfg);

	info.description();

	getImagePixelFormat(ImageFormat::Undefined);
	getImagePixelFormat(ImageFormat::S8_UINT);
	getImagePixelFormat(ImageFormat::D16_UNORM_S8_UINT);
	isStencilFormat(ImageFormat::S8_UINT);
	isDepthFormat(ImageFormat::R8G8B8A8_UNORM);

	hasReadAccess(AccessType::IndirectCommandRead);
	hasReadAccess(AccessType::TransferWrite);
	hasWriteAccess(AccessType::TransferWrite);
	hasWriteAccess(AccessType::IndirectCommandRead);

	out << imageData << "\n";

	PipelineMaterialInfo pipelineInfo;
	pipelineInfo.data();

	pipelineInfo.setDepthBounds(DepthBounds());
	pipelineInfo.setDepthBounds(DepthBounds{true});
	pipelineInfo.enableStencil(StencilInfo());
	pipelineInfo.enableStencil(StencilInfo(), StencilInfo());
	pipelineInfo.disableStancil();
	pipelineInfo.setLineWidth(0.0f);
	getInputEventName(InputEventName::Max);

	PipelineMaterialInfo pipelineInfo2{DepthBounds(), StencilInfo()};

	out << InputKeyCode::A << "\n";
	out << InputEventName::Background << "\n";

	for (size_t i = 0; i < toInt(VkFormat::VK_FORMAT_ASTC_12x12_SRGB_BLOCK) + 1; ++ i) {
		out << xenolith::vk::getVkFormatName(VkFormat(i)) << ":" << xenolith::vk::getFormatBlockSize(VkFormat(i));
	}

#ifdef VK_VERSION_1_3
	for (size_t i = toInt(VkFormat::VK_FORMAT_G8B8G8R8_422_UNORM); i < toInt(VkFormat::VK_FORMAT_A8_UNORM_KHR) + 1; ++ i) {
		out << xenolith::vk::getVkFormatName(VkFormat(i)) << ":" << xenolith::vk::getFormatBlockSize(VkFormat(i));
	}
#endif

	out << getColorSpaceName(ColorSpace(0));
	for (size_t i = toInt(VkColorSpaceKHR::VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT); i < toInt(VkColorSpaceKHR::VK_COLOR_SPACE_DISPLAY_NATIVE_AMD) + 1; ++ i) {
		out << xenolith::vk::getVkColorSpaceName(VkColorSpaceKHR(i));
	}
	for (size_t i = 0; i < -VK_ERROR_UNKNOWN + 1; ++ i) {
		out << xenolith::vk::getVkResultName(VkResult(-i));
	}
	for (size_t i = -VK_ERROR_OUT_OF_POOL_MEMORY; i < -VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT + 1; ++ i) {
		out << xenolith::vk::getVkResultName(VkResult(-i));
	}

	auto atlasData1 = valid::makeRandomBytes<Interface>(16);
	auto atlasData2 = valid::makeRandomBytes<Interface>(16);

	auto atlas = Rc<DataAtlas>::create(DataAtlas::Custom, 2, 16);
	atlas->addObject("First", atlasData1.data());
	atlas->addObject("Second", atlasData2.data());

	atlas->getObjectByOrder(0);
	atlas->getObjectByOrder(3);
	atlas->getObjectByName("First");

	auto atlas2 = Rc<DataAtlas>::create(DataAtlas::ImageAtlas, 2, 16);
	atlas2->addObject(1234, atlasData1.data());
	atlas2->addObject(5678, atlasData2.data());
	atlas2->getObjectByName(1234);
	atlas2->getObjectByName(5678);
	atlas2->compile();

	atlas2->getObjectByName(1234);
	atlas2->getObjectByName(5678);

	return true;
}

static xenolith::GestureRecognizer::ButtonMask makeButtonMask() {
	xenolith::GestureRecognizer::ButtonMask ret;
	ret.set(toInt(xenolith::core::InputMouseButton::MouseLeft));
	ret.set(toInt(xenolith::core::InputMouseButton::MouseMiddle));
	ret.set(toInt(xenolith::core::InputMouseButton::MouseRight));
	ret.set(toInt(xenolith::core::InputMouseButton::MouseScrollUp));
	ret.set(toInt(xenolith::core::InputMouseButton::MouseScrollDown));
	ret.set(toInt(xenolith::core::InputMouseButton::MouseScrollLeft));
	ret.set(toInt(xenolith::core::InputMouseButton::MouseScrollRight));
	return ret;
}

static xenolith::InputEventData makeInputEventData(uint32_t id, xenolith::InputEventName name, geom::Vec2 loc) {
	using namespace xenolith;

	InputEventData data;
	data.id = id;
	data.event = name;
	data.button = InputMouseButton::Touch;
	data.x = loc.x;
	data.y = loc.y;
	return data;
}

static xenolith::InputEventData makeInputKeyData(xenolith::InputEventName name, xenolith::InputKeyCode key, char32_t c,
		xenolith::InputKeyComposeState state = xenolith::InputKeyComposeState::Nothing) {
	using namespace xenolith;

	InputEventData data;
	data.id = toInt(key);
	data.event = name;
	data.button = InputMouseButton::Touch;
	data.x = 0.0f;
	data.y = 0.0f;
	data.key.keycode = key;
	data.key.compose = state;
	data.key.keysym = toInt(key);
	data.key.keychar = c;
	return data;
}

static xenolith::InputEventData makeInputKeyData(xenolith::InputEventName name, char32_t c) {
	using namespace xenolith;

	InputEventData data;
	data.id = toInt(InputKeyCode::Unknown);
	data.event = name;
	data.button = InputMouseButton::Touch;
	data.x = 0.0f;
	data.y = 0.0f;
	data.key.keycode = InputKeyCode::Unknown;
	data.key.compose = InputKeyComposeState::Nothing;
	data.key.keysym = c;
	data.key.keychar = c;
	return data;
}

static xenolith::InputEvent makeInputEvent(uint32_t id, xenolith::InputEventName name, geom::Vec2 loc) {
	using namespace xenolith;

	return InputEvent{makeInputEventData(id, name, loc), loc, loc, loc, 0, 0, 0, InputModifier::None, InputModifier::None};
}

static xenolith::InputEvent makeInputKeyEvent(xenolith::InputEventName name, xenolith::InputKeyCode key) {
	using namespace xenolith;

	auto ev = makeInputEvent(toInt(key), name, Vec2(0.0f, 0.0f));
	ev.data.key.keycode = key;
	return ev;
}
static void updateInputEvent(xenolith::InputEvent &event, xenolith::InputEventName name, geom::Vec2 loc) {
	event.previousLocation = event.currentLocation;
	event.previousTime = event.currentTime;
	event.data.event = name;
	event.data.x = loc.x;
	event.data.y = loc.y;
	event.currentLocation = loc;
}

static bool XenolithCoreTest_input() {
	using namespace xenolith;

	GestureScroll scroll;
	scroll.cleanup();
	scroll.location();

	GestureTap tap;
	tap.cleanup();

	GesturePress press;
	press.cleanup();

	GestureSwipe swipe;
	swipe.cleanup();

	GesturePinch pinch;
	pinch.cleanup();

	StringStream out;
	out << GestureEvent::Began;
	out << GestureEvent::Activated;
	out << GestureEvent::Ended;
	out << GestureEvent::Cancelled;

	do {
		Map<uint32_t, InputEvent> inputs;
		auto pinchRec = Rc<GesturePinchRecognizer>::create([] (const GesturePinch &) {

		}, makeButtonMask());

		auto it1 = inputs.emplace(0, makeInputEvent(0, InputEventName::Begin, Vec2(10.0f, 10.0f))).first;
		auto it2 = inputs.emplace(1, makeInputEvent(1, InputEventName::Begin, Vec2(100.0f, 100.0f))).first;

		pinchRec->handleInputEvent(it1->second, 1.0f);
		pinchRec->handleInputEvent(it2->second, 1.0f);

		updateInputEvent(it1->second, InputEventName::Move, Vec2(20.0f, 20.0f));
		updateInputEvent(it2->second, InputEventName::Move, Vec2(90.0f, 90.0f));

		pinchRec->handleInputEvent(it1->second, 1.0f);
		pinchRec->handleInputEvent(it2->second, 1.0f);

		updateInputEvent(it1->second, InputEventName::End, Vec2(30.0f, 30.0f));
		updateInputEvent(it2->second, InputEventName::End, Vec2(80.0f, 80.0f));

		pinchRec->getEventCount();
		pinchRec->hasEvent(it1->second);
		pinchRec->handleInputEvent(it1->second, 1.0f);
		pinchRec->handleInputEvent(it2->second, 1.0f);
		pinchRec->cancel();
		pinchRec->hasEvent(it1->second);
	} while (0);

	do {
		auto rec = Rc<GestureScrollRecognizer>::create([] (const GestureScroll &) { return true; });
		auto ev = makeInputEvent(0, InputEventName::Scroll, Vec2(10.0f, 10.0f));
		rec->handleInputEvent(ev, 1.0f);
	} while (0);

	do {
		auto rec = Rc<GestureMoveRecognizer>::create([] (const GestureData &) { return true; }, false);
		auto ev = makeInputEvent(0, InputEventName::MouseMove, Vec2(10.0f, 10.0f));
		rec->canHandleEvent(ev);
		rec->handleInputEvent(ev, 1.0f);
	} while (0);

	do {
		GestureKeyRecognizer::KeyMask mask;
		mask.set();

		auto rec = Rc<GestureKeyRecognizer>::create([] (const GestureData &) { return true; }, move(mask));
		auto ev = makeInputKeyEvent(InputEventName::KeyPressed, InputKeyCode::ENTER);
		rec->canHandleEvent(ev);
		rec->handleInputEvent(ev, 1.0f);

		ev = makeInputKeyEvent(InputEventName::KeyRepeated, InputKeyCode::ENTER);
		rec->canHandleEvent(ev);
		rec->handleInputEvent(ev, 1.0f);
		rec->isKeyPressed(InputKeyCode::ENTER);

		ev = makeInputKeyEvent(InputEventName::KeyCanceled, InputKeyCode::ENTER);
		rec->canHandleEvent(ev);
		rec->handleInputEvent(ev, 1.0f);
		rec->isKeyPressed(InputKeyCode::ENTER);
	} while (0);

	do {
		Map<uint32_t, InputEvent> inputs;
		auto rec = Rc<GestureSwipeRecognizer>::create([] (const GestureSwipe &) {
			return true;
		}, 25.0f, false, makeButtonMask());

		auto it1 = inputs.emplace(0, makeInputEvent(0, InputEventName::Begin, Vec2(10.0f, 10.0f))).first;
		rec->handleInputEvent(it1->second, 1.0f);

		updateInputEvent(it1->second, InputEventName::Move, Vec2(20.0f, 20.0f));
		rec->handleInputEvent(it1->second, 1.0f);

		auto it2 = inputs.emplace(1, makeInputEvent(1, InputEventName::Begin, Vec2(100.0f, 100.0f))).first;
		rec->handleInputEvent(it2->second, 1.0f);

		updateInputEvent(it1->second, InputEventName::Move, Vec2(-30.0f, -30.0f));
		rec->handleInputEvent(it1->second, 1.0f);

		updateInputEvent(it2->second, InputEventName::Move, Vec2(110.0f, 110.0f));
		rec->handleInputEvent(it2->second, 1.0f);

		updateInputEvent(it1->second, InputEventName::End, Vec2(30.0f, 30.0f));
		rec->handleInputEvent(it1->second, 1.0f);

		updateInputEvent(it2->second, InputEventName::Move, Vec2(120.0f, 120.0f));
		rec->handleInputEvent(it2->second, 1.0f);

		updateInputEvent(it2->second, InputEventName::End, Vec2(120.0f, 120.0f));
		rec->handleInputEvent(it2->second, 1.0f);
		rec->cancel();
	} while (0);

	do {
		uint64_t ival = 51'000;
		Map<uint32_t, InputEvent> inputs;
		auto rec = Rc<GesturePressRecognizer>::create([] (const GesturePress &) {
			return true;
		}, TimeInterval::milliseconds(50), true, makeButtonMask());

		auto it1 = inputs.emplace(0, makeInputEvent(0, InputEventName::Begin, Vec2(10.0f, 10.0f))).first;
		rec->handleInputEvent(it1->second, 1.0f);

		updateInputEvent(it1->second, InputEventName::Move, Vec2(11.0f, 11.0f));
		rec->handleInputEvent(it1->second, 1.0f);

		xenolith::platform::sleep(51'000);
		rec->update(ival += 51'000);
		xenolith::platform::sleep(51'000);
		rec->update(ival += 51'000);
		xenolith::platform::sleep(51'000);
		rec->update(ival += 51'000);
		xenolith::platform::sleep(51'000);
		rec->update(ival += 51'000);

		updateInputEvent(it1->second, InputEventName::End, Vec2(11.0f, 11.0f));
		rec->handleInputEvent(it1->second, 1.0f);
		rec->cancel();

		inputs.clear();
		it1 = inputs.emplace(0, makeInputEvent(0, InputEventName::Begin, Vec2(10.0f, 10.0f))).first;
		rec->handleInputEvent(it1->second, 1.0f);

		updateInputEvent(it1->second, InputEventName::Move, Vec2(110.0f, 110.0f));
		rec->handleInputEvent(it1->second, 1.0f);

		updateInputEvent(it1->second, InputEventName::End, Vec2(110.0f, 110.0f));
		rec->handleInputEvent(it1->second, 1.0f);
		rec->cancel();
	} while (0);


	do {
		Map<uint32_t, InputEvent> inputs;
		auto rec = Rc<GestureTapRecognizer>::create([] (const GestureTap &) {
			return true;
		}, makeButtonMask(), 2);

		auto it1 = inputs.emplace(0, makeInputEvent(0, InputEventName::Begin, Vec2(10.0f, 10.0f))).first;
		rec->handleInputEvent(it1->second, 1.0f);

		updateInputEvent(it1->second, InputEventName::Move, Vec2(11.0f, 11.0f));
		rec->handleInputEvent(it1->second, 1.0f);

		updateInputEvent(it1->second, InputEventName::End, Vec2(11.0f, 11.0f));
		rec->handleInputEvent(it1->second, 1.0f);

		xenolith::platform::sleep(500'000);

		inputs.clear();
		it1 = inputs.emplace(0, makeInputEvent(0, InputEventName::Begin, Vec2(10.0f, 10.0f))).first;
		rec->handleInputEvent(it1->second, 1.0f);

		updateInputEvent(it1->second, InputEventName::Move, Vec2(11.0f, 11.0f));
		rec->handleInputEvent(it1->second, 1.0f);

		updateInputEvent(it1->second, InputEventName::End, Vec2(11.0f, 11.0f));
		rec->handleInputEvent(it1->second, 1.0f);
		rec->cancel();
	} while (0);

	do {
		Map<uint32_t, InputEvent> inputs;
		auto rec = Rc<GestureTouchRecognizer>::create([] (const GestureData &data) {
			if (data.location() == Vec2(12.0f, 12.0f)) {
				return false;
			}
			return true;
		}, makeButtonMask());

		rec->getMaxEvents();
		rec->setMaxEvents(2);

		auto it1 = inputs.emplace(0, makeInputEvent(0, InputEventName::Begin, Vec2(10.0f, 10.0f))).first;
		rec->handleInputEvent(it1->second, 1.0f);

		updateInputEvent(it1->second, InputEventName::Move, Vec2(12.0f, 12.0f));
		rec->handleInputEvent(it1->second, 1.0f);

		updateInputEvent(it1->second, InputEventName::End, Vec2(11.0f, 11.0f));
		rec->handleInputEvent(it1->second, 1.0f);

		xenolith::platform::sleep(500'000);

		inputs.clear();
		it1 = inputs.emplace(0, makeInputEvent(0, InputEventName::Begin, Vec2(10.0f, 10.0f))).first;
		rec->handleInputEvent(it1->second, 1.0f);
		rec->getLocation();

		updateInputEvent(it1->second, InputEventName::Move, Vec2(11.0f, 11.0f));
		rec->handleInputEvent(it1->second, 1.0f);

		updateInputEvent(it1->second, InputEventName::Cancel, Vec2(11.0f, 11.0f));
		rec->handleInputEvent(it1->second, 1.0f);
		inputs.clear();

		it1 = inputs.emplace(0, makeInputEvent(0, InputEventName::Begin, Vec2(12.0f, 12.0f))).first;
		rec->handleInputEvent(it1->second, 1.0f);

		updateInputEvent(it1->second, InputEventName::Move, Vec2(11.0f, 11.0f));
		rec->handleInputEvent(it1->second, 1.0f);

		updateInputEvent(it1->second, InputEventName::Cancel, Vec2(11.0f, 11.0f));
		rec->handleInputEvent(it1->second, 1.0f);
		rec->cancel();
	} while (0);

	do {
		auto rec = Rc<GestureRecognizer>::create();
		rec = nullptr;
	} while (0);

	do {
		TextInterface iface;
		auto pool = Rc<PoolRef>::alloc();
		auto d = Rc<InputDispatcher>::create(pool, &iface);

		auto ev = d->acquireNewStorage();

		GestureKeyRecognizer::KeyMask keyMask;
		keyMask.set();

		auto l1 = Rc<InputListener>::create(-1);
		l1->addTouchRecognizer([l1] (const GestureData &data) {
			if (data.location() == Vec2(12.0f, 12.0f)) {
				l1->setExclusiveForTouch(data.input->data.id);
			}
			return true;
		}, makeButtonMask());
		l1->addKeyRecognizer([] (const GestureData &) {
			return true;
		}, move(keyMask));

		auto l2 = Rc<InputListener>::create(-3);
		l2->addTouchRecognizer([] (const GestureData &data) {
			if (data.location() == Vec2(12.0f, 12.0f)) {
				return false;
			}
			return true;
		}, makeButtonMask());
		l2->addKeyRecognizer([l2] (const GestureData &data) {
			if (data.input->data.key.keychar == 'B') {
				l2->setExclusive();
			}
			return true;
		}, move(keyMask));

		auto l3 = Rc<InputListener>::create(-2);
		auto l4 = Rc<InputListener>::create(1);
		l4->addPinchRecognizer([] (const GesturePinch &) {
			return true;
		});
		auto l5 = Rc<InputListener>::create(3);
		l5->setPointerEnterCallback([] (bool) { return true; });
		l5->setBackgroudCallback([] (bool) { return true; });
		l5->setFocusCallback([] (bool) { return true; });

		auto l6 = Rc<InputListener>::create(2);
		l6->addScrollRecognizer([] (const GestureScroll &) {
			return true;
		});
		l6->addMouseOverRecognizer([] (const GestureData &) {
			return true;
		});

		ev->addListener(l1, 0);
		ev->addListener(l2, 0);
		ev->addListener(l3, 0);
		ev->addListener(l4, 0);
		ev->addListener(l5, 0);
		ev->addListener(l6, 0);

		auto mngr = d->getTextInputManager();
		d->commitStorage(move(ev));
		mngr->setInputEnabled(true);

		TextInputHandler handler;
		handler.run(mngr, u"Test", TextCursor(), TextCursor(), TextInputType::Text);
		handler.setString(u"ASDF",  TextCursor(), TextCursor());
		handler.setCursor(TextCursor(1, 2));
		handler.setMarked(TextCursor(1, 1));

		handler.getString();
		handler.getCursor();
		handler.getMarked();
		handler.isInputEnabled();
		handler.isActive();

		mngr->insertText(u"WA1234SD", TextCursor(1, 2));
		mngr->setMarkedText(u"WA1234SD", TextCursor(1, 2), TextCursor(1, 2));
		mngr->cursorChanged(TextCursor(2, 2));
		mngr->deleteBackward();
		mngr->deleteBackward();

		mngr->insertText(u"WA1234SD", TextCursor(1, 2));
		mngr->cursorChanged(TextCursor(2, 2));
		mngr->deleteForward();
		mngr->deleteForward();

		mngr->unmarkText();
		mngr->textChanged(WideStringView(), TextCursor(), TextCursor());
		mngr->textChanged(u"WA1234SD", TextCursor(), TextCursor());
		mngr->getStringByRange(TextCursor(2, 2));

		d->handleInputEvent(makeInputKeyData(InputEventName::KeyPressed, InputKeyCode::A, 'A'));
		d->handleInputEvent(makeInputKeyData(InputEventName::KeyRepeated, InputKeyCode::A, 'A'));
		d->handleInputEvent(makeInputKeyData(InputEventName::KeyReleased, InputKeyCode::A, 'A'));

		d->handleInputEvent(makeInputKeyData(InputEventName::KeyPressed, InputKeyCode::C, 'C'));
		d->handleInputEvent(makeInputKeyData(InputEventName::KeyRepeated, InputKeyCode::C, 'C'));
		d->handleInputEvent(makeInputKeyData(InputEventName::KeyRepeated, InputKeyCode::C, 'C'));
		d->handleInputEvent(makeInputKeyData(InputEventName::KeyRepeated, InputKeyCode::C, 'C'));
		d->handleInputEvent(makeInputKeyData(InputEventName::KeyRepeated, InputKeyCode::C, 'C'));
		d->handleInputEvent(makeInputKeyData(InputEventName::KeyRepeated, InputKeyCode::C, 'C'));
		d->handleInputEvent(makeInputKeyData(InputEventName::KeyReleased, InputKeyCode::C, 'C'));

		d->handleInputEvent(makeInputKeyData(InputEventName::KeyPressed, InputKeyCode::DELETE, 0));
		d->handleInputEvent(makeInputKeyData(InputEventName::KeyReleased, InputKeyCode::DELETE, 0));
		d->handleInputEvent(makeInputKeyData(InputEventName::KeyPressed, InputKeyCode::BACKSPACE, 0));
		d->handleInputEvent(makeInputKeyData(InputEventName::KeyReleased, InputKeyCode::BACKSPACE, 0));
		d->handleInputEvent(makeInputKeyData(InputEventName::KeyPressed, InputKeyCode::ESCAPE, 0));
		d->handleInputEvent(makeInputKeyData(InputEventName::KeyReleased, InputKeyCode::ESCAPE, 0));

		handler.cancel();
		handler.run(mngr, u"Test", TextCursor(), TextCursor(), TextInputType::Text);
		mngr->setInputEnabled(true);

		d->handleInputEvent(makeInputKeyData(InputEventName::KeyPressed, InputKeyCode::C, 'C'));
		d->handleInputEvent(makeInputKeyData(InputEventName::KeyRepeated, InputKeyCode::C, 'C', InputKeyComposeState::Composing));
		d->handleInputEvent(makeInputKeyData(InputEventName::KeyRepeated, InputKeyCode::C, 'C', InputKeyComposeState::Composed));
		d->handleInputEvent(makeInputKeyData(InputEventName::KeyRepeated, InputKeyCode::C, 'C'));
		d->handleInputEvent(makeInputKeyData(InputEventName::KeyRepeated, InputKeyCode::C, 'C'));
		d->handleInputEvent(makeInputKeyData(InputEventName::KeyRepeated, InputKeyCode::C, 'C'));
		d->handleInputEvent(makeInputKeyData(InputEventName::KeyReleased, InputKeyCode::C, 'C', InputKeyComposeState::Forced));
		handler.cancel();

		d->handleInputEvent(makeInputKeyData(InputEventName::KeyPressed, InputKeyCode::A, 'A'));
		d->handleInputEvent(makeInputKeyData(InputEventName::KeyRepeated, InputKeyCode::A, 'A'));
		d->setListenerExclusive(l2);
		d->handleInputEvent(makeInputKeyData(InputEventName::KeyReleased, InputKeyCode::A, 'A'));

		d->handleInputEvent(makeInputKeyData(InputEventName::KeyPressed, InputKeyCode::A, 'A'));
		d->handleInputEvent(makeInputKeyData(InputEventName::KeyRepeated, InputKeyCode::A, 'A'));
		d->setListenerExclusiveForKey(l2, InputKeyCode::A);
		d->handleInputEvent(makeInputKeyData(InputEventName::KeyReleased, InputKeyCode::A, 'A'));

		d->handleInputEvent(makeInputEventData(0, InputEventName::Begin, Vec2(1.0f, 1.0f)));
		d->handleInputEvent(makeInputEventData(0, InputEventName::Begin, Vec2(1.0f, 1.0f)));
		d->handleInputEvent(makeInputEventData(0, InputEventName::Move, Vec2(2.0f, 2.0f)));
		d->getActiveEvents();
		d->setListenerExclusive(l2);
		d->handleInputEvent(makeInputEventData(0, InputEventName::MouseMove, Vec2(2.0f, 2.0f)));
		d->handleInputEvent(makeInputEventData(0, InputEventName::Cancel, Vec2(2.0f, 2.0f)));

		d->handleInputEvent(makeInputEventData(0, InputEventName::Begin, Vec2(1.0f, 1.0f)));
		d->handleInputEvent(makeInputEventData(0, InputEventName::Move, Vec2(12.0f, 12.0f)));
		d->setListenerExclusiveForTouch(l2, 0);
		d->handleInputEvent(makeInputEventData(0, InputEventName::Cancel, Vec2(2.0f, 2.0f)));

		d->handleInputEvent(makeInputEventData(0, InputEventName::Scroll, Vec2(2.0f, 2.0f)));

		d->handleInputEvent(InputEventData::BoolEvent(InputEventName::Background, true));
		d->handleInputEvent(InputEventData::BoolEvent(InputEventName::Background, false));
		d->handleInputEvent(InputEventData::BoolEvent(InputEventName::PointerEnter, true));
		d->handleInputEvent(makeInputEventData(0, InputEventName::Begin, Vec2(1.0f, 1.0f)));
		d->handleInputEvent(makeInputEventData(0, InputEventName::MouseMove, Vec2(2.0f, 2.0f)));
		d->handleInputEvent(InputEventData::BoolEvent(InputEventName::PointerEnter, false));
		d->handleInputEvent(InputEventData::BoolEvent(InputEventName::FocusGain, true));

		d->handleInputEvent(makeInputKeyData(InputEventName::KeyPressed, 'B'));
		d->handleInputEvent(makeInputKeyData(InputEventName::KeyPressed, 'B'));
		d->handleInputEvent(makeInputKeyData(InputEventName::KeyRepeated, 'B'));
		d->handleInputEvent(makeInputKeyData(InputEventName::KeyReleased, 'B'));
		d->handleInputEvent(makeInputKeyData(InputEventName::KeyPressed, 'B'));
		d->handleInputEvent(makeInputKeyData(InputEventName::KeyRepeated, 'B'));
		d->handleInputEvent(makeInputKeyData(InputEventName::KeyCanceled, 'B'));

		d->hasActiveInput();
		d->isInBackground();
		d->isPointerWithinWindow();
		d->hasFocus();

		l5->setPointerEnterCallback(nullptr);
		l5->setBackgroudCallback(nullptr);
		l5->setFocusCallback(nullptr);
		l6->clear();

		l3->isSwallowAllEvents();
		l3->isSwallowAllEvents(InputListener::makeEventMask({InputEventName::Begin, InputEventName::Move, InputEventName::End}));
		l3->isSwallowAnyEvents(InputListener::makeEventMask({InputEventName::Begin, InputEventName::Move, InputEventName::End}));
		l3->isSwallowEvent(InputEventName::Begin);
		l3->clearSwallowEvents(InputListener::makeEventMask({InputEventName::Begin, InputEventName::Move, InputEventName::End}));
		l3->clearSwallowEvent(InputEventName::Begin);
		l3->setSwallowEvent(InputEventName::Begin);
		l3->setSwallowEvents(InputListener::makeEventMask({InputEventName::Begin, InputEventName::Move, InputEventName::End}));
		l3->setSwallowAllEvents();
		l3->clearSwallowAllEvents();

	} while (0);

	return true;
}

static bool XenolithCoreTest_action() {
	using namespace xenolith;

	float program[8] = { 0.3f, 0.3f, 0.3f, 0.3f, 0.3f, 0.3f, 0.3f, 0.3f };

	interpolation::interpolateTo(0.5f, interpolation::Type::Linear, nullptr);

	interpolation::interpolateTo(0.5f, interpolation::Type::EaseIn, nullptr);
	interpolation::interpolateTo(0.5f, interpolation::Type::EaseOut, nullptr);
	interpolation::interpolateTo(0.5f, interpolation::Type::EaseInOut, nullptr);

	interpolation::interpolateTo(0.5f, interpolation::Type::Sine_EaseIn, nullptr);
	interpolation::interpolateTo(0.5f, interpolation::Type::Sine_EaseOut, nullptr);
	interpolation::interpolateTo(0.5f, interpolation::Type::Sine_EaseInOut, nullptr);

	interpolation::interpolateTo(0.5f, interpolation::Type::Quad_EaseIn, nullptr);
	interpolation::interpolateTo(0.5f, interpolation::Type::Quad_EaseOut, nullptr);
	interpolation::interpolateTo(0.5f, interpolation::Type::Quad_EaseInOut, nullptr);

	interpolation::interpolateTo(0.5f, interpolation::Type::Cubic_EaseIn, nullptr);
	interpolation::interpolateTo(0.5f, interpolation::Type::Cubic_EaseOut, nullptr);
	interpolation::interpolateTo(0.5f, interpolation::Type::Cubic_EaseInOut, nullptr);

	interpolation::interpolateTo(0.5f, interpolation::Type::Quart_EaseIn, nullptr);
	interpolation::interpolateTo(0.5f, interpolation::Type::Quart_EaseOut, nullptr);
	interpolation::interpolateTo(0.5f, interpolation::Type::Quart_EaseInOut, nullptr);

	interpolation::interpolateTo(0.5f, interpolation::Type::Quint_EaseIn, nullptr);
	interpolation::interpolateTo(0.5f, interpolation::Type::Quint_EaseOut, nullptr);
	interpolation::interpolateTo(0.5f, interpolation::Type::Quint_EaseInOut, nullptr);

	interpolation::interpolateTo(0.5f, interpolation::Type::Expo_EaseIn, nullptr);
	interpolation::interpolateTo(0.5f, interpolation::Type::Expo_EaseOut, nullptr);
	interpolation::interpolateTo(0.5f, interpolation::Type::Expo_EaseInOut, nullptr);

	interpolation::interpolateTo(0.5f, interpolation::Type::Circ_EaseIn, nullptr);
	interpolation::interpolateTo(0.5f, interpolation::Type::Circ_EaseOut, nullptr);
	interpolation::interpolateTo(0.5f, interpolation::Type::Circ_EaseInOut, nullptr);

	interpolation::interpolateTo(0.5f, interpolation::Type::Elastic_EaseIn, program);
	interpolation::interpolateTo(0.5f, interpolation::Type::Elastic_EaseOut, program);
	interpolation::interpolateTo(0.5f, interpolation::Type::Elastic_EaseInOut, program);

	interpolation::interpolateTo(0.5f, interpolation::Type::Back_EaseIn, nullptr);
	interpolation::interpolateTo(0.5f, interpolation::Type::Back_EaseOut, nullptr);
	interpolation::interpolateTo(0.5f, interpolation::Type::Back_EaseInOut, nullptr);

	interpolation::interpolateTo(0.5f, interpolation::Type::Bounce_EaseIn, nullptr);
	interpolation::interpolateTo(0.5f, interpolation::Type::Bounce_EaseOut, nullptr);
	interpolation::interpolateTo(0.5f, interpolation::Type::Bounce_EaseInOut, nullptr);

	interpolation::interpolateTo(0.5f, interpolation::Type::Custom, program);
	interpolation::interpolateTo(0.5f, interpolation::Type::Custom, nullptr);
	interpolation::interpolateTo(0.5f, interpolation::Type::Max, nullptr);

	interpolation::easeIn(0.5f, 2.0f);
	interpolation::easeOut(0.5f, 2.0f);
	interpolation::easeInOut(0.5f, 2.0f);

	return true;
}

static Bytes makeData(BytesView bytes, bitmap::PixelFormat fmt) {
	Bitmap out(bytes);
	out.convert(fmt);
	return out.write(bitmap::FileFormat::Png);
}

static void testImageLoader() {
	using namespace xenolith::core;

	auto png1 = filesystem::currentDir<Interface>("resources/1.png");
	Bytes imageData = filesystem::readIntoMemory<Interface>(png1);
	Bitmap bmp(imageData);

	auto rgbaData = makeData(imageData, bitmap::PixelFormat::RGBA8888);
	auto rgbData = makeData(imageData, bitmap::PixelFormat::RGB888);
	auto iaData = makeData(imageData, bitmap::PixelFormat::IA88);
	auto aData = makeData(imageData, bitmap::PixelFormat::A8);

	Bytes bytes; bytes.resize(bmp.width() * bmp.height() * 4);

	Resource::loadImageMemoryData(nullptr, 0, rgbaData, ImageFormat::R8G8B8A8_UNORM, [] (BytesView) { });
	Resource::loadImageMemoryData(nullptr, 0, rgbaData, ImageFormat::R8G8B8_UNORM, [] (BytesView) { });
	Resource::loadImageMemoryData(nullptr, 0, rgbaData, ImageFormat::R8G8_UNORM, [] (BytesView) { });
	Resource::loadImageMemoryData(nullptr, 0, rgbaData, ImageFormat::R8_UNORM, [] (BytesView) { });

	Resource::loadImageMemoryData(nullptr, 0, rgbData, ImageFormat::R8G8B8A8_UNORM, [] (BytesView) { });
	Resource::loadImageMemoryData(nullptr, 0, rgbData, ImageFormat::R8G8B8_UNORM, [] (BytesView) { });
	Resource::loadImageMemoryData(nullptr, 0, rgbData, ImageFormat::R8G8_UNORM, [] (BytesView) { });
	Resource::loadImageMemoryData(nullptr, 0, rgbData, ImageFormat::R8_UNORM, [] (BytesView) { });

	Resource::loadImageMemoryData(nullptr, 0, iaData, ImageFormat::R8G8B8A8_UNORM, [] (BytesView) { });
	Resource::loadImageMemoryData(nullptr, 0, iaData, ImageFormat::R8G8B8_UNORM, [] (BytesView) { });
	Resource::loadImageMemoryData(nullptr, 0, iaData, ImageFormat::R8G8_UNORM, [] (BytesView) { });
	Resource::loadImageMemoryData(nullptr, 0, iaData, ImageFormat::R8_UNORM, [] (BytesView) { });

	Resource::loadImageMemoryData(nullptr, 0, aData, ImageFormat::R8G8B8A8_UNORM, [] (BytesView) { });
	Resource::loadImageMemoryData(nullptr, 0, aData, ImageFormat::R8G8B8_UNORM, [] (BytesView) { });
	Resource::loadImageMemoryData(nullptr, 0, aData, ImageFormat::R8G8_UNORM, [] (BytesView) { });
	Resource::loadImageMemoryData(nullptr, 0, aData, ImageFormat::R8_UNORM, [] (BytesView) { });


	Resource::loadImageMemoryData(bytes.data(), bytes.size(), rgbaData, ImageFormat::R8G8B8A8_UNORM, [] (BytesView) { });
	Resource::loadImageMemoryData(bytes.data(), bytes.size(), rgbaData, ImageFormat::R8G8B8_UNORM, [] (BytesView) { });
	Resource::loadImageMemoryData(bytes.data(), bytes.size(), rgbaData, ImageFormat::R8G8_UNORM, [] (BytesView) { });
	Resource::loadImageMemoryData(bytes.data(), bytes.size(), rgbaData, ImageFormat::R8_UNORM, [] (BytesView) { });

	Resource::loadImageMemoryData(bytes.data(), bytes.size(), rgbData, ImageFormat::R8G8B8A8_UNORM, [] (BytesView) { });
	Resource::loadImageMemoryData(bytes.data(), bytes.size(), rgbData, ImageFormat::R8G8B8_UNORM, [] (BytesView) { });
	Resource::loadImageMemoryData(bytes.data(), bytes.size(), rgbData, ImageFormat::R8G8_UNORM, [] (BytesView) { });
	Resource::loadImageMemoryData(bytes.data(), bytes.size(), rgbData, ImageFormat::R8_UNORM, [] (BytesView) { });

	Resource::loadImageMemoryData(bytes.data(), bytes.size(), iaData, ImageFormat::R8G8B8A8_UNORM, [] (BytesView) { });
	Resource::loadImageMemoryData(bytes.data(), bytes.size(), iaData, ImageFormat::R8G8B8_UNORM, [] (BytesView) { });
	Resource::loadImageMemoryData(bytes.data(), bytes.size(), iaData, ImageFormat::R8G8_UNORM, [] (BytesView) { });
	Resource::loadImageMemoryData(bytes.data(), bytes.size(), iaData, ImageFormat::R8_UNORM, [] (BytesView) { });

	Resource::loadImageMemoryData(bytes.data(), bytes.size(), aData, ImageFormat::R8G8B8A8_UNORM, [] (BytesView) { });
	Resource::loadImageMemoryData(bytes.data(), bytes.size(), aData, ImageFormat::R8G8B8_UNORM, [] (BytesView) { });
	Resource::loadImageMemoryData(bytes.data(), bytes.size(), aData, ImageFormat::R8G8_UNORM, [] (BytesView) { });
	Resource::loadImageMemoryData(bytes.data(), bytes.size(), aData, ImageFormat::R8_UNORM, [] (BytesView) { });
}

static Rc<xenolith::core::Resource> makeResource() {
	using namespace xenolith::core;

	auto png1 = filesystem::currentDir<Interface>("resources/1.png");
	Bytes imageData = filesystem::readIntoMemory<Interface>(png1);

	auto path = filesystem::currentDir<Interface>("resources/mnist/t10k-labels.idx1-ubyte");
	Bytes bytes = valid::makeRandomBytes<Interface>(128);

	Resource::Builder builder("Resource");

	builder.addBufferByRef("Buffer1", BufferInfo{BufferUsage::UniformBuffer, bytes.size()}, bytes);
	builder.addBufferByRef("Buffer1", BufferInfo{BufferUsage::UniformBuffer, bytes.size()}, bytes);

	builder.addBuffer("Buffer2", BufferInfo{BufferUsage::UniformBuffer, bytes.size()}, FilePath(path));
	builder.addBuffer("Buffer2", BufferInfo{BufferUsage::UniformBuffer, bytes.size()}, FilePath(path));

	builder.addBuffer("Buffer3", BufferInfo{BufferUsage::UniformBuffer, bytes.size()}, BytesView(bytes));
	builder.addBuffer("Buffer3", BufferInfo{BufferUsage::UniformBuffer, bytes.size()}, BytesView(bytes));

	builder.addBuffer("Buffer4", BufferInfo{BufferUsage::UniformBuffer, bytes.size()},
			[] (uint8_t *, uint64_t, const BufferData::DataCallback &) { });
	builder.addBuffer("Buffer4", BufferInfo{BufferUsage::UniformBuffer, bytes.size()},
			[] (uint8_t *, uint64_t, const BufferData::DataCallback &) { });

	builder.addImageByRef("Image1", ImageInfo{ImageUsage::Sampled}, imageData);
	builder.addImageByRef("Image1", ImageInfo{ImageUsage::Sampled}, imageData);

	builder.addImage("Image2", ImageInfo{ImageUsage::Sampled}, FilePath(png1));
	builder.addImage("Image2", ImageInfo{ImageUsage::Sampled}, FilePath(png1));
	builder.addImage("Image5", ImageInfo{ImageUsage::Sampled}, FilePath("resources/1.png"));

	builder.addImage("Image3", ImageInfo{ImageUsage::Sampled}, BytesView(imageData));
	builder.addImage("Image3", ImageInfo{ImageUsage::Sampled}, BytesView(imageData));

	builder.addImage("Image4", ImageInfo{ImageUsage::Sampled},
			[] (uint8_t *, uint64_t, const BufferData::DataCallback &) { });
	builder.addImage("Image4", ImageInfo{ImageUsage::Sampled},
			[] (uint8_t *, uint64_t, const BufferData::DataCallback &) { });

	auto res = Rc<Resource>::create(move(builder));
	res->getPool();
	res->getBuffer("Buffer2");
	return res;
}

static bool XenolithCoreTest_queue() {
	using namespace xenolith::core;

	auto frag = filesystem::currentDir<Interface>("resources/xl_2d_material.frag");
	auto vert = filesystem::currentDir<Interface>("resources/xl_2d_material.vert");
	auto png1 = filesystem::currentDir<Interface>("resources/1.png");
	auto path = filesystem::currentDir<Interface>("resources/mnist/t10k-labels.idx1-ubyte");

	Bytes imageData = filesystem::readIntoMemory<Interface>(png1);
	Bytes fragData = filesystem::readIntoMemory<Interface>(frag);
	Bytes vertData = filesystem::readIntoMemory<Interface>(vert);
	Bytes bytes = valid::makeRandomBytes<Interface>(128);

	Queue::Builder builder("Noise");

	auto res = makeResource();
	builder.addLinkedResource(res);
	res->setCompiled(true);
	builder.addLinkedResource(res);

	auto buf1 = builder.addBufferByRef("Buffer1", BufferInfo{BufferUsage::UniformBuffer, bytes.size()}, bytes);
	builder.addBufferByRef("Buffer1", BufferInfo{BufferUsage::UniformBuffer, bytes.size()}, bytes);

	builder.addBuffer("Buffer2", BufferInfo{BufferUsage::UniformBuffer, bytes.size()}, FilePath(path));
	builder.addBuffer("Buffer2", BufferInfo{BufferUsage::UniformBuffer, bytes.size()}, FilePath(path));

	builder.addBuffer("Buffer3", BufferInfo{BufferUsage::UniformBuffer, bytes.size()}, BytesView(bytes));
	builder.addBuffer("Buffer3", BufferInfo{BufferUsage::UniformBuffer, bytes.size()}, BytesView(bytes));

	builder.addBuffer("Buffer4", BufferInfo{BufferUsage::UniformBuffer, bytes.size()},
			[] (uint8_t *, uint64_t, const BufferData::DataCallback &) { });
	builder.addBuffer("Buffer4", BufferInfo{BufferUsage::UniformBuffer, bytes.size()},
			[] (uint8_t *, uint64_t, const BufferData::DataCallback &) { });

	auto image1 = builder.addImageByRef("Image1", ImageInfo{ImageUsage::Sampled}, imageData);
	builder.addImageByRef("Image1", ImageInfo{ImageUsage::Sampled}, imageData);

	auto image2 = builder.addImage("Image2", ImageInfo{ImageUsage::Sampled}, FilePath(png1));
	builder.addImage("Image2", ImageInfo{ImageUsage::Sampled}, FilePath(png1));

	auto image3 = builder.addImage("Image3", ImageInfo{ImageUsage::Sampled}, BytesView(imageData));
	builder.addImage("Image3", ImageInfo{ImageUsage::Sampled}, BytesView(imageData));

	builder.addImage("Image4", ImageInfo{ImageUsage::Sampled},
			[] (uint8_t *, uint64_t, const BufferData::DataCallback &) { });
	builder.addImage("Image4", ImageInfo{ImageUsage::Sampled},
			[] (uint8_t *, uint64_t, const BufferData::DataCallback &) { });

	ProgramInfo info1;
	info1.stage = ProgramStage::Fragment;
	info1.bindings = memory::vector<ProgramDescriptorBinding>{
		ProgramDescriptorBinding{0, 0, DescriptorType::SampledImage, 0}
	};
	info1.constants = memory::vector<ProgramPushConstantBlock>{
		ProgramPushConstantBlock{0, 0}
	};
	info1.entryPoints = memory::vector<ProgramEntryPointBlock>{
		ProgramEntryPointBlock{0, "name", 1, 1, 1}
	};

	builder.addProgram("program0", [vertData] (const ProgramData::DataCallback &cb) {
		cb(SpanView<uint32_t>((const uint32_t *)vertData.data(), vertData.size() / sizeof(uint32_t)));
	}, &info1);

	builder.setDefaultSyncPassState(FrameRenderPassState::Submitted);
	builder.addProgram("program1", SpanView<uint32_t>(), &info1);
	builder.addProgramByRef("program2", SpanView<uint32_t>(), &info1);
	builder.addProgram("program3", [] (const ProgramData::DataCallback &) { }, &info1);
	builder.addProgram("program1", SpanView<uint32_t>(), &info1);
	builder.addProgramByRef("program2", SpanView<uint32_t>(), &info1);
	builder.addProgram("program3", [] (const ProgramData::DataCallback &) { }, &info1);

	auto fragProgram = builder.addProgram("program4", SpanView<uint32_t>((const uint32_t *)fragData.data(), fragData.size() / sizeof(uint32_t)), &info1);
	builder.addProgramByRef("program5", SpanView<uint32_t>((const uint32_t *)fragData.data(), fragData.size() / sizeof(uint32_t)), &info1);
	auto vertProgram = builder.addProgram("program6", [vertData] (const ProgramData::DataCallback &cb) {
		cb(SpanView<uint32_t>((const uint32_t *)vertData.data(), vertData.size() / sizeof(uint32_t)));
	});

	auto a1 = builder.addAttachemnt("Attachment1", [&] (AttachmentBuilder &b) -> Rc<Attachment> {
		b.defineAsOutput();
		auto a = Rc<ImageAttachment>::create(b, image1, ImageAttachment::AttachmentInfo({
			AttachmentLayout::ShaderReadOnlyOptimal,
			AttachmentLayout::TransferSrcOptimal
		}));
		a->isCompatible(*image2);
		a->setInputCallback([] (FrameQueue &, const Rc<AttachmentHandle> &, Function<void(bool)> &&) { });
		a->getName();
		return a;
	});

	auto a2 = builder.addAttachemnt("Attachment2", [&] (AttachmentBuilder &b) -> Rc<Attachment> {
		b.defineAsOutput();
		auto a = Rc<BufferAttachment>::create(b, buf1);
		a->isCompatible(*image2);
		a->clear();
		a->getName();
		return a;
	});

	auto a3 = builder.addAttachemnt("Attachment3", [&] (AttachmentBuilder &b) -> Rc<Attachment> {
		b.defineAsOutput();
		b.defineAsInput();
		auto a = Rc<ImageAttachment>::create(b, image3, ImageAttachment::AttachmentInfo());
		a->isCompatible(*image2);
		a->clear();
		a->getName();
		return a;
	});

	Rc<QueuePass> pass1;
	Rc<QueuePass> pass2;
	Rc<QueuePass> pass3;

	builder.addPass("Pass1", PassType::Graphics, RenderOrdering(0), [&] (QueuePassBuilder &b) {
		b.addAttachment(a1);
		b.addAttachment(a2);
		b.addAttachment(a3);

		auto p = Rc<QueuePass>::create(b);

		pass1 = p;

		return p;
	});

	builder.addPass("Pass2", PassType::Graphics, RenderOrdering(1), [&] (QueuePassBuilder &b) {
		b.addAttachment(a1);
		b.addAttachment(a2);
		b.addAttachment(a3);
		b.addSubmittedCallback([] (const QueuePassData &, FrameQueue &, bool success) {

		});

		auto p = Rc<QueuePass>::create(b);

		pass2 = p;

		return p;
	});

	builder.addPass("Pass3", PassType::Graphics, RenderOrdering(2), [&] (QueuePassBuilder &b) {
		auto pa1 = b.addAttachment(a1, [] (AttachmentPassBuilder &ab) {
			ab.setAttachmentOps(AttachmentOps::WritesColor);
			ab.setInitialLayout(AttachmentLayout::ShaderReadOnlyOptimal);
			ab.setFinalLayout(AttachmentLayout::ShaderReadOnlyOptimal);
			ab.setLoadOp(AttachmentLoadOp::Load);
			ab.setStoreOp(AttachmentStoreOp::Store);
			ab.setStencilLoadOp(AttachmentLoadOp::Load);
			ab.setStencilStoreOp(AttachmentStoreOp::Store);
			ab.setColorMode(ColorMode::SolidColor);
		});
		b.addAttachment(a2);
		auto pa3 = b.addAttachment(a3);

		auto p = Rc<QueuePass>::create(b);

		auto l = b.addDescriptorLayout([] (PipelineLayoutBuilder &lb) {
			lb.setUsesTextureSet(true);
		});

		b.addSubpass([&] (SubpassBuilder &sb) {
			sb.addColor(pa1, AttachmentDependencyInfo());
			sb.addResolve(pa1, pa3, AttachmentDependencyInfo(), AttachmentDependencyInfo());

			ProgramData pdata;
			pdata.key = StringView("program10");

			auto shaderSpecInfo = Vector<SpecializationInfo>({
				// no specialization required for vertex shader
				SpecializationInfo(fragProgram, Vector<SpecializationConstant>{
					SpecializationConstant(PredefinedConstant::BuffersArraySize)
				}),
				// specialization for fragment shader - use platform-dependent array sizes
				SpecializationInfo(vertProgram, Vector<SpecializationConstant>{
					SpecializationConstant(PredefinedConstant::SamplersArraySize),
					SpecializationConstant(PredefinedConstant::TexturesArraySize)
				})
			});

			sb.addGraphicPipeline("Pipeline1", l, shaderSpecInfo, PipelineMaterialInfo({
				BlendInfo(),
				DepthInfo(true, true, CompareOp::Less)
			}), DynamicState::Viewport);

			auto shaderSpecInfo2 = Vector<SpecializationInfo>({
				// no specialization required for vertex shader
				SpecializationInfo(&pdata, Vector<SpecializationConstant>{
					SpecializationConstant(PredefinedConstant::BuffersArraySize)
				}),
				// specialization for fragment shader - use platform-dependent array sizes
				SpecializationInfo(vertProgram, Vector<SpecializationConstant>{
					SpecializationConstant(PredefinedConstant::SamplersArraySize),
					SpecializationConstant(PredefinedConstant::TexturesArraySize)
				})
			});

			sb.addGraphicPipeline("Pipeline2", l, shaderSpecInfo2, PipelineMaterialInfo({
				BlendInfo(),
				DepthInfo(true, true, CompareOp::Less)
			}), DynamicState::Viewport);
		});

		pass3 = p;

		return p;
	});

	auto &attachments = pass2->getData()->attachments;
	for (auto &it : attachments) {
		it->attachment->attachment->getNextRenderPass(it->pass);
		it->attachment->attachment->getPrevRenderPass(it->pass);
	}

	auto queue = Rc<Queue>::create(move(builder));

	queue->getOutput();
	queue->getOutput(AttachmentType::Image);
	queue->getTransferImageOutput();
	queue->getPass("Pass1");
	queue->getProgram("program1");
	queue->getOutputAttachments();
	queue->getOutputAttachment<ImageAttachment>();
	queue->getInputAttachment<ImageAttachment>();
	queue->getLinkedResources();
	queue->getGraphicPipelines();
	queue->getGraphicPipeline("Pipeline1");
	queue->getComputePipelines();
	queue->getComputePipeline("Pipeline1");
	queue->getDefaultSyncPassState();
	queue->getName();

	return true;
}

}

namespace STAPPLER_VERSIONIZED stappler::xenolith::test {

static bool XenolithCoreTest_locale() {
	using namespace locale;

	define("ru-ru", {
		pair("RichTextCopy", "Копировать"),
		pair("RichTextMakeBookmark", "В закладки"),
		pair("RichTextSendEmail", "Отправить письмом"),
		pair("RichTextShare", "Поделиться"),
		pair("RichTextReportMisprint", "Сообщить об ошибке"),

		pair("RichTextMisprintReported", "Ваше сообщение об опечатке отправлено. Опечатка будет исправлена в ближайшее время."),
		pair("RichTextBookmarkCreated", "Закладка создана."),
		pair("RichTextNoNetworkConnection", "Нет соединения с интернетом, попробуйте позже."),

		pair("RichTextBookmarks", "Закладки"),
		pair("RichTextBookmarkFiller", "Избранные статьи и закладки появятся в этом разделе"),
		pair("RichTextHistoryFiller", "Здесь будет показана история просмотра лекций"),
		pair("NumTest", "Первый:Второй:Третий"),
		pair("Num:NumTest", "First:Second:Third"),
	});

	define("ru-ru", {
		pair(uint32_t(0), "Копировать"),
		pair(uint32_t(1), "В закладки"),
		pair(uint32_t(2), "Отправить письмом"),
		pair(uint32_t(3), "Поделиться"),
		pair(uint32_t(4), "Сообщить об ошибке"),

		pair(uint32_t(5), "Ваше сообщение об опечатке отправлено. Опечатка будет исправлена в ближайшее время."),
		pair(uint32_t(6), "Закладка создана."),
		pair(uint32_t(7), "Нет соединения с интернетом, попробуйте позже."),

		pair(uint32_t(8), "Закладки"),
		pair(uint32_t(9), "Избранные статьи и закладки появятся в этом разделе"),
		pair(uint32_t(10), "Здесь будет показана история просмотра лекций"),
	});

	std::array<StringView, toInt(TimeTokens::Max)> ruRuTimeToken{
		"сегодня", "вчера",
		"января", "февраля", "марта", "апреля", "мая", "июня",
		"июля", "августа", "сентября", "октября", "ноября", "декабря"
	};

	define("ru-ru", ruRuTimeToken);

	define("en-us", {
		pair("RichTextCopy", "Copy"),
		pair("RichTextMakeBookmark", "Make bookmark"),
		pair("RichTextSendEmail", "Send as email"),
		pair("RichTextShare", "Share"),
		pair("RichTextReportMisprint", "Report misprint"),

		pair("RichTextMisprintReported", "Your misprint report has been sent. Misprint will be corrected soon."),
		pair("RichTextBookmarkCreated", "Bookmark created"),
		pair("RichTextNoNetworkConnection", "No connection with internet, try again later."),

		pair("RichTextBookmarks", "Bookmarks"),
		pair("RichTextBookmarkFiller", "Your favorites and bookmarks are going to be here"),
		pair("RichTextHistoryFiller", "Reading history are going to be here"),
		pair("NumTest", "First:Second:Third"),
		pair("Num:NumTest", "First:Second:Third"),
	});

	define("en-us", {
		pair(uint32_t(0), "Copy"),
		pair(uint32_t(1), "Make bookmark"),
		pair(uint32_t(2), "Send as email"),
		pair(uint32_t(3), "Share"),
		pair(uint32_t(4), "Report misprint"),

		pair(uint32_t(5), "Your misprint report has been sent. Misprint will be corrected soon."),
		pair(uint32_t(6), "Bookmark created"),
		pair(uint32_t(7), "No connection with internet, try again later."),

		pair(uint32_t(8), "Bookmarks"),
		pair(uint32_t(9), "Your favorites and bookmarks are going to be here"),
		pair(uint32_t(10), "Reading history are going to be here"),
	});

	std::array<StringView, toInt(TimeTokens::Max)> enUsTimeToken{
		"today", "yesterday",
		"jan", "feb", "mar", "apr", "may", "jun",
		"jul", "aug", "sep", "oct", "nov", "dev"
	};

	define("en-us", enUsTimeToken);

	timeToken(TimeTokens::Today);
	timeToken(TimeTokens::Yesterday);
	timeTokenTable();

	string(WideStringView());
	string(123);

	setDefault("en-us");
	getDefault();
	setLocale("ru-ru");
	getLocale();

	auto loc = "RichTextCopy"_meta;

	string(loc);
	string(0);
	timeTokenTable();

	string("NumTest"_meta);
	numeric("NumTest"_meta, 1);
	numeric("NumTest"_meta, 1);
	numeric("NumTest"_meta, 12);
	numeric("NumTest"_meta, 22);

	setNumRule("ru-ru", [] (uint32_t idx) -> uint8_t {
		if (idx < 10) {
			return 0;
		} else if (idx < 20) {
			return 1;
		} else if (idx < 100) {
			return 2;
		} else {
			return 3;
		}
	});

	setNumRule("en-us", [] (uint32_t idx) -> uint8_t {
		if (idx < 10) {
			return 2;
		} else if (idx < 20) {
			return 1;
		} else {
			return 0;
		}
	});

	string("NumTest"_meta);
	numeric("NumTest"_meta, 1);
	numeric("NumTest"_meta, 1);
	numeric("NumTest"_meta, 12);
	numeric("NumTest"_meta, 22);
	numeric("NumTest"_meta, 122);

	WideString wstr = "RichTextCopy"_locale.to_std_ustring();

	hasLocaleTagsFast(WideStringView());
	hasLocaleTagsFast(wstr);
	hasLocaleTagsFast(string::toUtf16<Interface>(localeIndex(2)));
	hasLocaleTagsFast(u"str%=1%test");
	hasLocaleTagsFast(u"str%RichTextCopy%test");
	hasLocaleTagsFast(u"str%=11234");
	hasLocaleTagsFast(u"str%RichTextCopytest");

	hasLocaleTags(WideStringView());
	hasLocaleTags(wstr);
	hasLocaleTags(string::toUtf16<Interface>(localeIndex(2)));
	hasLocaleTags(u"str%=1%test");
	hasLocaleTags(u"str%RichTextCopy%test");
	hasLocaleTags(u"str%=11234");
	hasLocaleTags(u"str%RichTextCopytest");

	resolveLocaleTags(wstr);
	resolveLocaleTags(string::toUtf16<Interface>(localeIndex(2)));
	resolveLocaleTags(u"str%=1%test");
	resolveLocaleTags(u"str%RichTextCopy%test");
	resolveLocaleTags(u"str%=11234");
	resolveLocaleTags(u"str%RichTextCopytest");
	resolveLocaleTags(u"str%Num:NumTest:1%test");
	resolveLocaleTags(u"str%Num:NumsTest:1%test");
	resolveLocaleTags(u"str%Num:NumsTest:-1%test");

	localDate(Time::now());
	localDate(Time::now() - TimeInterval::seconds(60 * 60));
	localDate(Time::now() - TimeInterval::seconds(24 * 60 * 60));
	localDate(Time::now() - TimeInterval::seconds(48 * 60 * 60));
	localDate(Time::now() - TimeInterval::seconds(72 * 60 * 60));
	localDate(Time::now() - TimeInterval::seconds(24 * 60 * 60 * 370));

	localDate(ruRuTimeToken, Time::now());
	localDate(ruRuTimeToken, Time::now() - TimeInterval::seconds(60 * 60));
	localDate(ruRuTimeToken, Time::now() - TimeInterval::seconds(24 * 60 * 60));
	localDate(ruRuTimeToken, Time::now() - TimeInterval::seconds(48 * 60 * 60));
	localDate(ruRuTimeToken, Time::now() - TimeInterval::seconds(72 * 60 * 60));
	localDate(ruRuTimeToken, Time::now() - TimeInterval::seconds(24 * 60 * 60 * 370));

	timeToken(TimeTokens::Today);
	timeToken(TimeTokens::Yesterday);

	language("ru-ru");
	language("en-us");
	language(StringView());
	common("ru");
	common("en");
	common(StringView());

	return true;
}

}

namespace STAPPLER_VERSIONIZED stappler::app::test {

struct XenolithCoreTest : Test {
	XenolithCoreTest() : Test("XenolithCoreTest") { }

	virtual bool run() override {
		auto mempool = memory::pool::create();
		memory::pool::push(mempool);

		testImageLoader();

		XenolithCoreTest_core();
		XenolithCoreTest_input();
		XenolithCoreTest_action();
		XenolithCoreTest_queue();

		xenolith::test::XenolithCoreTest_locale();

		memory::pool::pop();

		return true;
	}
} _XenolithCoreTest;

}

#endif
