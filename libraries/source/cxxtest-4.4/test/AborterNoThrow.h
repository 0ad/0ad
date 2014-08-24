#include <cxxtest/TestSuite.h>

//
// This is a test suite which doesn't use exception handling.
// It is used to verify --abort-on-fail + --have-eh
//

class AborterNoThrow : public CxxTest::TestSuite
{
public:
    void testFailures()
    {
        TS_FAIL(1);
        TS_FAIL(2);
        TS_FAIL(3);
        TS_FAIL(4);
        TS_FAIL(5);
    }
};
