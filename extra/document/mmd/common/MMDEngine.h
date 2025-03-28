/*

	Copyright © 2016 - 2017 Fletcher T. Penney.
	Copyright © 2017 Roman Katuntsev <sbkarr@stappler.org>


	The `MultiMarkdown 6` project is released under the MIT License..

	GLibFacade.c and GLibFacade.h are from the MultiMarkdown v4 project:

		https://github.com/fletcher/MultiMarkdown-4/

	MMD 4 is released under both the MIT License and GPL.


	CuTest is released under the zlib/libpng license. See CuTest.c for the text
	of the license.


	## The MIT License ##

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

*/

#ifndef MMD_COMMON_MMDENGINE_H_
#define MMD_COMMON_MMDENGINE_H_

#include "SPRef.h"
#include "MMDCommon.h"

namespace STAPPLER_VERSIONIZED stappler::mmd {

class SP_PUBLIC Engine : public Ref {
public:
	using ProcessCallback = Callback<void(const Content &, StringView, const Token &)>;

	virtual ~Engine();

	bool init(memory::pool_t *, StringView, const Extensions & = DefaultExtensions);
	bool init(StringView, const Extensions & = DefaultExtensions);

	void clear();

	void setQuotesLanguage(QuotesLanguage);
	QuotesLanguage getQuotesLanguage() const;

	void process(const ProcessCallback &);

protected:
	void prepare();

	struct Internal;

	Internal *_internal = nullptr;
};

}

#endif /* MMD_COMMON_MMDENGINE_H_ */
