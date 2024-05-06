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

#ifndef TESTS_COMMON_XENOLITH_TESTAPPSCENE_H_
#define TESTS_COMMON_XENOLITH_TESTAPPSCENE_H_

#include "MaterialScene.h"
#include "MaterialSceneContent.h"
#include "XL2dSprite.h"
#include "XL2dLayer.h"

#include "TestLayout.h"

namespace STAPPLER_VERSIONIZED stappler::xenolith::app {

class TestAppScene : public material2d::Scene {
public:
	virtual ~TestAppScene();

	virtual bool init(Application *, const core::FrameContraints &constraints);

	virtual void onPresented(Director *) override;
	virtual void onFinished(Director *) override;

	virtual void update(const UpdateTime &) override;
	virtual void onEnter(xenolith::Scene *) override;
	virtual void onExit() override;

	virtual void render(FrameInfo &info) override;

	void runLayout(LayoutName l, Rc<basic2d::SceneLayout2d> &&);

protected:
	void runNext(LayoutName name);

	using Scene::init;
};

}

#endif /* TESTS_COMMON_XENOLITH_TESTAPPSCENE_H_ */
