/* Copyright (C) 2017 Wildfire Games.
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

#include "simulation2/system/ComponentTest.h"

#include "simulation2/components/ICmpCinemaManager.h"

class TestCmpCinemaManager : public CxxTest::TestSuite
{
public:
	void setUp()
	{
		CXeromyces::Startup();
	}

	void tearDown()
	{
		CXeromyces::Terminate();
	}

	void test_basic()
	{
		ComponentTestHelper test(g_ScriptRuntime);
		JSContext* cx = test.GetScriptInterface().GetContext();
		JSAutoRequest rq(cx);

		ICmpCinemaManager* cmp = test.Add<ICmpCinemaManager>(CID_CinemaManager, "", SYSTEM_ENTITY);

		TS_ASSERT_EQUALS(cmp->HasPath(L"test"), false);
		cmp->AddPath(generatePath(L"test"));
		TS_ASSERT_EQUALS(cmp->HasPath(L"test"), true);
		cmp->DeletePath(L"test");
		TS_ASSERT_EQUALS(cmp->HasPath(L"test"), false);

		cmp->AddPath(generatePath(L"long_path", fixed::FromInt(3600)));
		TS_ASSERT_EQUALS(cmp->HasPath(L"long_path"), true);

		TS_ASSERT_EQUALS(cmp->IsEnabled(), false);
		cmp->AddCinemaPathToQueue(L"long_path");
		cmp->Play();
		size_t number_of_turns = 0;
		while (cmp->IsEnabled())
		{
			CMessageUpdate msg(fixed::FromInt(36));
			cmp->HandleMessage(msg, true);
			++number_of_turns;
		}
		TS_ASSERT_EQUALS(number_of_turns, 100);
	}

private:
	// Generates a simple cinema path with two position and two target nodes.
	CCinemaPath generatePath(const CStrW& name, const fixed& duration = fixed::FromInt(1))
	{
		// Helper nodes
		CFixedVector3D nodeA(fixed::FromInt(1), fixed::FromInt(0), fixed::FromInt(1));
		CFixedVector3D nodeB(fixed::FromInt(9), fixed::FromInt(0), fixed::FromInt(9));
		CFixedVector3D shift(fixed::FromInt(3), fixed::FromInt(3), fixed::FromInt(3));

		// Constructs the default cinema path data
		CCinemaData pathData;
		pathData.m_Name = name;
		pathData.m_Timescale = fixed::FromInt(1);
		pathData.m_Orientation = L"target";
		pathData.m_Mode = L"ease_inout";
		pathData.m_Style = L"default";

		// Creates two parallel segments from the A node to the B node
		TNSpline positionSpline, targetSpline;
		positionSpline.AddNode(nodeA, CFixedVector3D(), fixed::FromInt(0));
		positionSpline.AddNode(nodeB, CFixedVector3D(), duration);
		targetSpline.AddNode(nodeA + shift, CFixedVector3D(), fixed::FromInt(0));
		targetSpline.AddNode(nodeB + shift, CFixedVector3D(), duration);

		return CCinemaPath(pathData, positionSpline, targetSpline);
	}
};
