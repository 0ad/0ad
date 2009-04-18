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
	// a legitimate va_list to pass to sys_vsnprintf:

	void _test_truncate(int buffer_size, const char* expected_output, int expected_return, /* char* input_string */...)
	{
		char buf[17] = "................"; // fill with dots so null-termination is made obvious

		va_list ap;
		va_start(ap, expected_return);

		int ret = sys_vsnprintf(buf, buffer_size, "%s", ap);

		TS_ASSERT_STR_EQUALS(buf, expected_output);
		TS_ASSERT_EQUALS(ret, expected_return);

		std::string past_buffer (buf + buffer_size);
		TS_ASSERT(past_buffer.find_first_not_of('.') == past_buffer.npos);

		va_end(ap);
	}

	void _test_sprintf(const char* expected_output, const char* format, ...)
	{
		char buf[256];

		va_list ap;
		va_start(ap, format);

		sys_vsnprintf(buf, sizeof(buf), format, ap);
		TS_ASSERT_STR_EQUALS(buf, expected_output);

		va_end(ap);
	}

public:
	void test_truncate()
	{
		_test_truncate(8, "1234",     4, "1234");
		_test_truncate(8, "1234567",  7, "1234567");
		_test_truncate(8, "1234567", -1, "12345678");
		_test_truncate(8, "1234567", -1, "123456789");
		_test_truncate(8, "1234567", -1, "123456789abcdef");
	}

	void test_lld()
	{
		i64 z = 0;
		i64 n = 65536;
		_test_sprintf("0", "%lld", z);
		_test_sprintf("65536", "%lld", n);
		_test_sprintf("4294967296", "%lld", n*n);
		_test_sprintf("281474976710656", "%lld", n*n*n);
		_test_sprintf("-281474976710656", "%lld", -n*n*n);
		_test_sprintf("123 456 281474976710656 789", "%d %d %lld %d", 123, 456, n*n*n, 789);
	}

	void test_pos()
	{
		_test_sprintf("a b", "%1$c %2$c", 'a', 'b');
		_test_sprintf("b a", "%2$c %1$c", 'a', 'b');
	}

	void test_pos_lld()
	{
		_test_sprintf("1 2 3", "%1$d %2$lld %3$d", 1, (i64)2, 3);
		_test_sprintf("2 1 3", "%2$lld %1$d %3$d", 1, (i64)2, 3);
	}
};
