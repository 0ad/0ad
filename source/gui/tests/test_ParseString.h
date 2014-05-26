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

#include "gui/GUIbase.h"
#include "gui/GUIutil.h"
#include "ps/CLogger.h"

class TestGuiParseString : public CxxTest::TestSuite 
{
public:
	void test_clientarea()
	{
		TestLogger nolog;
		CClientArea ca;

		// Test only pixels
		TS_ASSERT(ca.SetClientArea("0.0 -10 20.0 -30"));
		TS_ASSERT_EQUALS(ca, CClientArea(CRect(0, -10, 20, -30), CRect(0, 0, 0, 0)));

		// Test only pixels, but with math
		TS_ASSERT(ca.SetClientArea("0 -100-10+100 20+200-200 -30"));
		TS_ASSERT_EQUALS(ca, CClientArea(CRect(0, -10, 20, -30), CRect(0, 0, 0, 0)));

		// Test only percent
		TS_ASSERT(ca.SetClientArea("-5% 10.0% -20% 30.0%"));
		TS_ASSERT_EQUALS(ca, CClientArea(CRect(0, 0, 0, 0), CRect(-5, 10, -20, 30)));

		// Test only percent, but with math
		TS_ASSERT(ca.SetClientArea("15%-5%-15% 10% -20% 30%+500%-500%"));
		TS_ASSERT_EQUALS(ca, CClientArea(CRect(0, 0, 0, 0), CRect(-5, 10, -20, 30)));

		// Test mixed
		TS_ASSERT(ca.SetClientArea("5% -10 -20% 30"));
		TS_ASSERT_EQUALS(ca, CClientArea(CRect(0, -10, 0, 30), CRect(5, 0, -20, 0)));

		// Test mixed with math
		TS_ASSERT(ca.SetClientArea("5%+10%-10% 30%-10-30% 50-20%-50 30-100+100"));
		TS_ASSERT_EQUALS(ca, CClientArea(CRect(0, -10, 0, 30), CRect(5, 0, -20, 0)));

		// Test for fail with too many/few parameters
		TS_ASSERT(!ca.SetClientArea("10 20 30 40 50"));
		TS_ASSERT(!ca.SetClientArea("10 20 30"));

		// Test for fail with garbage data
		TS_ASSERT(!ca.SetClientArea("Hello world!"));
		TS_ASSERT(!ca.SetClientArea("abc 123 xyz 789"));
		TS_ASSERT(!ca.SetClientArea("300 wide, 400 high"));
	}

	void test_rect()
	{
		TestLogger nolog;
		CRect test;

		TS_ASSERT(__ParseString(CStrW(L"0.0 10.0 20.0 30.0"), test));
		TS_ASSERT_EQUALS(CRect(0.0, 10.0, 20.0, 30.0), test);

		TS_ASSERT(!__ParseString(CStrW(L"0 10 20"), test));
		TS_ASSERT(!__ParseString(CStrW(L"0 10 20 30 40"), test));
		TS_ASSERT(!__ParseString(CStrW(L"0,0 10,0 20,0 30,0"), test));
	}

	void test_size()
	{
		TestLogger nolog;
		CSize test;

		TS_ASSERT(__ParseString(CStrW(L"0.0 10.0"), test));
		TS_ASSERT_EQUALS(CSize(0.0, 10.0), test);

		TS_ASSERT(!__ParseString(CStrW(L"0"), test));
		TS_ASSERT(!__ParseString(CStrW(L"0 10 20"), test));
		TS_ASSERT(!__ParseString(CStrW(L"0,0 10,0"), test));
	}

	void test_pos()
	{
		TestLogger nolog;
		CPos test;

		TS_ASSERT(__ParseString(CStrW(L"0.0 10.0"), test));
		TS_ASSERT_EQUALS(CPos(0.0, 10.0), test);

		TS_ASSERT(!__ParseString(CStrW(L"0"), test));
		TS_ASSERT(!__ParseString(CStrW(L"0 10 20"), test));
		TS_ASSERT(!__ParseString(CStrW(L"0,0 10,0"), test));
	}
};
