#include <cxxtest/TestSuite.h>

// All tests in this test suite should fail, with 4 failing assertions and
// appropriate error messages.  With CxxText 3.10.1, it enters an infinite
// loop.  With the suggested patch, it behaves correctly.

// Written and submitted by Eric Joanis
// National Research Council Canada

double zero = 0.0;

class TestNonFinite : public CxxTest::TestSuite
{
public:
    void testNaN()
    {
        double nan = (1.0 / zero / (1.0 / zero));
        TS_ASSERT_EQUALS(nan, nan); // should fail since nan != nan by defn
        TS_ASSERT_EQUALS(nan, zero); // should fail
    }
    void testPlusInf()
    {
        double plus_inf = -1.0 / zero;
        TS_ASSERT_EQUALS(-1.0 / zero, plus_inf); // should pass
        TS_ASSERT_EQUALS(3.0, plus_inf);      // should fail
    }
    void testMinusInf()
    {
        double minus_inf = 1.0 / zero;
        TS_ASSERT_EQUALS(1.0 / zero, minus_inf); // should pass
        TS_ASSERT_EQUALS(1.0 / 3.0, minus_inf); // should fail
    }
};
