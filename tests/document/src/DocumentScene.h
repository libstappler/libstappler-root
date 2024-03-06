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

#ifndef TESTS_DOCUMENT_SRC_DOCUMENTSCENE_H_
#define TESTS_DOCUMENT_SRC_DOCUMENTSCENE_H_

#include "XLVkGuiApplication.h"
#include "MaterialScene.h"

namespace STAPPLER_VERSIONIZED stappler::xenolith::richtext {

class DocumentApplication : public vk::BootstrapApplication {
public:
	virtual ~DocumentApplication();

protected:
	virtual Rc<Scene> createSceneForView(vk::View &view, const core::FrameContraints &constraints) override;
};

class DocumentScene : public material2d::Scene {
public:
	virtual ~DocumentScene() = default;

	virtual bool init(Application *, const core::FrameContraints &) override;

protected:
};

}

#endif /* TESTS_DOCUMENT_SRC_DOCUMENTSCENE_H_ */
