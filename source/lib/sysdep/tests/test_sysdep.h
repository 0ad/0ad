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
#include "lib/posix/posix.h"	// fminf etc.

class TestSysdep : public CxxTest::TestSuite 
{
public:
	void test_float_int()
	{
		TS_ASSERT_EQUALS(cpu_i32FromFloat(0.99999f), 0);
		TS_ASSERT_EQUALS(cpu_i32FromFloat(1.0f), 1);
		TS_ASSERT_EQUALS(cpu_i32FromFloat(1.01f), 1);
		TS_ASSERT_EQUALS(cpu_i32FromFloat(5.6f), 5);

		TS_ASSERT_EQUALS(cpu_i32FromDouble(0.99999), 0);
		TS_ASSERT_EQUALS(cpu_i32FromDouble(1.0), 1);
		TS_ASSERT_EQUALS(cpu_i32FromDouble(1.01), 1);
		TS_ASSERT_EQUALS(cpu_i32FromDouble(5.6), 5);

		TS_ASSERT_EQUALS(cpu_i64FromDouble(0.99999), 0LL);
		TS_ASSERT_EQUALS(cpu_i64FromDouble(1.0), 1LL);
		TS_ASSERT_EQUALS(cpu_i64FromDouble(1.01), 1LL);
		TS_ASSERT_EQUALS(cpu_i64FromDouble(5.6), 5LL);
	}

	void test_round()
	{
		TS_ASSERT_EQUALS(rintf(0.99999f), 1.0f);
		TS_ASSERT_EQUALS(rintf(1.0f), 1.0f);
		TS_ASSERT_EQUALS(rintf(1.01f), 1.0f);
		TS_ASSERT_EQUALS(rintf(5.6f), 6.0f);

		TS_ASSERT_EQUALS(rint(0.99999), 1.0);
		TS_ASSERT_EQUALS(rint(1.0), 1.0);
		TS_ASSERT_EQUALS(rint(1.01), 1.0);
		TS_ASSERT_EQUALS(rint(5.6), 6.0);
	}

	void test_min_max()
	{
		TS_ASSERT_EQUALS(fminf(0.0f, 10000.0f), 0.0f);
		TS_ASSERT_EQUALS(fminf(100.0f, 10000.0f), 100.0f);
		TS_ASSERT_EQUALS(fminf(-1.0f, 2.0f), -1.0f);
		TS_ASSERT_EQUALS(fminf(-2.0f, 1.0f), -2.0f);
		TS_ASSERT_EQUALS(fminf(0.001f, 0.00001f), 0.00001f);

		TS_ASSERT_EQUALS(fmaxf(0.0f, 10000.0f), 10000.0f);
		TS_ASSERT_EQUALS(fmaxf(100.0f, 10000.0f), 10000.0f);
		TS_ASSERT_EQUALS(fmaxf(-1.0f, 2.0f), 2.0f);
		TS_ASSERT_EQUALS(fmaxf(-2.0f, 1.0f), 1.0f);
		TS_ASSERT_EQUALS(fmaxf(0.001f, 0.00001f), 0.001f);
	}
};
