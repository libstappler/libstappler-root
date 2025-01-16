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

#ifndef TESTS_COMMON_XENOLITH_TESTAPPDELEGATE_H_
#define TESTS_COMMON_XENOLITH_TESTAPPDELEGATE_H_

#include "XLVkGuiApplication.h"
#include "XLStorageServer.h"
#include "XLNetworkController.h"
#include "XLAssetLibrary.h"

namespace STAPPLER_VERSIONIZED stappler::xenolith::app {

class TestAppDelegate : private vk::GuiApplication {
public:
	static EventHeader onSwapchainConfig;

	virtual ~TestAppDelegate();

	virtual bool init(ApplicationInfo &&);

	virtual void run();

	using mem_std::AllocBase::operator new;
	using mem_std::AllocBase::operator delete;

	using Ref::release;
	using Ref::retain;

	const core::SurfaceInfo &getSurfaceInfo() const { return _surfaceInfo; }
	const core::SwapchainConfig &getSwapchainConfig() const { return _swapchainConfig; }

	void setPreferredPresentMode(core::PresentMode);
	void setTripleBuffering(bool);

protected:
	core::SwapchainConfig selectConfig(const core::SurfaceInfo &info);

	virtual void loadExtensions() override;
	virtual void finalizeExtensions() override;

	Value _storageParams;
	Rc<xenolith::storage::Server> _storageServer;
	Rc<xenolith::network::Controller> _networkController;
	Rc<xenolith::storage::AssetLibrary> _assetLibrary;

	Mutex _configMutex;
	bool _tripleBuffering = false;
	core::PresentMode _preferredPresentMode = core::PresentMode::Unsupported;

	core::SurfaceInfo _surfaceInfo;
	core::SwapchainConfig _swapchainConfig;
	View *_rootView = nullptr;
};

}

#endif /* TESTS_COMMON_XENOLITH_TESTAPPDELEGATE_H_ */
