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

#ifdef MODULE_STAPPLER_GEOM

#include "SPVec2.h"
#include "SPVec3.h"
#include "SPVec4.h"
#include "SPMat4.h"
#include "SPQuaternion.h"
#include "SPColor.h"
#include "SPColorHCT.h"
#include "SPGeometry.h"
#include "SPPadding.h"

namespace STAPPLER_VERSIONIZED stappler::app::test {

static void runColorTest() {
	geom::Color3B color3B = geom::Color3B::getColorByName("#ABABAB");
	geom::Color4B color4B = geom::Color4B::getColorByName("#ABABAB");

	geom::readColor(StringView("rgb(127,127,127)"), color3B);
	geom::readColor(StringView("hsl(127,50%,50%)"), color3B);
	geom::readColor(StringView("#FFF"), color3B);
	geom::readColor(StringView("#ABABAB"), color3B);
	geom::readColor(StringView("white"), color3B);
	geom::readColor(StringView("silver"), color3B);
	geom::readColor(StringView("gray"), color3B);
	geom::readColor(StringView("grey"), color3B);
	geom::readColor(StringView("black"), color3B);
	geom::readColor(StringView("maroon"), color3B);
	geom::readColor(StringView("red"), color3B);
	geom::readColor(StringView("orange"), color3B);
	geom::readColor(StringView("yellow"), color3B);
	geom::readColor(StringView("olive"), color3B);
	geom::readColor(StringView("lime"), color3B);
	geom::readColor(StringView("green"), color3B);
	geom::readColor(StringView("aqua"), color3B);
	geom::readColor(StringView("blue"), color3B);
	geom::readColor(StringView("navy"), color3B);
	geom::readColor(StringView("teal"), color3B);
	geom::readColor(StringView("fuchsia"), color3B);
	geom::readColor(StringView("purple"), color3B);
	geom::readColor(StringView("Red50"), color3B);
	geom::readColor(StringView("Pink50"), color3B);
	geom::readColor(StringView("Purple50"), color3B);
	geom::readColor(StringView("DeepPurple50"), color3B);
	geom::readColor(StringView("Indigo50"), color3B);
	geom::readColor(StringView("Blue50"), color3B);
	geom::readColor(StringView("LightBlue50"), color3B);
	geom::readColor(StringView("Cyan50"), color3B);
	geom::readColor(StringView("Teal50"), color3B);
	geom::readColor(StringView("Green50"), color3B);
	geom::readColor(StringView("LightGreen50"), color3B);
	geom::readColor(StringView("Lime50"), color3B);
	geom::readColor(StringView("Yellow50"), color3B);
	geom::readColor(StringView("Amber50"), color3B);
	geom::readColor(StringView("Orange50"), color3B);
	geom::readColor(StringView("DeepOrange50"), color3B);
	geom::readColor(StringView("Brown50"), color3B);
	geom::readColor(StringView("Grey50"), color3B);
	geom::readColor(StringView("BlueGrey50"), color3B);

	geom::readColor(StringView("rgb(127,257,50%)"), color4B);
	geom::readColor(StringView("rgba(1.0,1.0,1.0,1.0)"), color4B);
	geom::readColor(StringView("hsl(127,50%,50%)"), color4B);
	geom::readColor(StringView("hsla(361,50%,102%,1.1)"), color4B);
	geom::readColor(StringView("#FFF"), color4B);
	geom::readColor(StringView("#FFF0"), color4B);
	geom::readColor(StringView("#ABABAB"), color4B);
	geom::readColor(StringView("#ABABAB00"), color4B);
	geom::readColor(StringView("white"), color4B);
	geom::readColor(StringView("silver"), color4B);
	geom::readColor(StringView("gray"), color4B);
	geom::readColor(StringView("grey"), color4B);
	geom::readColor(StringView("black"), color4B);
	geom::readColor(StringView("maroon"), color4B);
	geom::readColor(StringView("red"), color4B);
	geom::readColor(StringView("orange"), color4B);
	geom::readColor(StringView("yellow"), color4B);
	geom::readColor(StringView("olive"), color4B);
	geom::readColor(StringView("lime"), color4B);
	geom::readColor(StringView("green"), color4B);
	geom::readColor(StringView("aqua"), color4B);
	geom::readColor(StringView("blue"), color4B);
	geom::readColor(StringView("navy"), color4B);
	geom::readColor(StringView("teal"), color4B);
	geom::readColor(StringView("fuchsia"), color4B);
	geom::readColor(StringView("purple"), color4B);
	geom::readColor(StringView("Red50"), color4B);
	geom::readColor(StringView("Pink50"), color4B);
	geom::readColor(StringView("Purple50"), color4B);
	geom::readColor(StringView("DeepPurple50"), color4B);
	geom::readColor(StringView("Indigo50"), color4B);
	geom::readColor(StringView("Blue50"), color4B);
	geom::readColor(StringView("LightBlue50"), color4B);
	geom::readColor(StringView("Cyan50"), color4B);
	geom::readColor(StringView("Teal50"), color4B);
	geom::readColor(StringView("Green50"), color4B);
	geom::readColor(StringView("LightGreen50"), color4B);
	geom::readColor(StringView("Lime50"), color4B);
	geom::readColor(StringView("Yellow50"), color4B);
	geom::readColor(StringView("Amber50"), color4B);
	geom::readColor(StringView("Orange50"), color4B);
	geom::readColor(StringView("DeepOrange50"), color4B);
	geom::readColor(StringView("Brown50"), color4B);
	geom::readColor(StringView("Grey50"), color4B);
	geom::readColor(StringView("BlueGrey50"), color4B);

	geom::Color color = color4B;
	color.lighter(1);
	color.darker(1);
	color.previous();
	color.next();
	color.medium();
	color.specific(2).text();
	color.specific(geom::Color::Level::b800).text();

	geom::Color4F color4F(color4B);

	color3B.name<mem_pool::Interface>();
	color3B.name<mem_std::Interface>();
	color.name<mem_pool::Interface>();
	color.name<mem_std::Interface>();

	geom::Color color2(geom::Color::getColorByName("maroon").value());
	color2.lighter(1);
	color2.darker(1);
	color2.previous();
	color2.next();
	color2.medium();
	color2.specific(2).text();
	color2.specific(geom::Color::Level::b800).text();

	geom::Color color3(geom::Color::Red_A200);
	color3.lighter(1);
	color3.darker(1);
	color3.previous();
	color3.next();
	color3.medium();
	color3.specific(2).text();
	color3.specific(geom::Color::Level::b800).text();

	geom::Color color4(geom::Color::getColorByName("#123456"));
	color4.lighter(1);
	color4.darker(1);
	color4.previous();
	color4.next();
	color4.medium();
	color4.specific(2).text();
	color4.specific(geom::Color::Level::b800).text();
	color4.name<mem_pool::Interface>();
	color4.name<mem_std::Interface>();
	color4.asColor3B().name<mem_pool::Interface>();
	color4.asColor3B().name<mem_std::Interface>();

	progress(color, color2, 0.5f);
	progress(color3B, color2.asColor3B(), 0.5f);
	progress(color4B, color2.asColor4B(), 0.5f);
	progress(color4F, color2.asColor4F(), 0.5f);

	color4F.setMasked(color4F, geom::ColorMask::Color);
	color4F.setUnmasked(color4F, geom::ColorMask::A);
	color4F.getColor();
	color4F.getOpacity();

	StringStream out;

	auto color3BInput = [&] (const geom::Color3B &c) {
		out << c << "\n";
	};
	auto color4BInput = [&] (const geom::Color4B &c) {
		out << c << "\n";
	};
	auto color4FInput = [&] (const geom::Color4F &c) {
		out << c << "\n";
	};

	out << color3B << "\n";
	out << geom::Color3B(color4B) << "\n";
	out << color4B << "\n";
	out << geom::Color4B::white(123) << "\n";
	out << geom::Color4B::black(123) << "\n";
	out << color << "\n";
	out << color4F << "\n";
	out << color4F + color4F << "\n";
	out << color4F * color4F << "\n";
	out << color4F * 0.5f << "\n";
	out << geom::Color4B(color3B, 123) << "\n";
	out << geom::Color4F(color3B, 123) << "\n";
	out << geom::Color4B(color4F) << "\n";
	out << geom::Color4B() << "\n";
	out << geom::Color() << "\n";
	out << geom::Color(geom::Color::getColorByName("maroon").value()) << "\n";
	out << geom::Color(color.value()) << "\n";
	out << geom::Color(color.value(), color.index()) << "\n";
	out << geom::Color(geom::Color::Tone::BlueGrey, geom::Color::Level::b100) << "\n";

	color3BInput(color);
	color4BInput(color);
	color4FInput(color);

	out << (color3B == color3B) << (color3B != color3B)
		<< (color3B == color4B) << (color3B != color4B)
		<< (color3B == color4F) << (color3B != color4F)
		<< (color4F == color3B) << (color4F != color3B)
		<< (color4F == color4B) << (color4F != color4B)
		<< (color4F == color4F) << (color4F != color4F)
		<< (color4B == color3B) << (color4B != color3B)
		<< (color4B == color4B) << (color4B != color4B)
		<< (color4B == color4F) << (color4B != color4F)
		<< "\n";

	geom::Cam16::signum(0);
	geom::Cam16::LstarFromY(0.01);

	geom::ColorHCT colorHCT(color4F);
	geom::ColorHCT::solveColorHCT(colorHCT.data.hue, colorHCT.data.chroma, colorHCT.data.tone, colorHCT.data.alpha);

	out << colorHCT << "\n";
	out << geom::ColorHCT(color4F, 0.5) << "\n";
}

static void runMetricTest() {
	geom::Metric m;
	m.readStyleValue("auto", false, true);
	m.readStyleValue("0.0px", false, true);
	m.readStyleValue("20%", false, true);
	m.readStyleValue("20.0em", false, true);
	m.readStyleValue("20.0rem", false, true);
	m.readStyleValue("20.0px", false, true);
	m.readStyleValue("20.0pt", false, true);
	m.readStyleValue("20.0pc", false, true);
	m.readStyleValue("20.0mm", false, true);
	m.readStyleValue("20.0cm", false, true);
	m.readStyleValue("20.0in", false, true);
	m.readStyleValue("20.0vw", false, true);
	m.readStyleValue("20.0vh", false, true);
	m.readStyleValue("20.0vmax", false, true);
	m.readStyleValue("20.0vmin", false, true);
	m.readStyleValue("20.0dpi", true, true);
	m.readStyleValue("20.0dpcm", true, true);
	m.readStyleValue("20.0dppx", true, true);
	m.readStyleValue("20.0", false, true);
	m.readStyleValue("20.0", false, false);

	geom::Rect rect(0.0f, 0.0f, 10.0f, 10.0f);
	geom::Rect rect2(5.0f, 5.0f, 15.0f, 15.0f);
	rect.containsPoint(geom::Vec2(5.0f, 5.0f));
	rect.containsPoint(geom::Vec2(15.0f, 15.0f));
	rect.intersectsRect(rect2);
	rect.intersectsCircle(geom::Vec2(15.0f, 15.0f), 4.0f);
	rect.intersectsCircle(geom::Vec2(5.0f, 13.0f), 4.0f);
	rect.intersectsCircle(geom::Vec2(13.0f, 5.0f), 4.0f);
	rect.intersectsCircle(geom::Vec2(13.0f, 13.0f), 4.0f);

	geom::Rect rect3(0.0f, 0.0f, 10.0f, 10.0f);
	rect3.merge(rect2);

	geom::Rect rect4(0.0f, 0.0f, 10.0f, 10.0f);
	rect4.unionWithRect(rect2);

	geom::Rect rect5(5.0f, 5.0f, 15.0f, 15.0f);
	rect5.unionWithRect(rect);

	geom::URect urect(5, 5, 15, 15);
	urect.containsPoint(geom::UVec2{10, 10});
	urect.containsPoint(geom::UVec2{0, 0});

	rect5.equals(rect2);

	geom::Padding padding((rand() % 100) / 100.0f, (rand() % 100) / 100.0f, (rand() % 100) / 100.0f, (rand() % 100) / 100.0f);

	float values[4];
	geom::Mat4 mat((rand() % 100) / 100.0f, 0, 0, (rand() % 100) / 100.0f, 0, 0);
	mat.getRotation(values);
	mat.getTranslation(values);

	geom::Mat4 mat2;
	mat.scale(geom::Vec3(1.0f, 2.0f, 3.0f), &mat2);
	geom::Mat4::createBillboard(geom::Vec3(0.0f, 0.0f, 0.0f), geom::Vec3(10.0f, 10.0f, 10.0f), geom::Vec3::UNIT_Z, &mat2);
	geom::Mat4::createBillboard(geom::Vec3(0.0f, 0.0f, 0.0f), geom::Vec3(10.0f, 10.0f, 10.0f), geom::Vec3::UNIT_Z, geom::Vec3::UNIT_X, &mat2);

	geom::Mat4::createLookAt(geom::Vec3(0.0f, 0.0f, 0.0f), geom::Vec3(10.0f, 10.0f, 10.0f), geom::Vec3::UNIT_Z, &mat2);
	geom::Mat4::createLookAt(0.0f, 0.0f, 0.0f, 10.0f, 10.0f, 10.0f, 0.0f, 0.0f, 1.0f, &mat2);

	geom::Mat4::createPerspective(90.0f, 1.0f, 0.0f, 100.0f, &mat2);
	geom::Mat4::createOrthographic(120.0f, 90.0f, 0.0f, 100.0f, &mat2);
	geom::Mat4::createOrthographicOffCenter(120.0f, 90.0f, 120.0f, 90.0f, 0.0f, 100.0f, &mat2);
	geom::Mat4::createScale(geom::Vec3(1.0f, 1.0f, 1.0f), &mat2);
	geom::Mat4::createScale(1.0f, 1.0f, 1.0f, &mat2);
	geom::Mat4::createRotation(geom::Vec3::UNIT_Z, numbers::pi, &mat2);
	geom::Mat4::createRotation(geom::Vec3::UNIT_Z * 1.5f, numbers::pi, &mat2);
	geom::Mat4::createRotationX(numbers::pi, &mat2);
	geom::Mat4::createRotationY(numbers::pi, &mat2);
	geom::Mat4::createRotationZ(numbers::pi, &mat2);
	geom::Mat4::createTranslation(geom::Vec3::UNIT_Z, &mat2);

	geom::Vec3 vecA, vecB;
	geom::Quaternion quat;

	mat2 = geom::Mat4(
		-1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, -1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 0.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f );
	mat2.decompose(&vecA, &quat, &vecB);

	mat2 = geom::Mat4(
		-2.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, -4.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 0.0f );
	mat2.decompose(&vecA, &quat, &vecB);

	mat2 = geom::Mat4(
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, -3.0f, 0.0f, 0.0f,
		0.0f, 0.0f, -4.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 0.0f );
	mat2.decompose(&vecA, &quat, &vecB);

	mat2.getRotation(&quat);
	mat2.getTranslation(&vecA);
	mat2.getUpVector(&vecA);
	mat2.getDownVector(&vecA);
	mat2.getLeftVector(&vecA);
	mat2.getRightVector(&vecA);
	mat2.getForwardVector(&vecA);
	mat2.getBackVector(&vecA);

	geom::Mat4 mat3;

	mat2.rotate(geom::Vec3::UNIT_Z, numbers::pi);
	mat2.rotate(geom::Vec3::UNIT_Z, numbers::pi, &mat3);
	mat2.rotateX(numbers::pi);
	mat2.rotateX(numbers::pi, &mat3);
	mat2.rotateY(numbers::pi);
	mat2.rotateY(numbers::pi, &mat3);
	mat2.rotateZ(numbers::pi);
	mat2.rotateZ(numbers::pi, &mat3);

	mat2.scale(2.0f);
	mat2.scale(2.0f, &mat3);

	mat2.translate(geom::Vec3::UNIT_Z, &mat3);

	geom::Mat4 mat4;
	geom::Mat4::createRotation(geom::Vec3(3.0f, 5.0f, 1.0f).getNormalized(), numbers::pi / 3.0f, &mat4);

	geom::Quaternion q(geom::Vec3(1.0f, 2.0f, 4.0f).getNormalized(), numbers::pi / 3.0f);
	geom::Quaternion q2(mat4);

	geom::Quaternion q3(q);
	q3.inverse();
	q3 = q.getInversed();

	q3.multiply(q);
	geom::Quaternion::multiply(q, q2, &q3);

	geom::Quaternion q4(1.0f, 2.0f, 3.0f, 4.0f);
	auto q5 = q4.getNormalized();

	geom::Quaternion::lerp(q, q, 0.5f, &q4);
	geom::Quaternion::lerp(q, q5, 0.5f, &q4);
	geom::Quaternion::lerp(q, q5, 0.0f, &q4);
	geom::Quaternion::lerp(q, q5, 1.0f, &q4);

	geom::Quaternion::slerp(q, q, 0.5f, &q4);
	geom::Quaternion::slerp(q, q5, 0.0f, &q4);
	geom::Quaternion::slerp(q, q5, 0.5f, &q4);
	geom::Quaternion::slerp(q, q5, 1.0f, &q4);

	geom::Quaternion::squad(q, q5, q2, q3, 0.0f, &q4);
	geom::Quaternion::squad(q, q5, q2, q3, 0.5f, &q4);
	geom::Quaternion::squad(q, q5, q2, q3, 1.0f, &q4);

	geom::Quaternion q6(1.0f, 2.0f, 3.0f, 4.0f);
	q6.toAxisAngle(&vecA);
	q6.getInversed();
	q6.getNormalized();

	geom::Vec2 A(1.0f, 2.0f);
	geom::Vec2 B(2.0f, 20.0f);
	geom::Vec2 C(21.0f, -1.0f);
	geom::Vec2 D(20.0f, 21.0f);

	geom::Vec2 E, F;
	float S, T;

	geom::Vec2::isLineIntersect(A, B, C, D, &S, &T);
	geom::Vec2::isLineIntersect(A, D, B, C, &S, &T);
	geom::Vec2::isLineOverlap(A, B, C, D);
	geom::Vec2::isLineOverlap(A, D, B, C);
	geom::Vec2::isSegmentOverlap(A, B, C, D, &E, &F);
	geom::Vec2::isSegmentOverlap(A, D, B, C, &E, &F);
	geom::Vec2::isSegmentIntersect(A, B, C, D);
	geom::Vec2::isSegmentIntersect(A, D, B, C);
	geom::Vec2::isLineParallel(A, B, C, D);
	geom::Vec2::isLineParallel(A, D, B, C);
	geom::Vec2::getIntersectPoint(A, D, B, C);

	A = geom::Vec2(1.0f, 0.0f);
	B = geom::Vec2(12.0f, 0.0f);
	C = geom::Vec2(2.0f, 0.0f);
	D = geom::Vec2(20.0f, 0.0f);

	geom::Vec2::isSegmentOverlap(A, B, C, D, &E, &F);
	geom::Vec2::isSegmentOverlap(A, D, B, C, &E, &F);

	A.rotateByAngle(B, 1.0f);
	A.getAngle(C);
	A.rotate(B, 1.0f);
	A.rotate(geom::Vec2::ZERO, 1.0f);
	A.getNormalized();

	D.clamp(geom::Vec2(25, 25), geom::Vec2(50, 50));
	D = geom::Vec2(20.0f, 21.0f);
	D.clamp(geom::Vec2(-25, -25), geom::Vec2(-10, -10));

	geom::Vec2::clamp(geom::Vec2(20.0f, 21.0f), geom::Vec2(25, 25), geom::Vec2(50, 50), &D);
	geom::Vec2::clamp(geom::Vec2(20.0f, 21.0f), geom::Vec2(-25, -25), geom::Vec2(-10, -10), &D);

	geom::Vec2::angle(C, D);

	geom::Vec2 G(geom::Extent2(rand() % 10, rand() % 10));
	geom::Vec2::dot(C / 10.0f, -D);
	C.getAngle();
	D.project(C);

	geom::Vec3 v3_A(geom::Size3((rand() % 100) / 100.0f, (rand() % 100) / 100.0f, (rand() % 100) / 100.0f)),
			v3_B((rand() % 100) / 100.0f, (rand() % 100) / 100.0f, (rand() % 100) / 100.0f),
			v3_C(geom::Extent3(rand() % 100, rand() % 100, rand() % 100));

	auto v3_D = v3_C.getNormalized();
	geom::Vec3::angle(v3_A, v3_B);

	geom::Vec3::clamp(v3_A, geom::Vec3(-2.0f, -2.0f, -2.0f), geom::Vec3(-1.0f, -1.0f, -1.0f), &v3_D);
	geom::Vec3::clamp(v3_A, geom::Vec3(1.0f, 1.0f, 1.0f), geom::Vec3(2.0f, 2.0f, 2.0f), &v3_D);
	v3_A.clamp(geom::Vec3(-2.0f, -2.0f, -2.0f), geom::Vec3(-1.0f, -1.0f, -1.0f));
	v3_B.clamp(geom::Vec3(1.0f, 1.0f, 1.0f), geom::Vec3(2.0f, 2.0f, 2.0f));
	v3_D = v3_B - v3_A;

	geom::Vec4 v4_A((rand() % 100) / 100.0f, (rand() % 100) / 100.0f, (rand() % 100) / 100.0f, (rand() % 100) / 100.0f),
			v4_B((rand() % 100) / 100.0f, (rand() % 100) / 100.0f, (rand() % 100) / 100.0f, (rand() % 100) / 100.0f);
	geom::Vec4 v4_C = v4_B.getNormalized();

	geom::Vec4::angle(v4_A, v4_B);
	geom::Vec4::dot(v4_A, v4_B);

	geom::Vec4::clamp(v4_A, geom::Vec4(-2.0f, -2.0f, -2.0f, -2.0f), geom::Vec4(-1.0f, -1.0f, -1.0f, -1.0f), &v4_C);
	geom::Vec4::clamp(v4_A, geom::Vec4(1.0f, 1.0f, 1.0f, 1.0f), geom::Vec4(2.0f, 2.0f, 2.0f, 2.0f), &v4_C);
	v4_A.clamp(geom::Vec4(-2.0f, -2.0f, -2.0f, -2.0f), geom::Vec4(-1.0f, -1.0f, -1.0f, -1.0f));
	v4_B.clamp(geom::Vec4(1.0f, 1.0f, 1.0f, 1.0f), geom::Vec4(2.0f, 2.0f, 2.0f, 2.0f));

	v4_C.add(geom::Vec4(1.0f, 1.0f, 1.0f, 1.0f));
	v4_C += geom::Vec4(1.0f, 1.0f, 1.0f, 1.0f);
}

struct GeomTest : Test {
	GeomTest() : Test("GeomTest") { }

