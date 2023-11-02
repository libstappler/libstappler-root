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

#include "TestAppDelegate.h"
#include "TestAppScene.h"
#include "XLPlatform.h"

namespace stappler::xenolith::app {

XL_DECLARE_EVENT_CLASS(TestAppDelegate, onSwapchainConfig);

TestAppDelegate::~TestAppDelegate() { }

bool TestAppDelegate::init(ViewCommandLineData &&data, void *native) {
	_data = move(data);

	Application::CommonInfo info({
		.bundleName = _data.bundleName,
		.applicationName = _data.applicationName,
		.applicationVersion = _data.applicationVersion,
		.userAgent = _data.userAgent,
		.locale = _data.userLanguage,
		.nativeHandle = native
	});

	_storageParams = Value({
		pair("driver", Value("sqlite")),
		pair("dbname", Value(filesystem::cachesPath<Interface>("root.sqlite"))),
		pair("serverName", Value("RootStorage"))
	});

	return GuiApplication::init(move(info));
}

void TestAppDelegate::run(Function<void()> &&initCb) {
	db::setStorageRoot(&_storageRoot);

	if (_storageParams.getString("driver") == "sqlite") {
		auto path = _storageParams.getString("dbname");
		filesystem::mkdir(filepath::root(filepath::root(path)));
		filesystem::mkdir(filepath::root(path));
	}

	_storageServer = Rc<storage::Server>::create(static_cast<Application *>(this), _storageParams);

	if (!_storageServer) {
		log::error("Application", "Fail to launch application: onBuildStorage failed");
	}

	_networkController = Rc<network::Controller>::alloc(static_cast<Application *>(this), "Root");

	auto libpath = filesystem::writablePath<Interface>("library");
	filesystem::mkdir(libpath);

	_assetLibrary = Rc<storage::AssetLibrary>::create(static_cast<Application *>(this), _networkController, Value({
		pair("driver", Value("sqlite")),
		pair("dbname", Value(filesystem::cachesPath<Interface>("assets.sqlite"))),
		pair("serverName", Value("AssetStorage"))
	}));

	addExtension(Rc<storage::Server>(_storageServer));
	addExtension(Rc<network::Controller>(_networkController));
	addExtension(Rc<storage::AssetLibrary>(_assetLibrary));

	GuiApplication::CallbackInfo callbacks({
		.initCallback = [&] (const Application &) {
			GuiApplication::addView(ViewInfo{
				.name = _data.applicationName,
				.bundleId = _data.bundleName,
				.rect = URect(UVec2{0, 0}, _data.screenSize),
				.density = _data.density,
				.selectConfig = [this] (View &, const core::SurfaceInfo &info) -> core::SwapchainConfig {
					return selectConfig(info);
				},
				.onCreated = [this] (View &view, const core::FrameContraints &constraints) {
					auto scene = Rc<TestAppScene>::create(static_cast<Application *>(this), constraints);
					view.getDirector()->runScene(move(scene));
				},
				.onClosed = [this] (View &view) {
					end();
				}
			});
			if (initCb) {
				initCb();
			}
		},
		.updateCallback = [&] (const Application &, const UpdateTime &time) {

		}
	});

	GuiApplication::run(callbacks);

	_storageServer = nullptr;
	_networkController = nullptr;
}

void TestAppDelegate::setPreferredPresentMode(core::PresentMode mode) {
	std::unique_lock<Mutex> lock(_configMutex);
	_preferredPresentMode = mode;
}

core::SwapchainConfig TestAppDelegate::selectConfig(const core::SurfaceInfo &info) {
	std::unique_lock<Mutex> lock(_configMutex);
	core::SwapchainConfig ret;
	ret.extent = info.currentExtent;
	ret.imageCount = std::max(uint32_t(2), info.minImageCount);

	ret.presentMode = info.presentModes.front();
	if (_preferredPresentMode != core::PresentMode::Unsupported) {
		for (auto &it : info.presentModes) {
			if (it == _preferredPresentMode) {
				ret.presentMode = it;
				break;
			}
		}
	}

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
		ret.imageCount = std::max(uint32_t(3), ret.imageCount);
	}

	ret.transform = info.currentTransform;

	performOnMainThread([this, info, ret] {
		_surfaceInfo = info;
		_swapchainConfig = ret;

		onSwapchainConfig(this);
	});

	return ret;
}

}
