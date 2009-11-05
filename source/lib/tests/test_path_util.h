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

#include "lib/lib.h"
#include "lib/self_test.h"

#include "lib/path_util.h"

// Macros, not functions, to get proper line number reports when tests fail

#define TEST_NAME_ONLY(path, correct_result) \
{ \
	const wchar_t* result = path_name_only(path); \
	TS_ASSERT_WSTR_EQUALS(result, correct_result); \
}

class TestPathUtil : public CxxTest::TestSuite 
{
public:

	void test_subpath()
	{
		// obvious true
		TS_ASSERT(path_is_subpath(L"abc/def/", L"abc/def/") == true);	// same
		TS_ASSERT(path_is_subpath(L"abc/def/", L"abc/") == true);	// 2 is subpath
		TS_ASSERT(path_is_subpath(L"abc/", L"abc/def/") == true);	// 1 is subpath

		// nonobvious true
		TS_ASSERT(path_is_subpath(L"", L"") == true);
		TS_ASSERT(path_is_subpath(L"abc/def/", L"abc/def") == true);	// no '/' !

		// obvious false
		TS_ASSERT(path_is_subpath(L"abc", L"def") == false);	// different, no path

		// nonobvious false
		TS_ASSERT(path_is_subpath(L"abc", L"") == false);	// empty comparand
		// .. different but followed by common subdir
		TS_ASSERT(path_is_subpath(L"abc/def/", L"ghi/def/") == false);
		TS_ASSERT(path_is_subpath(L"abc/def/", L"abc/ghi") == false);
	}

	// TODO: can't test path validate yet without suppress-error-dialog

	void test_name_only()
	{
		// path with filename
		TEST_NAME_ONLY(L"abc/def", L"def");
		// nonportable path with filename
		TEST_NAME_ONLY(L"abc\\def\\ghi", L"ghi");
		// mixed path with filename
		TEST_NAME_ONLY(L"abc/def\\ghi", L"ghi");
		// mixed path with filename (2)
		TEST_NAME_ONLY(L"abc\\def/ghi", L"ghi");
		// filename only
		TEST_NAME_ONLY(L"abc", L"abc");
		// empty
		TEST_NAME_ONLY(L"", L"");
	}
};
