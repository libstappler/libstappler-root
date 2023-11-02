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

#ifndef TESTS_COMMON_XENOLITH_WIDGETS_TESTMATERIALCOLORPICKER_H_
#define TESTS_COMMON_XENOLITH_WIDGETS_TESTMATERIALCOLORPICKER_H_

#include "TestLayout.h"
#include "MaterialColorHCT.h"
#include "XL2dSprite.h"

namespace stappler::xenolith::app {

class MaterialColorPicker : public Sprite {
public:
	enum Type {
		Hue,
		Chroma,
		Tone
	};
	static constexpr uint32_t QuadsCount = 60;

	virtual ~MaterialColorPicker() { }

	virtual bool init(Type, const material2d::ColorHCT &, Function<void(float)> &&cb);

	virtual void onContentSizeDirty() override;

	const material2d::ColorHCT &getTargetColor() const;
	void setTargetColor(const material2d::ColorHCT &color);

	void setValue(float value);
	float getValue() const;

	void setLabelColor(const Color4F &);

protected:
	virtual void updateVertexesColor() override;
	virtual void initVertexes() override;
	virtual void updateVertexes() override;

	String makeString();
	void updateValue();

	Type _type;
	float _value = 0.0f;
	material2d::ColorHCT _targetColor;
	Function<void(float)> _callback;
	Label *_label = nullptr;
	Layer *_indicator = nullptr;
	InputListener *_input = nullptr;
};

}

#endif /* TESTS_COMMON_XENOLITH_WIDGETS_TESTMATERIALCOLORPICKER_H_ */
