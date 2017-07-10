/* Copyright (C) 2010 Wildfire Games.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "lib/lib.h"
#include "lib/self_test.h"

#include "lib/path.h"

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
