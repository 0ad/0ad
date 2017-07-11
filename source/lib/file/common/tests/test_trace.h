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

#include "lib/file/common/trace.h"

class TestTraceEntry : public CxxTest::TestSuite
{
public:
	void test_entry()
	{
		std::wstring buf1;
		std::wstring buf2;

		TraceEntry t1(TraceEntry::Load, L"example.txt", 1234);
		TS_ASSERT_EQUALS(t1.Action(), TraceEntry::Load);
		TS_ASSERT_PATH_EQUALS(t1.Pathname(), OsPath(L"example.txt"));
		TS_ASSERT_EQUALS(t1.Size(), (size_t)1234);

		buf1 = t1.EncodeAsText();
		// The part before the ':' is an unpredictable timestamp,
		// so just test the string after that point
		TS_ASSERT_WSTR_EQUALS(wcschr(buf1.c_str(), ':'), L": L \"example.txt\" 1234\n");

		TraceEntry t2(TraceEntry::Store, L"example two.txt", 16777216);
		TS_ASSERT_EQUALS(t2.Action(), TraceEntry::Store);
		TS_ASSERT_PATH_EQUALS(t2.Pathname(), OsPath(L"example two.txt"));
		TS_ASSERT_EQUALS(t2.Size(), (size_t)16777216);

		buf2 = t2.EncodeAsText();
		TS_ASSERT_WSTR_EQUALS(wcschr(buf2.c_str(), ':'), L": S \"example two.txt\" 16777216\n");

		TraceEntry t3(buf1);
		TS_ASSERT_EQUALS(t3.Action(), TraceEntry::Load);
		TS_ASSERT_PATH_EQUALS(t3.Pathname(), OsPath(L"example.txt"));
		TS_ASSERT_EQUALS(t3.Size(), (size_t)1234);

		TraceEntry t4(buf2);
		TS_ASSERT_EQUALS(t4.Action(), TraceEntry::Store);
		TS_ASSERT_PATH_EQUALS(t4.Pathname(), OsPath(L"example two.txt"));
		TS_ASSERT_EQUALS(t4.Size(), (size_t)16777216);
	}

	void test_maxpath()
	{
		OsPath path1(std::wstring(PATH_MAX, L'x'));
		std::wstring buf1 = L"0: L \"" + path1.string() + L"\" 0\n";
		TraceEntry t1(buf1);
		TS_ASSERT_PATH_EQUALS(t1.Pathname(), path1);
	}
};
