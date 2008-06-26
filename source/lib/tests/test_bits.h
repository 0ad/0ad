#include "lib/self_test.h"

#include "lib/bits.h"

class TestBits : public CxxTest::TestSuite 
{
public:
	void test_is_pow2()
	{
		TS_ASSERT_EQUALS(is_pow2(0u), false);
		TS_ASSERT_EQUALS(is_pow2(~0u), false);
		TS_ASSERT_EQUALS(is_pow2(0x80000001), false);
		TS_ASSERT_EQUALS(is_pow2(1), true);
		TS_ASSERT_EQUALS(is_pow2(1u << 31), true);
	}

	void test_ceil_log2()
	{
		TS_ASSERT_EQUALS(ceil_log2(3u), 2u);
		TS_ASSERT_EQUALS(ceil_log2(0xffffffffu), 32u);
		TS_ASSERT_EQUALS(ceil_log2(1u), 0u);
		TS_ASSERT_EQUALS(ceil_log2(256u), 8u);
		TS_ASSERT_EQUALS(ceil_log2(0x80000000u), 31u);
	}

	void test_floor_log2()
	{
		TS_ASSERT_EQUALS(floor_log2(1.f), 0);
		TS_ASSERT_EQUALS(floor_log2(3.f), 1);
		TS_ASSERT_EQUALS(floor_log2(256.f), 8);
	}

	void test_round_up_to_pow2()
	{
		TS_ASSERT_EQUALS(round_up_to_pow2(0u), 1u);
		TS_ASSERT_EQUALS(round_up_to_pow2(1u), 1u);
		TS_ASSERT_EQUALS(round_up_to_pow2(127u), 128u);
		TS_ASSERT_EQUALS(round_up_to_pow2(128u), 128u);
		TS_ASSERT_EQUALS(round_up_to_pow2(129u), 256u);
	}

	void test_round_up()
	{
		TS_ASSERT_EQUALS(round_up( 0u, 16u), 0u);
		TS_ASSERT_EQUALS(round_up( 4u, 16u), 16u);
		TS_ASSERT_EQUALS(round_up(15u, 16u), 16u);
		TS_ASSERT_EQUALS(round_up(20u, 32u), 32u);
		TS_ASSERT_EQUALS(round_up(29u, 32u), 32u);
		TS_ASSERT_EQUALS(round_up(0x1000u, 0x1000u), 0x1000u);
		TS_ASSERT_EQUALS(round_up(0x1001u, 0x1000u), 0x2000u);
		TS_ASSERT_EQUALS(round_up(0x1900u, 0x1000u), 0x2000u);
	}

	void test_round_down()
	{
		TS_ASSERT_EQUALS(round_down( 0u, 16u), 0u);
		TS_ASSERT_EQUALS(round_down( 4u, 16u), 0u);
		TS_ASSERT_EQUALS(round_down(15u, 16u), 0u);
		TS_ASSERT_EQUALS(round_down(20u, 16u), 16u);
		TS_ASSERT_EQUALS(round_down(29u, 16u), 16u);
		TS_ASSERT_EQUALS(round_down(0x1900u, 0x1000u), 0x1000u);
		TS_ASSERT_EQUALS(round_down(0x2001u, 0x2000u), 0x2000u);
	}
};
