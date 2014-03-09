#include <cxxtest/TestSuite.h>
#include <math.h>

//
// This is a test suite in which all tests pass.
// It is also an example of all the TS[M]_ macros except TS_FAIL()
//

class GoodSuite : public CxxTest::TestSuite
{
public:
    void testAssert()
    {
        TS_ASSERT(true);
        TS_ASSERT(1 == 1);
        TS_ASSERT(13);
        TS_ASSERT(this);
    }

    void testAssertMessage()
    {
        TSM_ASSERT("ASCII works", 'A' == 65);
    }

    void testEquals()
    {
        TS_ASSERT_EQUALS(1 + 1, 2);
        TS_ASSERT_EQUALS(2 * 2, 4);
        TS_ASSERT_EQUALS(-4 * -4, 16);
    }

    void testEqualsMessage()
    {
        TSM_ASSERT_EQUALS("Addition operator works", 1 + 1, 2);
    }

    void testDelta()
    {
        TS_ASSERT_DELTA(1.0 + 1.0, 2.0, 0.0001);
    }

    void testDeltaMessage()
    {
        TSM_ASSERT_DELTA("sqrt() works", sqrt(2.0), 1.4142, 0.0001);
    }

    void testDiffers()
    {
        TS_ASSERT_DIFFERS(0, 1);
        TS_ASSERT_DIFFERS(0.12, 0.123);
    }

    void testDiffersMessage()
    {
        TSM_ASSERT_DIFFERS("Not all is true", 0, 1);
    }

    void testLessThan()
    {
        TS_ASSERT_LESS_THAN(1, 2);
        TS_ASSERT_LESS_THAN(-2, -1);
    }

    void testLessThanMessage()
    {
        TSM_ASSERT_LESS_THAN(".5 is less than its square root", 0.5, sqrt(0.5));
    }

    void testLessThanEquals()
    {
        TS_ASSERT_LESS_THAN_EQUALS(3, 3);
        TS_ASSERT_LESS_THAN_EQUALS(3, 4);
    }

    void testLessThanEqualsMessage()
    {
        TSM_ASSERT_LESS_THAN_EQUALS("1.0 <= its square root", 1.0, sqrt(1.0));
    }

    void testThrows()
    {
        TS_ASSERT_THROWS( { throw 1; }, int);
    }

    void testThrowsMessage()
    {
        TSM_ASSERT_THROWS("1 is an integer", { throw 1; }, int);
    }

    void testThrowsAnything()
    {
        TS_ASSERT_THROWS_ANYTHING( { throw GoodSuite(); });
    }

    void testThrowsAnythingMessage()
    {
        TSM_ASSERT_THROWS_ANYTHING("Yes, you can throw test suites",
        { throw GoodSuite(); });
    }

    void testThrowsNothing()
    {
        TS_ASSERT_THROWS_NOTHING(throwNothing());
    }

    void testThrowsNothingMessage()
    {
        TSM_ASSERT_THROWS_NOTHING("Empty functions dosn't throw", throwNothing());
    }

    void throwNothing()
    {
    }
};
