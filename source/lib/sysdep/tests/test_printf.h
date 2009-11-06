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

#include "lib/sysdep/sysdep.h"

class TestPrintf : public CxxTest::TestSuite 
{
	// Split some bits into separate functions, so we can get
	// a legitimate va_list to pass to sys_vswprintf:

	void _test_truncate(int buffer_size, const wchar_t* expected_output, int expected_return, /* wchar_t* input_string */...)
	{
		wchar_t buf[17] = L"................"; // fill with dots so null-termination is made obvious

		va_list ap;
		va_start(ap, expected_return);

		int ret = sys_vswprintf(buf, buffer_size, L"%ls", ap);

		TS_ASSERT_WSTR_EQUALS(buf, expected_output);
		TS_ASSERT_EQUALS(ret, expected_return);

		std::wstring past_buffer(buf + buffer_size);
		TS_ASSERT(past_buffer.find_first_not_of('.') == past_buffer.npos);

		va_end(ap);
	}

	void _test_sprintf(const wchar_t* expected_output, const wchar_t* format, ...)
	{
		wchar_t buf[256];

		va_list ap;
		va_start(ap, format);

		sys_vswprintf(buf, ARRAY_SIZE(buf), format, ap);
		TS_ASSERT_WSTR_EQUALS(buf, expected_output);

		va_end(ap);
	}

public:
	void test_truncate()
	{
		_test_truncate(0, L"................", -1, L"1234");
		_test_truncate(1, L"",                 -1, L"1234");

		_test_truncate(8, L"1234",     4, L"1234");
		_test_truncate(8, L"1234567",  7, L"1234567");
		_test_truncate(8, L"1234567", -1, L"12345678");
		_test_truncate(8, L"1234567", -1, L"123456789");
		_test_truncate(8, L"1234567", -1, L"123456789abcdef");
	}

	void test_lld()
	{
		i64 z = 0;
		i64 n = 65536;
		_test_sprintf(L"0", L"%lld", z);
		_test_sprintf(L"65536", L"%lld", n);
		_test_sprintf(L"4294967296", L"%lld", n*n);
		_test_sprintf(L"281474976710656", L"%lld", n*n*n);
		_test_sprintf(L"-281474976710656", L"%lld", -n*n*n);
		_test_sprintf(L"123 456 281474976710656 789", L"%d %d %lld %d", 123, 456, n*n*n, 789);
	}

	void test_pos()
	{
		_test_sprintf(L"a b", L"%1$c %2$c", 'a', 'b');
		_test_sprintf(L"b a", L"%2$c %1$c", 'a', 'b');
	}

	void test_pos_lld()
	{
		_test_sprintf(L"1 2 3", L"%1$d %2$lld %3$d", 1, (i64)2, 3);
		_test_sprintf(L"2 1 3", L"%2$lld %1$d %3$d", 1, (i64)2, 3);
	}
};
