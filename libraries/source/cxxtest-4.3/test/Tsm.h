#include <cxxtest/TestSuite.h>

//
// This is a test of some of the TSM_ macros
//

class TestMessageMacros : public CxxTest::TestSuite
{
public:
    void testMessageMacros()
    {
        int n = 42;
        char x = 'x', y = 'y';

        TSM_ASSERT("String", false);
        TSM_ASSERT(n, false);
        TSM_ASSERT_EQUALS("String", 2 + 2, 5);
        TSM_ASSERT_EQUALS(n, 2 + 2, 5);
        TSM_ASSERT_SAME_DATA("String", &x, &y, 1);
        TSM_ASSERT_SAME_DATA(n, &x, &y, 1);
        TSM_ASSERT_DELTA("String", 1.0, 2.0, 0.5);
        TSM_ASSERT_DELTA(42, 1.0, 2.0, 0.5);
        TSM_ASSERT_DIFFERS("String", 0, 0);
        TSM_ASSERT_DIFFERS(n, 0, 0);
        TSM_ASSERT_LESS_THAN("String", 2, 1);
        TSM_ASSERT_LESS_THAN(n, 2, 1);
        TSM_ASSERT_THROWS("String", throwNothing(), int);
        TSM_ASSERT_THROWS(n, throwNothing(), int);
        TSM_ASSERT_THROWS_ANYTHING("String", throwNothing());
        TSM_ASSERT_THROWS_ANYTHING(n, throwNothing());
        TSM_ASSERT_THROWS_NOTHING("String", throwInteger(n));
        TSM_ASSERT_THROWS_NOTHING(n, throwInteger(n));
        TSM_ASSERT_THROWS_ASSERT("String", throwNothing(), int, TS_ASSERT(true));
        TSM_ASSERT_THROWS_ASSERT(n, throwNothing(), int, TS_ASSERT(true));
        TSM_ASSERT_THROWS_EQUALS("String", throwNothing(), int, 1, 1);
        TSM_ASSERT_THROWS_EQUALS(n, throwNothing(), int, 1, 1);
        TSM_ASSERT_THROWS_EQUALS("String", throwInteger(n), int i, i, 43);
        TSM_ASSERT_THROWS_EQUALS(n, throwInteger(n), int i, i, 43);
    }

    void throwNothing()
    {
    }

    void throwInteger(int i)
    {
        throw i;
    }
};

#ifndef _CXXTEST_HAVE_EH
#   error cxxtestgen should have found exception handling here!
#endif // !_CXXTEST_HAVE_EH
