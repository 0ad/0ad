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

#include "lib/self_test.h"

#include "lib/regex.h"

class TestRegex : public CxxTest::TestSuite
{
public:
	void test_regex()
	{
		TS_ASSERT_EQUALS(match_wildcard(L"", L""), 1);
		TS_ASSERT_EQUALS(match_wildcard(L"a", 0), 1);	// NULL matches everything

		TS_ASSERT_EQUALS(match_wildcard(L"abc", L"abc")     , 1);	// direct match
		TS_ASSERT_EQUALS(match_wildcard(L"abc", L"???")     , 1);	// only ?
		TS_ASSERT_EQUALS(match_wildcard(L"abc", L"*"  )     , 1);	// only *

		TS_ASSERT_EQUALS(match_wildcard(L"ab" , L"a?" )     , 1);	// trailing ?
		TS_ASSERT_EQUALS(match_wildcard(L"abc", L"a?c")     , 1);	// middle ?
		TS_ASSERT_EQUALS(match_wildcard(L"abc", L"?bc")     , 1);	// leading ?

		TS_ASSERT_EQUALS(match_wildcard(L"abc", L"a*" )     , 1);	// trailing *
		TS_ASSERT_EQUALS(match_wildcard(L"abcdef", L"ab*ef"), 1);	// middle *
		TS_ASSERT_EQUALS(match_wildcard(L"abcdef", L"*f"   ), 1);	// leading *

		TS_ASSERT_EQUALS(match_wildcard(L"abcdef", L"a?cd*"), 1);	// ? and *
		TS_ASSERT_EQUALS(match_wildcard(L"abcdef", L"a*d?f"), 1);	// * and ?
		TS_ASSERT_EQUALS(match_wildcard(L"abcdef", L"a*d*" ), 1);	// multiple *
	}
};
