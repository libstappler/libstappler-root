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

	if (!Scene2d::init(move(builder), constraints)) {
		return false;
	}

	auto content = Rc<basic2d::SceneContent2d>::create();

	setContent(content);

	filesystem::mkdir(filesystem::cachesPath<Interface>());

	scheduleUpdate();

	content->pushLayout(makeLayoutNode(LayoutName::GeneralUpdateTest));

	return true;
}

void TestAppScene::onPresented(Director *dir) {
	Scene2d::onPresented(dir);
}

void TestAppScene::onFinished(Director *dir) {
	Scene2d::onFinished(dir);
}

void TestAppScene::update(const UpdateTime &time) {
	Scene2d::update(time);
}

void TestAppScene::onEnter(Scene *scene) {
	Scene2d::onEnter(scene);

	runAction(Rc<Sequence>::create(1.0f, [this] {
		runNext(LayoutName::GeneralUpdateTest);
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
	switch (name) {
	case LayoutName::GeneralUpdateTest:
		if (auto l = makeLayoutNode(LayoutName::MaterialColorPickerTest)) {
			static_cast<basic2d::SceneContent2d *>(_content)->pushLayout(move(l));
			runAction(Rc<Sequence>::create(1.0f, [this] {
				runNext(LayoutName::MaterialColorPickerTest);
			}));
		}
		break;
	case LayoutName::MaterialColorPickerTest:
		if (auto v = _director->getView()) {
			v->close();
		}
		break;
	default:
		break;
	}
}

}

#endif
