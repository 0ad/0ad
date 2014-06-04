/* Copyright (C) 2014 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "lib/self_test.h"

#include "ps/Overlay.h"
#include "ps/CLogger.h"

class TestCColor : public CxxTest::TestSuite 
{
public:
	void test_parse()
	{
		TestLogger nolog;
#define CHECK_CCOLOR_EQUAL(v,r,g,b,a) \
	{ \
		CStr str = v; \
		CColor c; \
		TS_ASSERT(c.ParseString(str, 255)); \
		CColor c_(r/255.f,g/255.f,b/255.f,a/255.f); \
		TS_ASSERT_EQUALS(c, c_); \
	}

		CHECK_CCOLOR_EQUAL("0 0 0 0", 0, 0, 0, 0);
		CHECK_CCOLOR_EQUAL("255 0 255 0", 255, 0, 255, 0);
		CHECK_CCOLOR_EQUAL("0 123 0 0", 0, 123, 0, 0);
		CHECK_CCOLOR_EQUAL("0 0 0 55", 0, 0, 0, 55);
		CHECK_CCOLOR_EQUAL("76 24 0", 76, 24, 0, 255);
		CHECK_CCOLOR_EQUAL("159 98 24", 159, 98, 24, 255);
#undef CHECK_CCOLOR_EQUAL
	}

	void test_parse_failure()
	{
		TestLogger nolog;

#define CHECK_CCOLOR_FAIL(v) \
	{ \
		CStr str = v; \
		CColor a; \
		TS_ASSERT(!a.ParseString(str, 255)); \
	}

		CHECK_CCOLOR_FAIL("abc");
		CHECK_CCOLOR_FAIL("0.123 1 2 3");
		CHECK_CCOLOR_FAIL("0 0 0 0 adfasd");
		CHECK_CCOLOR_FAIL("0 a0 b0 ax");
		CHECK_CCOLOR_FAIL("-124dsaf");
		CHECK_CCOLOR_FAIL("\0");
		CHECK_CCOLOR_FAIL("1 2 xxxx");

		// Not enough parameters
		CHECK_CCOLOR_FAIL(""); 
		CHECK_CCOLOR_FAIL("124");
		CHECK_CCOLOR_FAIL("0 55");

		// More parameters than allowed
		CHECK_CCOLOR_FAIL("0 5 1 5 6");

		// Out of bounds
		CHECK_CCOLOR_FAIL("-1 0 0");
		CHECK_CCOLOR_FAIL("256 0 0 0");
		CHECK_CCOLOR_FAIL("255 256 -1 0");
#undef CHECK_CCLOLO_FAIL
	}
};
