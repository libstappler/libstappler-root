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

#include "SPCommon.h"
#include "Test.h"
#include "SPSvgReader.h"
#include "SPVectorImage.h"

namespace STAPPLER_VERSIONIZED stappler::app::test {

constexpr auto s_svg1 = StringView(R"Svg(
<svg xmlns="http://www.w3.org/2000/svg" height="272" width="312" viewbox="0 0 220 100" version="1.1">
	<rect x="0" y="0" width="312" height="272" opacity="0.25" stroke-linecap="square" stroke-linejoin="bevel" />
	<g transform="translate(0 -780.4)">
		<path d="m104 948.4h104l-32-56-24 32-16-12zm-32-96v128h168v-128h-168zm16 16h136v96h-136v-96zm38 20a10 10 0 0 1 -10 10 10 10 0 0 1 -10 -10 10 10 0 0 1 10 -10 10 10 0 0 1 10 10z" fill-rule="evenodd" fill="#ccc"/>
	</g>
	<circle cx="64" cy="64" r="64" stroke-linecap="round" stroke-linejoin="round" stroke="none" />
	<path fill="#fff" d="m96.76 64-51.96 30v-60z" stroke-width=1px" stroke-linecap="butt" stroke-linejoin="miter" stroke-miterlimit="4.5" />
	<rect width="100" height="100" opacity="0.5" fill-opacity="0.5" stroke-opacity="0.5" />
	<rect x="120" width="100" height="100" rx="15%" ry="10%" opacity="1.0" fill-opacity="1.0" stroke-opacity="1.0" />
	<ellipse cx="100" cy="50" rx="100" ry="50" opacity="0.0" fill-opacity="0.0" stroke-opacity="0.0" />
	<line x1="0" y1="80" x2="100" y2="20" stroke="black" />
	<polyline points="0,0 50,150 100,75 150,50 200,140 250,140" stroke="black" />
	<polygon points="100,100 150,25 150,75 200,0" fill="none" stroke="black" />
	<path d="M 10 10 C 20 20, 40 20, 50 10" stroke="black" fill="transparent"/>
	<path d="M 70 10 C 70 20, 110 20, 110 10" stroke="black" fill="transparent"/>
	<path d="M 130 10 C 120 20, 180 20, 170 10" stroke="black" fill="transparent"/>
	<path d="M 10 60 C 20 80, 40 80, 50 60" stroke="black" fill="transparent"/>
	<path d="M 70 60 C 70 80, 110 80, 110 60" stroke="black" fill="transparent"/>
	<path d="M 130 60 C 120 80, 180 80, 170 60" stroke="black" fill="transparent"/>
	<path d="M 10 110 C 20 140, 40 140, 50 110" stroke="black" fill="transparent"/>
	<path d="M 70 110 C 70 140, 110 140, 110 110" stroke="black" fill="transparent"/>
	<path d="M 130 110 C 120 140, 180 140, 170 110" stroke="black" fill="transparent"/>
	<path d="M 10 80 C 40 10, 65 10, 95 80 S 150 150, 180 80" stroke="black" fill="transparent"/>
	<path d="M 10 80 Q 95 10 180 80" stroke="black" fill="transparent"/>
	<path d="M 10 80 Q 52.5 10, 95 80 T 180 80" stroke="black" fill="transparent"/>
	<path d="M 70 10 c 70 20, 110 20, 110 10" stroke="black" fill="transparent"/>
	<path d="M 10 80 c 40 10, 65 10, 95 80 s 150 150, 180 80" stroke="black" fill="transparent"/>
	<path d="M 10 80 q 95 10 180 80" stroke="black" fill="transparent"/>
	<path d="M 10 80 q 52.5 10, 95 80 t 180 80" stroke="black" fill="transparent"/>
	<path d="M 80 80 A 0 0, 0, 0, 0, 125 125 L 125 80 Z" fill="green"/>
	<defs>
		<circle id="myCircle" cx="5" cy="5" r="4" stroke="blue" style="fill: gold; stroke: maroon; stroke-width: 2px;" transform="rotate(-10)  skewY(40)" />
		<use id="myCircle2" href="#myCircle" transform="matrix(3 1 -1 3 30 40)" x="20" fill="white" stroke="red" fill-rule="nonzero" />
	</defs>
	<use href="#myCircle" id="use1" x="10" y="10" transform="rotate(-10 50 100) translate(-36 45.5) skewX(40) scale(1 0.5)" fill="blue" fill-rule="evenodd" />
	<use href="#myCircle" transform="matrix(3 1 -1 3 30 40)" x="20" y="20" fill="white" stroke="red" fill-rule="nonzero" />
	<use href="#myCircle2" fill="white" stroke="red" fill-rule="nonzero" />
</svg>
)Svg");

constexpr auto s_svg2 = StringView(R"Svg(
<svg xmlns="http://www.w3.org/2000/svg" height="272" width="312" viewbox="0 0 220 100" version="1.1">
	<path d="M 10 10 c 20 20, 40 20, 50 10" stroke="black" fill="transparent"/>
</svg>
)Svg");

struct VgTest : MemPoolTest {
	VgTest() : MemPoolTest("VgTest") { }

