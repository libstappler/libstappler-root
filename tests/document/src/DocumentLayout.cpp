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

#include "DocumentLayout.h"
#include "XLDirector.h"
#include "XLRTCommonSource.h"
#include "XLRTSourceAsset.h"
#include "XLScene.h"
#include "XLDirector.h"
#include "XLApplication.h"
#include "XLAssetLibrary.h"

namespace STAPPLER_VERSIONIZED stappler::xenolith::richtext {

bool DocumentLayout::init() {
	if (!FlexibleLayout::init()) {
		return false;
	}

	_appBar = setFlexibleNode(Rc<material2d::AppBar>::create(material2d::AppBarLayout::Small, material2d::SurfaceStyle(
			material2d::NodeStyle::Filled, material2d::ColorRole::PrimaryContainer)));
	_appBar->setTitle("Test App Bar");
	_appBar->setNavButtonIcon(IconName::Navigation_close_solid);

	auto actionMenu = Rc<material2d::MenuSource>::create();
	actionMenu->addButton("", IconName::Editor_format_align_center_solid, [this] (material2d::Button *, material2d::MenuSourceButton *) {
		if (_appBar->getLayout() == material2d::AppBarLayout::CenterAligned) {
			_appBar->setLayout(material2d::AppBarLayout::Small);
		} else {
			_appBar->setLayout(material2d::AppBarLayout::CenterAligned);
		}
	});
	actionMenu->addButton("", IconName::Editor_vertical_align_top_solid, [this] (material2d::Button *, material2d::MenuSourceButton *) {
		_director->getView()->setDecorationVisible(!_decorationVisible);
		_decorationVisible = !_decorationVisible;
	});
	_appBar->setActionMenuSource(move(actionMenu));

	_view = setBaseNode(Rc<View>::create());
	_view->setUseSelection(true);

	setFlexibleMinHeight(0.0f);
	setFlexibleMaxHeight(56.0f);

	return true;
}

void DocumentLayout::onEnter(Scene *scene) {
	FlexibleLayout::onEnter(scene);

	auto lib = scene->getDirector()->getApplication()->getExtension<AssetLibrary>();

	auto docAsset = Rc<SourceFileAsset>::create(filesystem::currentDir<Interface>("mmd/Test.text"), "application/markdown");

	_source = Rc<CommonSource>::create(lib, docAsset);

	_view->setSource(_source);
}

}

