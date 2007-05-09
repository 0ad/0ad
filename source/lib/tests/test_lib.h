#include "lib/self_test.h"

#include "lib/lib.h"

class TestLib : public CxxTest::TestSuite 
{
public:
	// 16-bit saturating arithmetic
	void test_addusw()
	{
		TS_ASSERT_EQUALS(addusw(4u, 0x100u), 0x0104u);
		TS_ASSERT_EQUALS(addusw(0u, 0xFFFFu), 0xFFFFu);
		TS_ASSERT_EQUALS(addusw(0x8000u, 0x8000u), 0xFFFFu);
		TS_ASSERT_EQUALS(addusw(0xFFF0u, 0x0004u), 0xFFF4u);
		TS_ASSERT_EQUALS(addusw(0xFFFFu, 0xFFFFu), 0xFFFFu);
	}

	void test_subusw()
	{
		TS_ASSERT_EQUALS(subusw(4u, 0x100u), 0u);
		TS_ASSERT_EQUALS(subusw(100u, 90u), 10u);
		TS_ASSERT_EQUALS(subusw(0x8000u, 0x8000u), 0u);
		TS_ASSERT_EQUALS(subusw(0x0FFFu, 0xFFFFu), 0u);
		TS_ASSERT_EQUALS(subusw(0xFFFFu, 0x0FFFu), 0xF000u);
	}

	void test_hi_lo()
	{
		TS_ASSERT_EQUALS(u64_hi(0x0123456789ABCDEFull), 0x01234567u);
		TS_ASSERT_EQUALS(u64_hi(0x0000000100000002ull), 0x00000001u);

		TS_ASSERT_EQUALS(u64_lo(0x0123456789ABCDEFull), 0x89ABCDEFu);
		TS_ASSERT_EQUALS(u64_lo(0x0000000100000002ull), 0x00000002u);

		TS_ASSERT_EQUALS(u32_hi(0x01234567u), 0x0123u);
		TS_ASSERT_EQUALS(u32_hi(0x00000001u), 0x0000u);

		TS_ASSERT_EQUALS(u32_lo(0x01234567u), 0x4567u);
		TS_ASSERT_EQUALS(u32_lo(0x00000001u), 0x0001u);

		TS_ASSERT_EQUALS(u64_from_u32(0xFFFFFFFFu, 0x80000008u), 0xFFFFFFFF80000008ull);
		TS_ASSERT_EQUALS(u32_from_u16(0x8000u, 0xFFFFu), 0x8000FFFFu);
	}

	// fp_to_u?? already validate the result.

	void test_rand()
	{
		// complain if huge interval or min > max
		debug_skip_next_err(ERR::INVALID_PARAM);
		TS_ASSERT_EQUALS(rand(1, 0), 0);
		debug_skip_next_err(ERR::INVALID_PARAM);
		TS_ASSERT_EQUALS(rand(2, ~0u), 0);

		// returned number must be in [min, max)
		for(int i = 0; i < 100; i++)
		{
			uint min = rand(), max = min+rand();
			uint x = rand(min, max);
			TS_ASSERT(min <= x && x < max);
		}

		// make sure both possible values are hit
		uint ones = 0, twos = 0;
		for(int i = 0; i < 100; i++)
		{
			uint x = rand(1, 3);
			// paranoia: don't use array (x might not be 1 or 2 - checked below)
			if(x == 1) ones++;
			if(x == 2) twos++;
		}
		TS_ASSERT_EQUALS(ones+twos, 100);
		TS_ASSERT(ones > 10 && twos > 10);
	}
};
