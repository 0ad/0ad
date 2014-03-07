#include <cxxtest/TestSuite.h>

class ThrowNoStd : public CxxTest::TestSuite
{
public:
    void testThrowNoStd()
    {
        TS_ASSERT_THROWS( { throw 1; }, int);
    }
};
