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

namespace STAPPLER_VERSIONIZED stappler::app::test {

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

		_desc = stream.str();
		return count == passed;
	}
} _GeomTest;

}

#endif
