#include <cxxtest/TestSuite.h>

class ForceNoEh : public CxxTest::TestSuite
{
public:
    void testCxxTestCanCompileWithoutExceptionHandling()
    {
        TS_ASSERT_EQUALS(1, 2);
        TS_ASSERT_EQUALS(2, 3);
        TS_ASSERT_THROWS_NOTHING(foo());
    }

    void foo()
    {
    }
};
