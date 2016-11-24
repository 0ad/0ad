/* Copyright (c) 2012 Wildfire Games
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

#include "lib/path.h"
#include "lib/os_path.h"

class TestPath : public CxxTest::TestSuite
{
public:
	void test_ctor()
	{
		const char* s1 = "a/b/c";
		const char* s2 = "a/b/\xEF\xBF\xBF";
		const wchar_t* w1 = L"a/b/c";
		const wchar_t w2[] = { 'a', '/', 'b', '/', 0xEF, 0xBF, 0xBF, 0 };
		const wchar_t w3[] = { 'a', '/', 'b', '/', 0xFFFF, 0 };

		// Empty strings
		Path p0a;
		Path p0b = Path(std::string());
		Path p0c = Path(std::wstring());
		TS_ASSERT(p0a.empty());
		TS_ASSERT_WSTR_EQUALS(p0a.string(), p0b.string());
		TS_ASSERT_WSTR_EQUALS(p0a.string(), p0c.string());

		// Construct from various types
		Path ps1a = Path(s1);
		Path ps2a = Path(s2);
		Path ps1b = Path(std::string(s1));
		Path ps2b = Path(std::string(s2));
		Path pw1a = Path(w1);
		Path pw2a = Path(w2);
		Path pw3a = Path(w3);
		Path pw1b = Path(std::wstring(w1));
		Path pw2b = Path(std::wstring(w2));
		Path pw3b = Path(std::wstring(w3));

		TS_ASSERT_WSTR_EQUALS(ps1a.string(), w1);
		TS_ASSERT_WSTR_EQUALS(ps1b.string(), w1);
		TS_ASSERT_WSTR_EQUALS(pw1a.string(), w1);
		TS_ASSERT_WSTR_EQUALS(pw1b.string(), w1);

		TS_ASSERT_WSTR_EQUALS(ps2a.string(), w2);
		TS_ASSERT_WSTR_EQUALS(ps2b.string(), w2);
		TS_ASSERT_WSTR_EQUALS(pw2a.string(), w2);
		TS_ASSERT_WSTR_EQUALS(pw2b.string(), w2);

		TS_ASSERT_WSTR_EQUALS(pw3a.string(), w3);
		TS_ASSERT_WSTR_EQUALS(pw3b.string(), w3);

#if OS_WIN
		TS_ASSERT_WSTR_EQUALS(OsString(pw2a), w2);
		TS_ASSERT_WSTR_EQUALS(OsString(pw3a), w3);
#else
		TS_ASSERT_STR_EQUALS(OsString(pw2a), s2);
		// OsString(pw3a) causes an intentional assertion failure, but we can't test that
#endif
	}
};
