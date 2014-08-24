#include <cxxtest/TestSuite.h>

class DynamicMax : public CxxTest::TestSuite
{
public:
    enum { DATA_SIZE = 24 };
    unsigned char x[DATA_SIZE], y[DATA_SIZE];

    void setUp()
    {
        for (unsigned i = 0; i < DATA_SIZE; ++ i)
        {
            x[i] = (unsigned char)i;
            y[i] = (unsigned char)~x[i];
        }
    }

    void test_Max_size_from_define()
    {
        TS_ASSERT_SAME_DATA(x, y, DATA_SIZE);
    }

    void test_Set_max_size()
    {
        CxxTest::setMaxDumpSize(16);
        TS_ASSERT_SAME_DATA(x, y, DATA_SIZE);
    }

    void test_Revert_to_max_size_from_define()
    {
        TS_ASSERT_SAME_DATA(x, y, DATA_SIZE);
    }

    void test_Set_max_size_to_zero__dumps_all()
    {
        CxxTest::setMaxDumpSize(0);
        TS_ASSERT_SAME_DATA(x, y, DATA_SIZE);
    }
};

class SetUpAffectsAllTests : public CxxTest::TestSuite
{
public:
    enum { DATA_SIZE = 24 };
    unsigned char x[DATA_SIZE], y[DATA_SIZE];

    void setUp()
    {
        for (unsigned i = 0; i < DATA_SIZE; ++ i)
        {
            x[i] = (unsigned char)i;
            y[i] = (unsigned char)~x[i];
        }

        CxxTest::setMaxDumpSize(12);
    }

    void test_Use_12_in_this_test()
    {
        TS_ASSERT_SAME_DATA(x, y, DATA_SIZE);
    }

    void test_Use_12_in_this_test_too()
    {
        TS_ASSERT_SAME_DATA(x, y, DATA_SIZE);
    }
};
