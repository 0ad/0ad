#include <cxxtest/TestSuite.h>

//
// This test suite is used to test the root/part functionality of CxxTest.
//

class Part2 : public CxxTest::TestSuite
{
public:
    void testSomething()
    {
        TS_ASSERT_THROWS_NOTHING(throwNothing());
    }

    void throwNothing()
    {
    }
};
