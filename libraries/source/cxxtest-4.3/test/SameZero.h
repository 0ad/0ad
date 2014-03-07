#include <cxxtest/TestSuite.h>

//
// This is a test of TS_ASSERT_SAME_DATA when passed NULL
//

class SameZero : public CxxTest::TestSuite
{
public:
    char data[4];

    void setUp()
    {
        for (unsigned i = 0; i < sizeof(data); ++ i)
        {
            data[i] = (char)i;
        }
    }

    void test_TS_ASSERT_SAME_DATA_passed_zero()
    {
        TS_ASSERT_SAME_DATA(data, 0, sizeof(data));
        TS_ASSERT_SAME_DATA(0, data, sizeof(data));
        TS_ASSERT_SAME_DATA(data, 0, 0);
        TS_ASSERT_SAME_DATA(0, data, 0);
        TS_ASSERT_SAME_DATA(0, 0, 0);
    }
};
