/* Copyright (C) 2009 Wildfire Games.
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

#include "graphics/Color.h"

class TestColor : public CxxTest::TestSuite
{
public:
	void setUp()
	{
		ColorActivateFastImpl();
	}

	void test_Color4ub()
	{
		CheckColor(0, 0, 0, 0x000000);
		CheckColor(1, 0, 0, 0x0000ff);
		CheckColor(0, 1, 0, 0x00ff00);
		CheckColor(0, 0, 1, 0xff0000);
		CheckColor(1, 1, 1, 0xffffff);
	}

private:
	void CheckColor(int r, int g, int b, u32 expected)
	{
		SColor4ub colorStruct = ConvertRGBColorTo4ub(RGBColor(r,g,b));
		u32 actual;
		memcpy(&actual, &colorStruct, sizeof(u32));
		expected |= 0xff000000;	// ConvertRGBColorTo4ub sets alpha to opaque
		TS_ASSERT_EQUALS(expected, actual);
	}
};
