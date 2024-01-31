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

#include "XLCore.h"
#include "XLCoreInfo.h"
#include "XLCoreInput.h"
#include "XLVk.h"

namespace STAPPLER_VERSIONIZED stappler::app::test {

struct XenolithCoreTest : Test {
	XenolithCoreTest() : Test("XenolithCoreTest") { }

	virtual bool run() override {
		using namespace xenolith::core;

		auto mempool = memory::pool::create();
		memory::pool::push(mempool);

		StringStream out;

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

		for (size_t i = 0; i < toInt(VkFormat::VK_FORMAT_ASTC_12x12_SRGB_BLOCK) + 1; ++ i) {
			out << xenolith::vk::getVkFormatName(VkFormat(i)) << ":" << xenolith::vk::getFormatBlockSize(VkFormat(i));
		}
		for (size_t i = toInt(VkFormat::VK_FORMAT_G8B8G8R8_422_UNORM); i < toInt(VkFormat::VK_FORMAT_A8_UNORM_KHR) + 1; ++ i) {
			out << xenolith::vk::getVkFormatName(VkFormat(i)) << ":" << xenolith::vk::getFormatBlockSize(VkFormat(i));;
		}

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

		memory::pool::pop();

		return true;
	}
} _XenolithCoreTest;

}

#endif
