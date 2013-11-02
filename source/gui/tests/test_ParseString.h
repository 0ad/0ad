/* Copyright (C) 2013 Wildfire Games.
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

#include "gui/GUIbase.h"

class TestGuiParseString : public CxxTest::TestSuite 
{
public:
	void test_clientarea()
	{
		CClientArea ca;
		TS_ASSERT(ca.SetClientArea("0 1 2 3"));
		TS_ASSERT_EQUALS(ca, CClientArea(CRect(0, 1, 2, 3), CRect(0, 0, 0, 0)));

		TS_ASSERT(ca.SetClientArea("5% 10%+1 20%-2 3"));
		TS_ASSERT_EQUALS(ca, CClientArea(CRect(0, 1, -2, 3), CRect(5, 10, 20, 0)));

		TS_ASSERT(!ca.SetClientArea("0+5% 1 2 3"));

		TS_ASSERT(!ca.SetClientArea("5%+10-10 1 2 3"));

		TS_ASSERT(ca.SetClientArea("5% 10%-1 -20%-2 3"));
		TS_ASSERT_EQUALS(ca, CClientArea(CRect(0, -1, -2, 3), CRect(5, 10, -20, 0)));

		TS_ASSERT(!ca.SetClientArea("5% 10%+1 -20%-2 3")); // parser bug
	}
};
