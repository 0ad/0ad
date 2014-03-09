#include <cxxtest/TestSuite.h>

//
// This tests CxxTest's handling of "__int64"
//

class Int64 : public CxxTest::TestSuite
{
public:
    void testInt64()
    {
        TS_ASSERT_EQUALS((__int64)1, (__int64)2);
        TS_ASSERT_DIFFERS((__int64)3, (__int64)3);
        TS_ASSERT_LESS_THAN((__int64)5, (__int64)4);
    }
};
