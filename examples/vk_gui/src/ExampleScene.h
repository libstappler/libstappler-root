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

#ifndef EXAMPLES_VK_GUI_SRC_EXAMPLESCENE_H_
#define EXAMPLES_VK_GUI_SRC_EXAMPLESCENE_H_

#include "XL2dScene.h"
#include "XL2dLabel.h"

namespace stappler::xenolith::app {

// Используем базовую 2D-сцену в качестве основы
class ExampleScene : public basic2d::Scene2d {
public:
	virtual ~ExampleScene() = default;

	// переопределяем создание сцены
	virtual bool init(Application *, const core::FrameConstraints &constraints) override;

	// переопределяем размещение объектов на сцене при изменении размера
	virtual void handleContentSizeDirty() override;

protected:
	using Scene::init;

	// Текстовое поле Hello world
	basic2d::Label *_helloWorldLabel = nullptr;
};

}

#endif /* EXAMPLES_VK_GUI_SRC_EXAMPLESCENE_H_ */
