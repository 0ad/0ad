#include <cxxtest/TestSuite.h>

class NoEh : public CxxTest::TestSuite
{
public:
    void testCxxTestCanCompileWithoutExceptionHandling()
    {
        TS_ASSERT_EQUALS(1, 2);
        TS_ASSERT_EQUALS(2, 3);
    }
};
