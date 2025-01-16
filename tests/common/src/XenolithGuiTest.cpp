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

#if MODULE_XENOLITH_RENDERER_MATERIAL2D && MODULE_XENOLITH_BACKEND_VKGUI && 0

#include "XLVkGuiApplication.h"
#include "XLCoreFrameRequest.h"
#include "XLCoreFrameQueue.h"
#include "XLView.h"
#include "XLInputListener.h"
#include "XL2dScene.h"
#include "XL2dSceneContent.h"
#include "XL2dLayer.h"
#include "backend/vk/XL2dVkShadowPass.h"
#include "XLPlatform.h"
#include "XLAction.h"

#include "TestAppDelegate.h"

namespace STAPPLER_VERSIONIZED stappler::app::test {

struct XenolithGuiTest : Test {
	XenolithGuiTest() : Test("XenolithGuiTest") { }

	virtual bool run() override {
		using namespace stappler::xenolith::app;

		auto mempool = memory::pool::create();
		memory::pool::push(mempool);

		auto caches = filesystem::cachesPath<Interface>();
		filesystem::remove(caches, true, true);
		filesystem::mkdir(caches);

		xenolith::ViewCommandLineData data;

		auto app = Rc<TestAppDelegate>::create(move(data));
		app->run();

		memory::pool::pop();

		return true;
	}
} _XenolithGuiTest;

}

#endif
