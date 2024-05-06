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

#if MODULE_XENOLITH_SCENE

#include "TestAppScene.h"
#include "TestAppDelegate.h"
#include "TestGeneralUpdate.h"

#include "XLVkAttachment.h"
#include "XLDirector.h"
#include "XL2dSprite.h"
#include "XLApplication.h"

#include "XLVkAttachment.h"
#include "XL2dSceneContent.h"

#include "backend/vk/XL2dVkShadowPass.h"

namespace STAPPLER_VERSIONIZED stappler::xenolith::app {

static auto INIT_LAYOUT = LayoutName::MaterialMenuTest;

TestAppScene::~TestAppScene() { }

bool TestAppScene::init(Application *app, const core::FrameContraints &constraints) {
	// build presentation RenderQueue
	core::Queue::Builder builder("Loader");

	builder.addImage("xenolith-1-480.png",
		core::ImageInfo(core::ImageFormat::R8G8B8A8_UNORM, core::ImageUsage::Sampled, core::ImageHints::Opaque),
		FilePath("resources/xenolith-1-480.png"));
	builder.addImage("xenolith-2-480.png",
		core::ImageInfo(core::ImageFormat::R8G8B8A8_UNORM, core::ImageUsage::Sampled, core::ImageHints::Opaque),
		FilePath("resources/xenolith-2-480.png"));

	basic2d::vk::ShadowPass::RenderQueueInfo info{
		app, Extent2(constraints.extent.width, constraints.extent.height), basic2d::vk::ShadowPass::Flags::None,
	};

	basic2d::vk::ShadowPass::makeDefaultRenderQueue(builder, info);

	if (!material2d::Scene::init(move(builder), constraints)) {
		return false;
	}

	auto content = Rc<material2d::SceneContent>::create();

	setContent(content);

	filesystem::mkdir(filesystem::cachesPath<Interface>());

	scheduleUpdate();

	content->pushLayout(makeLayoutNode(INIT_LAYOUT));

	return true;
}

void TestAppScene::onPresented(Director *dir) {
	auto c = dir->getFrameConstraints();
	auto tmp = c;

	tmp.transform = core::SurfaceTransformFlags::Rotate90 | core::SurfaceTransformFlags::PreRotated;
	dir->setFrameConstraints(tmp);

	tmp.transform = core::SurfaceTransformFlags::Rotate180 | core::SurfaceTransformFlags::PreRotated;
	dir->setFrameConstraints(tmp);

	tmp.transform = core::SurfaceTransformFlags::Rotate270 | core::SurfaceTransformFlags::PreRotated;
	dir->setFrameConstraints(tmp);

	tmp.transform = core::SurfaceTransformFlags::Mirror | core::SurfaceTransformFlags::PreRotated;
	dir->setFrameConstraints(tmp);

	tmp.transform = core::SurfaceTransformFlags::MirrorRotate90 | core::SurfaceTransformFlags::PreRotated;
	dir->setFrameConstraints(tmp);

	tmp.transform = core::SurfaceTransformFlags::MirrorRotate180 | core::SurfaceTransformFlags::PreRotated;
	dir->setFrameConstraints(tmp);

	tmp.transform = core::SurfaceTransformFlags::MirrorRotate270 | core::SurfaceTransformFlags::PreRotated;
	dir->setFrameConstraints(tmp);

	dir->setFrameConstraints(c);
	dir->getFps();
	dir->getView()->getAvgFrameInterval();
	dir->getView()->getAvgFrameTime();
	dir->getView()->getFrameInterval();
	dir->getView()->getBackButtonCounter();
	dir->getView()->retainBackButton();
	dir->getView()->releaseBackButton();
	dir->getView()->setDecorationVisible(true);

	auto image = dir->getView()->getSwapchainImageInfo();

	image.imageType = core::ImageType::Image1D;
	dir->getView()->getSwapchainImageViewInfo(image);

	image.imageType = core::ImageType::Image2D;
	dir->getView()->getSwapchainImageViewInfo(image);

	image.imageType = core::ImageType::Image3D;
	dir->getView()->getSwapchainImageViewInfo(image);

	Scene2d::onPresented(dir);
}

void TestAppScene::onFinished(Director *dir) {
	Scene2d::onFinished(dir);
}

void TestAppScene::update(const UpdateTime &time) {
	Scene2d::update(time);
}

void TestAppScene::onEnter(xenolith::Scene *scene) {
	Scene2d::onEnter(scene);

	runAction(Rc<Sequence>::create(0.6f, [this] {
		runNext(INIT_LAYOUT);
	}));
}

void TestAppScene::onExit() {
	std::cout << "AppScene::onExit\n";
	Scene2d::onExit();
}

void TestAppScene::render(FrameInfo &info) {
	Scene2d::render(info);
}

void TestAppScene::runLayout(LayoutName l, Rc<basic2d::SceneLayout2d> &&node) {
	static_cast<basic2d::SceneContent2d *>(_content)->replaceLayout(node);
	_contentSizeDirty = true;
}

void TestAppScene::runNext(LayoutName name) {
	if (toInt(name) == 0) {
		if (auto v = _director->getView()) {
			v->close();
		}
	} else {
		auto next = LayoutName(toInt(name) - 1);
		if (auto l = makeLayoutNode(next)) {
			static_cast<basic2d::SceneContent2d *>(_content)->pushLayout(move(l));
			runAction(Rc<Sequence>::create(0.5f, [this, next] {
				runNext(next);
			}));
		}
	}
}

}

#endif