	void readPathData(vg::PathData<mem_pool::Interface> &&data) {
		vg::PathData<mem_pool::Interface> subdata = move(data);
		data.encode<mem_pool::Interface>();
		subdata.clear();
	}
	void readPathData(vg::PathData<Interface> data) {
		vg::PathData<mem_pool::Interface> subdata;
		subdata.getWriter().addPath(data.encode<Interface>());
		readPathData(move(subdata));
	}

	void readSvg(StringView str) {
		vg::SvgReader reader;
		html::parse<vg::SvgReader, StringView, vg::SvgTag>(reader, str);

		for (auto &it : reader._paths) {
			vg::PathData<Interface> data;
			data.getWriter().addPath(it.second.toString(false));
			readPathData(data);
		}
	}

	void testPathData() {
		vg::PathData<Interface> data;
		data.getWriter().readFromFileContent(s_svg2);

		auto filepath = filesystem::currentDir<Interface>("resources/24px.svg");
		data.getWriter().readFromFile(filepath);

		auto str1 = data.toString<mem_pool::Interface>(true);
		auto bytes1 = data.encode<mem_pool::Interface>();
		data.toString<mem_std::Interface>(true);
		data.encode<mem_std::Interface>();

		vg::PathData<mem_pool::Interface> pdata;
		pdata.getWriter().readFromFile(s_svg2);

		auto str2 = pdata.toString<mem_pool::Interface>(true);
		auto bytes2 = pdata.encode<mem_pool::Interface>();
		pdata.toString<mem_std::Interface>(true);
		pdata.encode<mem_std::Interface>();

		auto tmpData = data;

		data.getWriter().addPath(pdata);
		pdata.getWriter().addPath(tmpData);

		data.getWriter()
				.moveTo(geom::Vec2(0.0f, 0.0f))
				.lineTo(geom::Vec2(10.0f, 10.0f))
				.quadTo(geom::Vec2(0.0f, 10.0f), geom::Vec2(15.0f, 15.0f))
				.cubicTo(geom::Vec2(10.0f, 0.0f), geom::Vec2(-15.0f, -15.0f), geom::Vec2(15.0f, 15.0f))
				.arcTo(geom::Vec2(10.0f, 10.0f), 10.0f, true, false, geom::Vec2(0.0f, 0.0f));

		pdata.getWriter().reserve(100);
		pdata.getWriter().addOval(geom::Rect(0.0f, 0.0f, 10.0f, 10.0f));
		pdata.getWriter().addRect(0.0f, 0.0f, 10.0f, 10.0f, 0.0f, 2.0f);
		pdata.getWriter().addRect(0.0f, 0.0f, 10.0f, 10.0f, 2.0f, 0.0f);
		pdata.getWriter().addRect(0.0f, 0.0f, 10.0f, 10.0f, 2.0f, 3.0f);
		pdata.getWriter().addRect(geom::Rect(0.0f, 0.0f, 10.0f, 10.0f), 2.0f, 3.0f);

		tmpData = data;
		tmpData = move(data);

		data.toString<mem_pool::Interface>(true);
		data.encode<mem_pool::Interface>();
		data.toString<mem_std::Interface>(true);
		data.encode<mem_std::Interface>();

		pdata.toString<mem_pool::Interface>(true);
		pdata.encode<mem_pool::Interface>();
		pdata.toString<mem_std::Interface>(true);
		pdata.encode<mem_std::Interface>();
	}

