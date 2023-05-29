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

#include "SPMat4.h"
#include "SPVec3.h"
#include "SPQuaternion.h"

namespace stappler::app::test {

struct MathTest : Test {
	MathTest() : Test("MathTest") { }

	virtual bool run() override {
		StringStream stream;
		size_t count = 0;
		size_t passed = 0;
		stream << "\n";

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
} _MathTest;

}

#endif
