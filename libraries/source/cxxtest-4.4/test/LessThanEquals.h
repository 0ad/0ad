#include <cxxtest/TestSuite.h>

//
// This test suites demonstrated TS_LESS_THAN_EQUALS
// and how it fails.
//

class LessThanEquals : public CxxTest::TestSuite
{
public:
    void testLessThanEquals()
    {
        TS_ASSERT_LESS_THAN_EQUALS(1, 2);
        TS_ASSERT_LESS_THAN_EQUALS(1, 1);

        TS_ASSERT_LESS_THAN_EQUALS(1, 0);
        TSM_ASSERT_LESS_THAN_EQUALS("1 <=? 0", 1, 0);

        ETS_ASSERT_LESS_THAN(1, 0);
        ETSM_ASSERT_LESS_THAN_EQUALS("1 <=? 0", 1, 0);
    }
};
