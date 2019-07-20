/* Copyright (C) 2015 Wildfire Games.
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
#include "lib/scoped_locale.h"

#include "third_party/cppformat/format.h"

class TestCppformat : public CxxTest::TestSuite
{
public:
	void test_basic()
	{
		// Make test behave independent of current host locale
		ScopedLocale(LC_ALL, "en_US.UTF-8");

		TS_ASSERT_EQUALS(fmt::sprintf("abc"), "abc");

		TS_ASSERT_EQUALS(fmt::sprintf("%d", 123), "123");
		TS_ASSERT_EQUALS(fmt::sprintf("%d", (int64_t)123), "123");
		TS_ASSERT_EQUALS(fmt::sprintf("%i", 123), "123");

		TS_ASSERT_EQUALS(fmt::sprintf("%04x", 123), "007b");
		TS_ASSERT_EQUALS(fmt::sprintf("%04X", 123), "007B");

		TS_ASSERT_EQUALS(fmt::sprintf("%f", 0.5f), "0.500000");
		TS_ASSERT_EQUALS(fmt::sprintf("%.1f", 0.1111f), "0.1");

		TS_ASSERT_EQUALS(fmt::sprintf("%d", 0x100000001ULL), "1");
		if (sizeof(long) == sizeof(int32_t))
		{
			TS_ASSERT_EQUALS(fmt::sprintf("%ld", 0x100000001ULL), "1");
		}
		else
		{
			TS_ASSERT_EQUALS(fmt::sprintf("%ld", 0x100000001ULL), "4294967297");
		}
		TS_ASSERT_EQUALS(fmt::sprintf("%lld", 0x100000001ULL), "4294967297");

		TS_ASSERT_EQUALS(fmt::sprintf("T%sT", "abc"), "TabcT");
		TS_ASSERT_EQUALS(fmt::sprintf("T%sT", std::string("abc")), "TabcT");
		TS_ASSERT_EQUALS(fmt::sprintf("T%sT", CStr("abc")), "TabcT");

		TS_ASSERT_EQUALS(fmt::sprintf("T%sT", (const char*)NULL), "T(null)T");

		TS_ASSERT_EQUALS(fmt::sprintf("T%pT", (void*)0x1234), "T0x1234T");
	}
};
