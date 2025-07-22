/**
 Copyright (c) 2025 Stappler Team <admin@stappler.org>

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

#include "GeneralLayout.h"
#include "XLContextInfo.h"
#include "XLSimpleButton.h"
#include "XLDirector.h"
#include "XLAppWindow.h"
#include "XL2dSceneContent.h"
#include "MonitorModeSelectionLayout.h"

namespace STAPPLER_VERSIONIZED stappler::xenolith::app {

bool GeneralLayout::init() {
	using namespace simpleui;

	if (!SceneLayout2d::init()) {
		return false;
	}

	// Создаём меню с вертикальной прокруткой
	_menu = addChild(Rc<ScrollView>::create(ScrollView::Vertical));
	_menu->setAnchorPoint(Anchor::MiddleTop);
	_menu->setIndicatorColor(Color::Grey_500);

	// Для работы прокрутке необходим контроллер, который наполняется элементами вдоль основной оси
	_menu->setController(Rc<ScrollController>::create());

	auto inputListener = addComponent(Rc<InputListener>::create());
	inputListener->setCloseRequestCallback([](bool value) {
		if (value) {
			log::debug("ExampleScene", "close request received");
		}
		return true;
	});

	inputListener->setScreenUpdateCallback([this](bool value) {
		// monitor list or modes lists updated
		updateScreenInfo();
		return true;
	});

	inputListener->setFullscreenCallback([this](bool value) {
		// enter or exit fullscreen mode
		rebuildMenu();
		return true;
	});

	return true;
}

void GeneralLayout::handleEnter(Scene *scene) {
	SceneLayout2d::handleEnter(scene);
	updateScreenInfo();
	rebuildMenu();
}

void GeneralLayout::handleContentSizeDirty() {
	SceneLayout2d::handleContentSizeDirty();

	auto cs = getContentSize();

	// Размещаем меню
	// Якорь находится посередине сверху
	_menu->setPosition(Vec2(cs.width / 2.0f, cs.height));

	// Задаём размер меню
	// Разворачиваем на полную высоту, но ограничиваем максимальную ширину
	_menu->setContentSize(Size2(std::min(_contentSize.width, 480.0f), _contentSize.height));
}

void GeneralLayout::rebuildMenu() {
	using namespace simpleui;

	auto controller = _menu->getController();

	controller->clear();

	// Задаём начальный отступ вдоль основной оси
	controller->addPlaceholder(32.0f);

	// Добавляем кнопку с фиксированным размером вдоль основной оси (высотой)
	// Побочная ось (в этом случае - ширина) наследуется от базового элемента
	controller->addItem([](const ScrollController::Item &) -> Rc<Node> {
		// Cоздаём простую конпку
		// Её позиционирование и размер контроллер прокрутки настроит сам
		return Rc<ButtonWithLabel>::create("Hello world",
				[] { log::debug("ExampleScene", "Hello world"); });
	}, 32.0f);

	controller->addItem([this](const ScrollController::Item &) -> Rc<Node> {
		return Rc<ButtonWithLabel>::create("Read from clibboard", [this] {
			_director->getApplication()->readFromClipboard(
					[this](BytesView bytes, StringView type) {
				if (type == "image/png" && !bytes.empty()) {
					auto tex = _director->getResourceCache()->addExternalEncodedImage("Clipboard",
							core::ImageInfo(core::ImageFormat::R8G8B8A8_UNORM,
									core::ImageUsage::Sampled),
							bytes);

					_menu->getController()->addItem(
							[tex = move(tex)](const ScrollController::Item &) mutable -> Rc<Node> {
						auto sprite = Rc<Sprite>::create(move(tex));
						sprite->setTextureAutofit(Autofit::Contain);
						return sprite;
					}, 200.0f);
					_menu->getController()->onScrollPosition(true);
				}
			}, [](SpanView<StringView> typeList) -> StringView {
				StringView ret;
				for (auto &it : typeList) {
					if (it == "image/png") {
						ret = it;
					}
				}
				return ret;
			}, this);
		});
	}, 32.0f, ZOrder(0), "ClipboardButton");

	controller->addItem([this](const ScrollController::Item &) -> Rc<Node> {
		return Rc<ButtonWithLabel>::create("Write to clipboard", [this] {
			_director->getApplication()->writeToClipboard(BytesView("Xenolith clipboard text"));
		});
	}, 32.0f);

	controller->addItem([this](const ScrollController::Item &) -> Rc<Node> {
		return Rc<ButtonWithLabel>::create("Capture screenshot", [this] {
			_director->getWindow()->captureScreenshot(
					[this](const core::ImageInfoData &image, BytesView data) {
				auto bmp = core::getBitmap(image, data);
				if (bmp) {
					auto png = bmp.write();

					_director->getApplication()->writeToClipboard(png, "image/png");
					log::debug("ExampleScene", "Capture screenshot");
				}
			});
		});
	}, 32.0f);

	/*controller->addItem([this](const ScrollController::Item &) -> Rc<Node> {
		// Cоздаём простую конпку
		// Её позиционирование и размер контроллер прокрутки настроит сам
		return Rc<ButtonWithLabel>::create(_exitGuardRetained ? "Disable Exit Guard"
															  : "Enable Exit Guard",
				[this] { toggleExitGuard(); });
	}, 32.0f, ZOrder(0), "ExitGuardButton");*/

	if (_director) {
		auto w = _director->getWindow();
		if (hasFlag(w->getCapabilities(), WindowCapabilities::FullscreenSwitch)) {
			if (w->isFullscreen()) {
				_menu->getController()->addItem([this](const ScrollController::Item &) -> Rc<Node> {
					return Rc<ButtonWithLabel>::create("Exit fullscreen", [this] {
						_director->getWindow()->setFullscreen(MonitorId(MonitorId::None),
								ModeInfo(ModeInfo::Current), [](Status) { });
					});
				}, 64.0f, ZOrder(0));
			}

			if (_screenInfo) {
				uint32_t index = 0;
				for (auto &it : _screenInfo->monitors) {
					auto name = toString("Fullscreen to: ", it.name, " (", it.edid.vendor, " ",
							it.edid.model, " ", it.edid.serial, ")");
					_menu->getController()->addItem(
							[this, name, mon = it, index](
									const ScrollController::Item &) -> Rc<Node> {
						return Rc<ButtonWithLabel>::create(name, [this, mon, index] {
							auto l = Rc<MonitorModeSelectionLayout>::create(_screenInfo, index);
							getSceneContent()->pushLayout(l);
						});
					},
							64.0f, ZOrder(0));
					++index;
				}
			}
		}
	}

	controller->addItem([this](const ScrollController::Item &) -> Rc<Node> {
		// Cоздаём простую конпку
		// Её позиционирование и размер контроллер прокрутки настроит сам
		return Rc<ButtonWithLabel>::create("Close", [this] {
			auto w = _director->getWindow();
			w->close(true);
		});
	}, 32.0f);

	controller->addPlaceholder(32.0f);

	controller->commitChanges();
}

void GeneralLayout::toggleExitGuard() {
	auto w = _director->getWindow();

	if (!_exitGuardRetained) {
		w->retainExitGuard();
		_exitGuardRetained = true;

		auto item = _menu->getController()->getItem("ExitGuardButton");
		if (item && item->node) {
			static_cast<simpleui::ButtonWithLabel *>(item->node)->setString("Disable Exit guard");
		}
	} else {
		w->releaseExitGuard();
		_exitGuardRetained = false;

		auto item = _menu->getController()->getItem("ExitGuardButton");
		if (item && item->node) {
			static_cast<simpleui::ButtonWithLabel *>(item->node)->setString("Enable Exit guard");
		}
	}
}

void GeneralLayout::updateScreenInfo() {
	using namespace simpleui;
	if (_director) {
		_director->getWindow()->acquireScreenInfo([this](NotNull<ScreenInfo> screenInfo) {
			_screenInfo = screenInfo;
			if (_running) {
				rebuildMenu();
			}
		}, this);
	}
}

} // namespace stappler::xenolith::app
