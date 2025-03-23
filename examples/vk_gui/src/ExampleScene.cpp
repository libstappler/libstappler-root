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

#include "XLCommon.h"
#include "XL2dSceneContent.h"
#include "ExampleScene.h"

namespace stappler::xenolith::app {

bool ExampleScene::init(Application *app, const core::FrameConstraints &constraints) {
	// Инициализируем суперкласс
	if (!Scene2d::init(app, constraints)) {
		return false;
	}

	// Создаём объект, хранящий содержимое сцены
	// Содержимое сцены связывается с конкретным проходом очереди рендеринга
	// Для нескольких проходов в рамках одной сцены потребуется несколько таких объектов
	auto content = Rc<basic2d::SceneContent2d>::create();

	// Создаём в содержимом сцены текстовое поле
	_helloWorldLabel = content->addChild(Rc<basic2d::Label>::create());
	_helloWorldLabel->setString("Hello World");
	_helloWorldLabel->setAnchorPoint(Anchor::Middle);

	// Применяем содержимое сцены
	setContent(content);

	// Настраиваем систему 2D-освещения (не обязательно, пример не использует тени)

	auto color = Color4F::WHITE;
	color.a = 0.5f;

	// Свет сверху под углом, дающий удлиннение теней снизу
	auto light = Rc<basic2d::SceneLight>::create(basic2d::SceneLightType::Ambient, Vec2(0.0f, 0.3f), 1.5f, color);

	// Свет строго сверху, дающий базовые тени
	auto ambient = Rc<basic2d::SceneLight>::create(basic2d::SceneLightType::Ambient, Vec2(0.0f, 0.0f), 1.5f, color);

	content->removeAllLights();

	// Базовый белый заполняющий свет
	content->setGlobalLight(Color4F::WHITE);

	content->addLight(move(light));
	content->addLight(move(ambient));

	// создаём директорию для хранения кешей
	filesystem::mkdir(filesystem::cachesPath<Interface>());

	return true;
}

void ExampleScene::handleContentSizeDirty() {
	Scene2d::handleContentSizeDirty();

	// Размещаем тексовое поле в нужном положении, в центре сцены
	_helloWorldLabel->setPosition(_content->getContentSize() / 2.0f);
}

}