	virtual bool run() override {
		StringStream stream;
		size_t count = 0;
		size_t passed = 0;
		stream << "\n";

		runTest(stream, "ApplyTest", count, passed, [&] {
			geom::Vec2 vec2_1(0.5f, -1.5f);
			geom::Vec3 vec3_1(0.5f, -1.5f, 2.5f);
			geom::Vec4 vec4_1(0.5f, -1.5f, 2.5f, -3.5f);

			return geom::_abs(-1.5f) == std::abs(-1.5f)
					&& geom::_abs(vec2_1) == geom::Vec2(std::abs(vec2_1.x), std::abs(vec2_1.y))
					&& geom::_abs(vec3_1) == geom::Vec3(std::abs(vec3_1.x), std::abs(vec3_1.y), std::abs(vec3_1.z))
					&& geom::_abs(vec4_1) == geom::Vec4(std::abs(vec4_1.x), std::abs(vec4_1.y), std::abs(vec4_1.z), std::abs(vec4_1.w))
					&& geom::_floor(-1.5f) == std::floor(-1.5f)
					&& geom::_floor(vec2_1) == geom::Vec2(std::floor(vec2_1.x), std::floor(vec2_1.y))
					&& geom::_floor(vec3_1) == geom::Vec3(std::floor(vec3_1.x), std::floor(vec3_1.y), std::floor(vec3_1.z))
					&& geom::_floor(vec4_1) == geom::Vec4(std::floor(vec4_1.x), std::floor(vec4_1.y), std::floor(vec4_1.z), std::floor(vec4_1.w));
		});

		runColorTest();
		runMetricTest();

		runTest(stream, "Mat4: extract scale", count, passed, [&] {
			geom::Mat4 mat;

			geom::Vec3 scale(2.0f, 3.0f, 4.0f);
			geom::Vec3 translate(10.0f, 20.0f, 30.0f);
			geom::Quaternion rotate(geom::Vec3::UNIT_Z, 1.0f);

			mat.scale(scale);
			mat.translate(translate);
			mat.rotate(rotate);

			geom::Vec3 extractedScale;
			geom::Vec3 extractedTranslation;
			geom::Quaternion extractedRotation;

			mat.decompose(&extractedScale, &extractedRotation, &extractedTranslation);

			geom::Vec3 extractedScale2;
			geom::Vec3 extractedTranslation2;
			geom::Quaternion extractedRotation2;

			mat.scale(1.0f / extractedScale.x, 1.0f / extractedScale.y, 1.0f / extractedScale.z);
			mat.decompose(&extractedScale2, &extractedRotation2, &extractedTranslation2);

			geom::Vec3 extractedScale3;
			geom::Vec3 extractedTranslation3;
			geom::Quaternion extractedRotation3;

			mat.scale(extractedScale.x, extractedScale.y, extractedScale.z);
			mat.decompose(&extractedScale3, &extractedRotation3, &extractedTranslation3);

			return extractedScale.fuzzyEquals(extractedScale3, math::MATH_TOLERANCE)
					&& extractedTranslation.fuzzyEquals(extractedTranslation3, math::MATH_TOLERANCE);
		});

		_desc = stream.str();
		return count == passed;
	}
} _GeomTest;

}

#endif
