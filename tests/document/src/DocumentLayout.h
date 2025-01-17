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

#ifndef TESTS_DOCUMENT_SRC_DOCUMENTLAYOUT_H_
#define TESTS_DOCUMENT_SRC_DOCUMENTLAYOUT_H_

#include "MaterialFlexibleLayout.h"
#include "MaterialAppBar.h"
#include "XL2dScrollView.h"
#include "XL2dScrollController.h"
#include "XLRTView.h"

namespace STAPPLER_VERSIONIZED stappler::xenolith::richtext {

class DocumentLayout : public material2d::FlexibleLayout {
public:
	virtual ~DocumentLayout() = default;

	virtual bool init();

	virtual void handleEnter(Scene *);

protected:
	material2d::AppBar *_appBar = nullptr;
	View *_view = nullptr;
	bool _decorationVisible = true;

	Rc<CommonSource> _source;
};

}

#endif /* TESTS_DOCUMENT_SRC_DOCUMENTLAYOUT_H_ */
