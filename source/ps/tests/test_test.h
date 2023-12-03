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

#include "ps/CStr.h"

class TestTest : public CxxTest::TestSuite
{
public:
	void test_assert_size_t()
	{
		const size_t a1 = std::numeric_limits<size_t>::max();
		const size_t b1 = std::numeric_limits<size_t>::max() - 1;
		const size_t c1 = std::numeric_limits<size_t>::min();
		size_t a2 = a1;
		size_t b2 = b1;
		size_t c2 = c1;

		TS_ASSERT_EQUALS(a2, a2);
		TS_ASSERT_DIFFERS(a2, b2);
		TS_ASSERT_DIFFERS(a2, c2);

		// These shouldn't cause warnings in CxxTest
		TS_ASSERT_EQUALS(a1, a1);
		TS_ASSERT_EQUALS(a1, a2);
		TS_ASSERT_EQUALS(a2, a1);

		// If TS_AS_STRING gives "{ 00 00 00 00  }", ValueTraits is failing
		// to handle these types properly
		TS_ASSERT_STR_EQUALS(TS_AS_STRING((size_t)0), "0");
		TS_ASSERT_STR_EQUALS(TS_AS_STRING((ssize_t)0), "0");
		TS_ASSERT_STR_EQUALS(TS_AS_STRING((unsigned int)0), "0");
	}

	void test_cstr()
	{
		TS_ASSERT_STR_EQUALS(TS_AS_STRING(CStr("test")), "\"test\"");
		TS_ASSERT_STR_EQUALS(TS_AS_STRING(std::string("test")), "\"test\"");

		TS_ASSERT_STR_EQUALS(TS_AS_STRING(CStrW(L"test")), "L\"test\"");
		TS_ASSERT_STR_EQUALS(TS_AS_STRING(std::wstring(L"test")), "L\"test\"");
	}
};
