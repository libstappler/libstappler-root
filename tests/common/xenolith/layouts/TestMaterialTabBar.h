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

#ifndef TESTS_COMMON_XENOLITH_LAYOUTS_TESTMATERIALTABBAR_H_
#define TESTS_COMMON_XENOLITH_LAYOUTS_TESTMATERIALTABBAR_H_

#include "MaterialSurface.h"
#include "MaterialColorScheme.h"
#include "MaterialTabBar.h"
#include "TestCheckbox.h"
#include "TestMaterial.h"
#include "TestMaterialColorPicker.h"
#include "XL2dLayerRounded.h"

namespace STAPPLER_VERSIONIZED stappler::xenolith::app {

class TestMaterialTabBar : public TestMaterial {
public:
	virtual ~TestMaterialTabBar() { }

	virtual bool init() override;

	virtual void onContentSizeDirty() override;
	virtual void onEnter(Scene *) override;

	virtual void update(const UpdateTime &) override;

protected:
	void sendEventForNode(Vec2 pos, uint32_t id);

	material2d::TabBar *_tabBar = nullptr;
};

}

#endif /* TESTS_COMMON_XENOLITH_LAYOUTS_TESTMATERIALTABBAR_H_ */
