#include "lib/self_test.h"

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
};
