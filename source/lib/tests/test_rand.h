#include "lib/self_test.h"

#include "lib/rand.h"

class TestRand : public CxxTest::TestSuite 
{
public:
	// complain if huge interval or min > max
	void TestParam()
	{
		debug_skip_next_err(ERR::INVALID_PARAM);
		TS_ASSERT_EQUALS(rand(1, 0), size_t(0));
		debug_skip_next_err(ERR::INVALID_PARAM);
		TS_ASSERT_EQUALS(rand(2, ~0u), size_t(0));
	}

	// returned number must be in [min, max)
	void TestReturnedRange()
	{
		for(int i = 0; i < 100; i++)
		{
			size_t min = rand(), max = min+rand();
			size_t x = rand(min, max);
			TS_ASSERT(min <= x && x < max);
		}
	}

	// make sure both possible values are hit
	void TestTwoValues()
	{
		size_t ones = 0, twos = 0;
		for(int i = 0; i < 100; i++)
		{
			size_t x = rand(1, 3);
			// paranoia: don't use array (x might not be 1 or 2 - checked below)
			if(x == 1) ones++;
			if(x == 2) twos++;
		}
		TS_ASSERT_EQUALS(ones+twos, 100);
		TS_ASSERT(ones > 10 && twos > 10);
	}
};
