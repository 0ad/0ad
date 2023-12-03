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

#include "ps/CLogger.h"
#include "ps/XML/Xeromyces.h"
#include "lib/file/vfs/vfs.h"

class TestXeromyces : public CxxTest::TestSuite
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

	// TODO: Should test the reading/parsing/writing code,
	// and parse error handling

	void test_LoadString()
	{
		CXeromyces xero;
		TS_ASSERT_EQUALS(xero.LoadString("<test><foo>bar</foo></test>"), PSRETURN_OK);
		TS_ASSERT_STR_EQUALS(xero.GetElementString(xero.GetRoot().GetNodeName()), "test");
	}

	void test_LoadString_invalid()
	{
		TestLogger logger;
		CXeromyces xero;
		TS_ASSERT_EQUALS(xero.LoadString("<test>"), PSRETURN_Xeromyces_XMLParseError);
	}
};