	bool testImage() {
		auto filepath = filesystem::currentDir<Interface>("resources/24px.svg");

		vg::VectorImage::isSvg(FilePath(filepath));
		vg::VectorImage::isSvg(s_svg2);
		vg::VectorImage::isSvg(BytesView((const uint8_t *)s_svg2.data(), s_svg2.size()));

		vg::PathData<Interface> data;
		data.getWriter().readFromFileContent(s_svg2);
		auto pathStr = data.toString<Interface>(false);
		auto pathData = data.encode<Interface>();

		auto img1 = Rc<vg::VectorImage>::create(s_svg2);
		auto img2 = Rc<vg::VectorImage>::create(BytesView((const uint8_t *)s_svg2.data(), s_svg2.size()));
		auto img3 = Rc<vg::VectorImage>::create(geom::Size2(100.0f, 100.0f), pathStr);

		vg::VectorPath path;
		path.init(BytesView(pathData));

		auto img4 = Rc<vg::VectorImage>::create(geom::Size2(100.0f, 100.0f), vg::VectorPath(path));
		auto img5 = Rc<vg::VectorImage>::create(FilePath(filepath));

		auto data1 = img1->popData();
		img1->setImageSize(geom::Size2(100.0f, 100.0f));
		img1->getImageSize();
		img1->getViewBox();

		img1->addPath("tag", "tag.cache");
		auto data2 = img1->popData();
		img1->addPath(path, "tag", "tag.cache");
		img1->getPath("tag");
		img1->getPath("tag");

		auto ref = img1->addPath("tag2", "tag.cache");
		auto data3 = img1->popData();
		img1->removePath(ref);

		img1->addPath("tag3", "tag.cache");
		img1->addPath(StringView(), "tag.cache");
		auto data4 = img1->popData();
		img1->removePath("tag3");

		auto drawOrder = img1->getDrawOrder();

		auto data5 = img1->popData();
		img1->setDrawOrder(drawOrder);

		auto data6 = img1->popData();
		img1->setDrawOrder(sp::move(drawOrder));

		auto data7 = img1->popData();
		img1->resetDrawOrder();

		auto data8 = img1->popData();
		img1->setViewBoxTransform(geom::Mat4::ROTATION_Z_270);
		img1->getViewBoxTransform();

		auto data9 = img1->popData();
		img1->setBatchDrawing(true);

		auto data10 = img1->popData();
		img1->setBatchDrawing(false);

		img1->isBatchDrawing();

		auto pathRef2 = img1->addPath(path, "tag", "tag.cache");
		pathRef2->count();
		pathRef2->setPath(pathStr);
		pathRef2->setPath(pathData);
		pathRef2->markCopyOnWrite();
		pathRef2->setFillColor(geom::Color::Amber_300);
		pathRef2->getFillColor();
		pathRef2->markCopyOnWrite();
		pathRef2->setStrokeColor(geom::Color::Amber_300);
		pathRef2->getStrokeColor();

		pathRef2->markCopyOnWrite();
		pathRef2->setFillOpacity(134);
		pathRef2->getFillOpacity();
		pathRef2->markCopyOnWrite();
		pathRef2->setStrokeOpacity(134);
		pathRef2->getStrokeOpacity();

		pathRef2->markCopyOnWrite();
		pathRef2->setStrokeWidth(2.0f);
		pathRef2->getStrokeWidth();

		pathRef2->markCopyOnWrite();
		pathRef2->setTransform(geom::Mat4::ROTATION_Z_180);
		pathRef2->markCopyOnWrite();
		pathRef2->applyTransform(geom::Mat4::IDENTITY);
		pathRef2->getTransform();

		pathRef2->markCopyOnWrite();
		pathRef2->setWindingRule(vg::Winding::EvenOdd);
		pathRef2->getWindingRule();

		pathRef2->empty();
		pathRef2->valid();
		pathRef2->markCopyOnWrite();
		pathRef2->clear();

		return (*pathRef2 ? true : false);
	}

	void testPath() {
		auto filepath = filesystem::currentDir<Interface>("resources/24px.svg");

		vg::PathData<mem_std::Interface> data;
		data.getWriter().readFromFileContent(s_svg2);
		auto pathStr = data.toString<Interface>(false);
		auto pathData = data.encode<Interface>();

		vg::PathData<mem_pool::Interface> pdata;
		pdata.getWriter().readFromFileContent(s_svg2);

		vg::VectorPath path(100);
		path.init();
		path.init(FilePath(filepath));

		vg::VectorPath path2(100);
		path2 = path;
		path2.reserve(100);
		path2.init(data);
		path2.addPath(pathStr);
		path2.addPath(pathData);
		path2.setFillColor(geom::Color::Orange_500, true);
		path2.setFillColor(geom::Color::Orange_500, false);
		path2.getLineCup();
		path2.getLineJoin();
		path2.getMiterLimit();

		vg::VectorPath path3(100);
		path3 = move(path);
		path3.init(pdata);
		path3.addPath(path2);
		path3.setStrokeColor(geom::Color::Orange_500, true);
		path3.setStrokeColor(geom::Color::Orange_500, false);
		path3.getCommands();
		path3.getPoints();
		path3.commandsCount();
		path3.dataCount();
	}

	virtual bool run(pool_t *) override {
		StringStream stream; stream << "\n";

		vg::CommandData commandData;

		readSvg(s_svg1);
		testPathData();
		testImage();
		testPath();

		_desc = stream.str();
		return true;
	}

} _VgTest;

}
