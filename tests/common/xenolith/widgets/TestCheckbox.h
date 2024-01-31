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

#ifndef TESTS_COMMON_XENOLITH_WIDGETS_TESTCHECKBOX_H_
#define TESTS_COMMON_XENOLITH_WIDGETS_TESTCHECKBOX_H_

#include "TestLayout.h"
#include "XL2dLayer.h"

namespace STAPPLER_VERSIONIZED stappler::xenolith::app {

class TestCheckbox : public Layer {
public:
	virtual ~TestCheckbox() { }

	virtual bool init(bool, Function<void(bool)> &&);

	virtual void setValue(bool);
	virtual bool getValue() const;

	virtual void setForegroundColor(const Color4F &);
	virtual Color4F getForegroundColor() const;

	virtual void setBackgroundColor(const Color4F &);
	virtual Color4F getBackgroundColor() const;

protected:
	using Layer::init;

	virtual void updateValue();

	bool _value = false;
	Function<void(bool)> _callback;
	Color4F _backgroundColor = Color::Grey_200;
	Color4F _foregroundColor = Color::Grey_500;

	InputListener *_input = nullptr;
};

class TestCheckboxWithLabel : public TestCheckbox {
public:
	virtual ~TestCheckboxWithLabel() { }

	virtual bool init(StringView, bool, Function<void(bool)> &&);

	virtual void onContentSizeDirty() override;

	void setLabelColor(const Color4F &);

protected:
	using TestCheckbox::init;

	Label *_label = nullptr;
};

}

#endif /* TESTS_COMMON_XENOLITH_WIDGETS_TESTCHECKBOX_H_ */
