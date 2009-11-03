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
#define TEST_APPEND(path1, path2, flags, correct_result) \
	{ \
		wchar_t dst[PATH_MAX] = {0}; \
		path_append(dst, path1, path2, flags); \
		TS_ASSERT_WSTR_EQUALS(dst, correct_result); \
	}

#define TEST_NAME_ONLY(path, correct_result) \
{ \
	const wchar_t* result = path_name_only(path); \
	TS_ASSERT_WSTR_EQUALS(result, correct_result); \
}

#define TEST_STRIP_FN(path_readonly, correct_result) \
{ \
	wchar_t path[PATH_MAX]; \
	path_copy(path, path_readonly); \
	path_strip_fn(path); \
	TS_ASSERT_WSTR_EQUALS(path, correct_result); \
}

class TestPathUtil : public CxxTest::TestSuite 
{
	void TEST_PATH_EXT(const wchar_t* path, const wchar_t* correct_result)
	{
		const wchar_t* result = path_extension(path);
		TS_ASSERT_WSTR_EQUALS(result, correct_result);
	}

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

	void test_append()
	{
		// simplest case
		TEST_APPEND(L"abc", L"def", 0, L"abc/def");
		// trailing slash
		TEST_APPEND(L"abc", L"def", PATH_APPEND_SLASH, L"abc/def/");
		// intervening slash
		TEST_APPEND(L"abc/", L"def", 0, L"abc/def");
		// nonportable intervening slash
		TEST_APPEND(L"abc\\", L"def", 0, L"abc\\def");
		// mixed path slashes
		TEST_APPEND(L"abc", L"def/ghi\\jkl", 0, L"abc/def/ghi\\jkl");
		// path1 empty
		TEST_APPEND(L"", L"abc/def/", 0, L"abc/def/");
		// path2 empty, no trailing slash
		TEST_APPEND(L"abc/def", L"", 0, L"abc/def");
		// path2 empty, require trailing slash
		TEST_APPEND(L"abc/def", L"", PATH_APPEND_SLASH, L"abc/def/");
		// require trailing slash, already exists
		TEST_APPEND(L"abc/", L"def/", PATH_APPEND_SLASH, L"abc/def/");
	}

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

	void test_strip_fn()
	{
		// path with filename
		TEST_STRIP_FN(L"abc/def", L"abc/");
		// nonportable path with filename
		TEST_STRIP_FN(L"abc\\def\\ghi", L"abc\\def\\");
		// mixed path with filename
		TEST_STRIP_FN(L"abc/def\\ghi", L"abc/def\\");
		// mixed path with filename (2)
		TEST_STRIP_FN(L"abc\\def/ghi", L"abc\\def/");
		// filename only
		TEST_STRIP_FN(L"abc", L"");
		// empty
		TEST_STRIP_FN(L"", L"");
		// path
		TEST_STRIP_FN(L"abc/def/", L"abc/def/");
		// nonportable path
		TEST_STRIP_FN(L"abc\\def\\ghi\\", L"abc\\def\\ghi\\");
	}

	void test_path_ext()
	{
		TEST_PATH_EXT(L"a/b/c.bmp", L"bmp");
		TEST_PATH_EXT(L"a.BmP", L"BmP");	// case sensitive
		TEST_PATH_EXT(L"c", L"");	// no extension
		TEST_PATH_EXT(L"", L"");	// empty
	}
};
