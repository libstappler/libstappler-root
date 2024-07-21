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

#include "XLCommon.h"
#include "XLPlatform.h"
#include "GuiApplication.h"
#include "GuiScene.h"

namespace stappler::xenolith::test {

static constexpr uint32_t SwapchainImageCount = 3;

static core::SwapchainConfig selectConfig(const core::SurfaceInfo &info) {
	core::SwapchainConfig ret;
	ret.extent = info.currentExtent;
	ret.imageCount = std::max(uint32_t(2), info.minImageCount);

	ret.presentMode = info.presentModes.front();

	if (std::find(info.presentModes.begin(), info.presentModes.end(), core::PresentMode::Immediate) != info.presentModes.end()) {
		ret.presentModeFast = core::PresentMode::Immediate;
	}

	auto it = info.formats.begin();
	while (it != info.formats.end()) {
		if (it->first != platform::getCommonFormat()) {
			++ it;
		} else {
			break;
		}
	}

	if (it == info.formats.end()) {
		ret.imageFormat = info.formats.front().first;
		ret.colorSpace = info.formats.front().second;
	} else {
		ret.imageFormat = it->first;
		ret.colorSpace = it->second;
	}

	if ((info.supportedCompositeAlpha & core::CompositeAlphaFlags::Opaque) != core::CompositeAlphaFlags::None) {
		ret.alpha = core::CompositeAlphaFlags::Opaque;
	} else if ((info.supportedCompositeAlpha & core::CompositeAlphaFlags::Inherit) != core::CompositeAlphaFlags::None) {
		ret.alpha = core::CompositeAlphaFlags::Inherit;
	}

	ret.transfer = (info.supportedUsageFlags & core::ImageUsage::TransferDst) != core::ImageUsage::None;

	if (ret.presentMode == core::PresentMode::Mailbox) {
		ret.imageCount = std::max(SwapchainImageCount, ret.imageCount);
	}

	ret.transform = info.currentTransform;
	return ret;
}

void runApplication(Application::CommonInfo &&info, float density, Function<void()> &&initCb) {
	auto name = info.applicationName;
	auto bundle = info.bundleName;

	auto mainLoop = Rc<vk::GuiApplication>::create(move(info));

	Application::CallbackInfo callbacks({
		.initCallback = [&] (const Application &) {
			mainLoop->addView(ViewInfo{
				.title = name,
				.bundleId = bundle,
				.density = density,
				.selectConfig = [] (View &, const core::SurfaceInfo &info) -> core::SwapchainConfig {
					return selectConfig(info);
				},
				.onCreated = [mainLoop = mainLoop.get()] (View &view, const core::FrameContraints &constraints) {
					auto scene = Rc<GuiScene>::create(mainLoop, constraints);
					view.getDirector()->runScene(move(scene));
				},
				.onClosed = [mainLoop = mainLoop.get()] (View &view) {
					mainLoop->end();
				}
			});
			if (initCb) {
				initCb();
			}
		},
		.updateCallback = [&] (const Application &, const UpdateTime &time) {

		}
	});

	mainLoop->run(callbacks);
}

}
