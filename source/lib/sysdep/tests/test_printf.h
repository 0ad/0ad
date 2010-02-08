/* Copyright (c) 2010 Wildfire Games
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
