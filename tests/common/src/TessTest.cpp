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

#include "SPCommon.h"
#include "Test.h"

#if MODULE_XENOLITH_RENDERER_BASIC2D

#include "XL2dVectorCanvas.h"
#include "XLIcons.h"

namespace stappler::app::test {

struct TessTest : Test {
	struct PathData {
		String name;
		String str;
		Bytes data;
	};

	TessTest() : Test("TessTest") { }

	void drawIcon(StringStream &stream, xenolith::basic2d::VectorCanvas *canvas, size_t i, size_t &failed, vg::DrawStyle style, bool antialiased) {
		auto name = xenolith::getIconName(xenolith::IconName(i));
		vg::VectorImage image;
		image.init(xenolith::Size2(1024, 1024));

		xenolith::drawIcon(image, xenolith::IconName(i), 0.0f);

		Rc<vg::VectorPathRef> path = image.getPaths().begin()->second;
		path->setAntialiased(antialiased);
		path->setStyle(style);
		path->setStrokeWidth(0.5f);

		auto res = canvas->draw(image.popData(), xenolith::Size2(1024, 1024));
		if (res->data.size() == 0) {
			auto pIt = paths.find(i);

			stream << "\tFailed ("
					<< (style == vg::DrawStyle::Stroke ? "Stroke" : "Fill")
					<< ", " << (antialiased ? "aa" : "non-aa") << "): " << name;
			for (auto &iit : pIt->second) {
				stream << "\n\t\tPath: " << iit.str;
			}

			stream << "\n";

			++ failed;
		}
	}

	virtual bool run() override {
		StringStream stream; stream << "\n";
		size_t failed = 0;
		auto mempool = memory::pool::create();
		memory::pool::push(mempool);

		auto canvas = Rc<xenolith::basic2d::VectorCanvas>::create(false);

		size_t i = toInt(xenolith::IconName::Dynamic_Loader);
		size_t max = toInt(xenolith::IconName::Max);

		for (; i < max; ++ i) {
			auto name = xenolith::getIconName(xenolith::IconName(i));
			vg::VectorImage image;
			image.init(xenolith::Size2(1024, 1024));
			xenolith::drawIcon(image, xenolith::IconName(i), 0.0f);
			auto pIt = paths.emplace(i, Vector<PathData>()).first;

			auto &path = image.getPaths();
			for (auto &it : path) {
				vg::VectorPath *path = it.second->getPath();

				auto data = path->encode();
				auto str = path->toString(true);

				pIt->second.emplace_back(PathData{name.str<Interface>(), move(str), move(data)});
			}
		}

		i = toInt(xenolith::IconName::Dynamic_Loader);
		for (; i < max; ++ i) {
			drawIcon(stream, canvas, i, failed, vg::DrawStyle::Stroke, false);
		}

		for (size_t j = 0; j <= size_t(toInt(geom::Tesselator::RelocateRule::Monotonize)); ++ j) {
			i = toInt(xenolith::IconName::Dynamic_Loader);

			canvas->setRelocateRule(geom::Tesselator::RelocateRule(j));

			for (; i < max; ++ i) {
				drawIcon(stream, canvas, i, failed, vg::DrawStyle::Fill, true);
			}
		}

		i = toInt(xenolith::IconName::Dynamic_Loader);
		for (; i < max; ++ i) {
			drawIcon(stream, canvas, i, failed, vg::DrawStyle::Fill, false);
		}

		memory::pool::pop();

		_desc = stream.str();
		return failed == 0;
	}

	Map<size_t, Vector<PathData>> paths;
} _TessTest;

}

#endif
