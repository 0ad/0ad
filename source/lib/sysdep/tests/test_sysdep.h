#include <cxxtest/TestSuite.h>

#include "lib/sysdep/sysdep.h"
#include "lib/posix.h"	// fminf etc.

class TestSysdep : public CxxTest::TestSuite 
{
public:
	void test_float_int()
	{
		TS_ASSERT_EQUAL(i32_from_float(0.99999f), 0);
		TS_ASSERT_EQUAL(i32_from_float(1.0f), 1);
		TS_ASSERT_EQUAL(i32_from_float(1.01f), 1);
		TS_ASSERT_EQUAL(i32_from_float(5.6f), 5);

		TS_ASSERT_EQUAL(i32_from_double(0.99999), 0);
		TS_ASSERT_EQUAL(i32_from_double(1.0), 1);
		TS_ASSERT_EQUAL(i32_from_double(1.01), 1);
		TS_ASSERT_EQUAL(i32_from_double(5.6), 5);

		TS_ASSERT_EQUAL(i64_from_double(0.99999), 0LL);
		TS_ASSERT_EQUAL(i64_from_double(1.0), 1LL);
		TS_ASSERT_EQUAL(i64_from_double(1.01), 1LL);
		TS_ASSERT_EQUAL(i64_from_double(5.6), 5LL);
	}

	void test_round()
	{
		TS_ASSERT_EQUAL(rintf(0.99999f), 1.0f);
		TS_ASSERT_EQUAL(rintf(1.0f), 1.0f);
		TS_ASSERT_EQUAL(rintf(1.01f), 1.0f);
		TS_ASSERT_EQUAL(rintf(5.6f), 5.0f);

		TS_ASSERT_EQUAL(rint(0.99999), 1.0);
		TS_ASSERT_EQUAL(rint(1.0), 1.0);
		TS_ASSERT_EQUAL(rint(1.01), 1.0);
		TS_ASSERT_EQUAL(rint(5.6), 5.0);
	}

	void test_min_max()
	{
		TS_ASSERT_EQUAL(fminf(0.0f, 10000.0f), 0.0f);
		TS_ASSERT_EQUAL(fminf(100.0f, 10000.0f), 100.0f);
		TS_ASSERT_EQUAL(fminf(-1.0f, 2.0f), -1.0f);
		TS_ASSERT_EQUAL(fminf(-2.0f, 1.0f), -2.0f);
		TS_ASSERT_EQUAL(fminf(0.001f, 0.00001f), 0.00001f);

		TS_ASSERT_EQUAL(fmaxf(0.0f, 10000.0f), 10000.0f);
		TS_ASSERT_EQUAL(fmaxf(100.0f, 10000.0f), 10000.0f);
		TS_ASSERT_EQUAL(fmaxf(-1.0f, 2.0f), 2.0f);
		TS_ASSERT_EQUAL(fmaxf(-2.0f, 1.0f), 1.0f);
		TS_ASSERT_EQUAL(fmaxf(0.001f, 0.00001f), 0.001f);
	}
};
