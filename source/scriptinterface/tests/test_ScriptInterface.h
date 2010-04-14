/* Copyright (C) 2010 Wildfire Games.
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

#include "scriptinterface/ScriptInterface.h"

#include "ps/CLogger.h"

class TestScriptInterface : public CxxTest::TestSuite
{
public:
	void test_loadscript_basic()
	{
		ScriptInterface script("Test");
		TestLogger logger;
		TS_ASSERT(script.LoadScript(L"test.js", L"var x = 1+1;"));
		TS_ASSERT_WSTR_NOT_CONTAINS(logger.GetOutput(), L"JavaScript error");
		TS_ASSERT_WSTR_NOT_CONTAINS(logger.GetOutput(), L"JavaScript warning");
	}

	void test_loadscript_error()
	{
		ScriptInterface script("Test");
		TestLogger logger;
		TS_ASSERT(!script.LoadScript(L"test.js", L"1+"));
		TS_ASSERT_WSTR_CONTAINS(logger.GetOutput(), L"JavaScript error: test.js line 1\nSyntaxError: syntax error");
	}

	void test_loadscript_strict_warning()
	{
		ScriptInterface script("Test");
		TestLogger logger;
		TS_ASSERT(script.LoadScript(L"test.js", L"1+1;"));
		TS_ASSERT_WSTR_CONTAINS(logger.GetOutput(), L"JavaScript warning: test.js line 1\nuseless expression");
	}

	void test_loadscript_strict_error()
	{
		ScriptInterface script("Test");
		TestLogger logger;
		TS_ASSERT(!script.LoadScript(L"test.js", L"with(1){}"));
		TS_ASSERT_WSTR_CONTAINS(logger.GetOutput(), L"JavaScript error: test.js line 1\nSyntaxError: strict mode code may not contain \'with\' statements");
	}
};
