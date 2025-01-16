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

#include "GuiScene.h"

#include "GuiSceneContent.h"

#include "backend/vk/XL2dVkShadowPass.h"

namespace stappler::xenolith::test {

GuiScene::~GuiScene() { }

bool GuiScene::init(Application *loop, const core::FrameContraints &constraints) {
	core::Queue::Builder builder("Loader");

	builder.addImage("xenolith-1-480.png",
			core::ImageInfo(core::ImageFormat::R8G8B8A8_UNORM, core::ImageUsage::Sampled, core::ImageHints::Opaque),
			FilePath("resources/xenolith-1-480.png"));
	builder.addImage("xenolith-2-480.png",
			core::ImageInfo(core::ImageFormat::R8G8B8A8_UNORM, core::ImageUsage::Sampled, core::ImageHints::Opaque),
			FilePath("resources/xenolith-2-480.png"));

	basic2d::vk::ShadowPass::RenderQueueInfo info{
		loop, Extent2(constraints.extent.width, constraints.extent.height), basic2d::vk::ShadowPass::Flags::None
	};

	basic2d::vk::ShadowPass::makeRenderQueue(builder, info);

	if (!Scene2d::init(move(builder), constraints)) {
		return false;
	}

	auto content = Rc<GuiSceneContent>::create();

	setContent(content);

	return true;
}

void GuiScene::handleEnter(Scene *scene) {
	Scene2d::handleEnter(scene);

	std::thread thread([] {
		String testString("test String");

		log::debug("GuiScene", string::tolower<Interface>(testString));
		log::debug("GuiScene", string::toupper<Interface>(testString));
		log::debug("GuiScene", string::totitle<Interface>(testString));
	});
	thread.detach();
}

}
