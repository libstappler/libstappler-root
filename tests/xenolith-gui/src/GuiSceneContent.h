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

#ifndef TESTS_XENOLITH_GUI_SRC_GUISCENECONTENT_H_
#define TESTS_XENOLITH_GUI_SRC_GUISCENECONTENT_H_

#include "XL2dSceneContent.h"
#include "XL2dLayer.h"

namespace stappler::xenolith::test {

class GuiSceneContent : public basic2d::SceneContent2d {
public:
	virtual ~GuiSceneContent();

	virtual bool init() override;

	virtual void onContentSizeDirty() override;

protected:
	basic2d::Layer *_layer = nullptr;
	basic2d::Layer *_layer2 = nullptr;
	basic2d::Layer *_layer3 = nullptr;
	basic2d::Layer *_layer4 = nullptr;
};

}

#endif /* TESTS_XENOLITH_GUI_SRC_GUISCENECONTENT_H_ */