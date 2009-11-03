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
		TS_ASSERT_WSTR_EQUALS(t1.Pathname().string(), L"example.txt");
		TS_ASSERT_EQUALS(t1.Size(), (size_t)1234);

		buf1 = t1.EncodeAsText();
		// The part before the ':' is an unpredictable timestamp,
		// so just test the string after that point
		TS_ASSERT_WSTR_EQUALS(wcschr(buf1.c_str(), ':'), L": L \"example.txt\" 1234\n");

		TraceEntry t2(TraceEntry::Store, L"example two.txt", 16777216);
		TS_ASSERT_EQUALS(t2.Action(), TraceEntry::Store);
		TS_ASSERT_WSTR_EQUALS(t2.Pathname().string(), L"example two.txt");
		TS_ASSERT_EQUALS(t2.Size(), (size_t)16777216);

		buf2 = t2.EncodeAsText();
		TS_ASSERT_WSTR_EQUALS(wcschr(buf2.c_str(), ':'), L": S \"example two.txt\" 16777216\n");

		TraceEntry t3(buf1);
		TS_ASSERT_EQUALS(t3.Action(), TraceEntry::Load);
		TS_ASSERT_WSTR_EQUALS(t3.Pathname().string(), L"example.txt");
		TS_ASSERT_EQUALS(t3.Size(), (size_t)1234);

		TraceEntry t4(buf2);
		TS_ASSERT_EQUALS(t4.Action(), TraceEntry::Store);
		TS_ASSERT_WSTR_EQUALS(t4.Pathname().string(), L"example two.txt");
		TS_ASSERT_EQUALS(t4.Size(), (size_t)16777216);
	}
};
