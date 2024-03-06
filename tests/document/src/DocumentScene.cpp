/**
 Copyright (c) 2024 Stappler LLC <admin@stappler.dev>

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

#include "DocumentScene.h"
#include "DocumentLayout.h"
#include "XL2dSceneContent.h"

namespace STAPPLER_VERSIONIZED stappler::xenolith::richtext {

DocumentApplication::~DocumentApplication() { }

Rc<Scene> DocumentApplication::createSceneForView(vk::View &view, const core::FrameContraints &constraints) {
	return Rc<DocumentScene>::create(this, constraints);
}

bool DocumentScene::init(Application *app, const core::FrameContraints &constraints) {
	if (!Scene::init(app, [] (Queue::Builder &builder) {
		builder.addImage("icon.png",
				core::ImageInfo(core::ImageFormat::R8G8B8A8_UNORM, core::ImageUsage::Sampled, core::ImageHints::Opaque),
				FilePath(filesystem::currentDir<Interface>("images/icon.png")));
	}, constraints)) {
		return false;
	}

	auto content = Rc<basic2d::SceneContent2d>::create();

	setContent(content);

	auto l = Rc<DocumentLayout>::create();

	content->pushLayout(l);

	return true;
}

}
