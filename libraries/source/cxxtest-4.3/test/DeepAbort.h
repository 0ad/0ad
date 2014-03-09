#include <cxxtest/TestSuite.h>

//
// This test suite verifies that the TS_ASSERT_THROWS*() macros are "abort on fail"-friendly
//

class DeepAbort : public CxxTest::TestSuite
{
public:
    void testAssertThrowsPassesAbort()
    {
        TS_ASSERT_THROWS(fail(), int);
        TS_FAIL("You shouldn't see this if --abort-on-fail is used");
    }

    void testMessageAssertThrowsPassesAbort()
    {
        TSM_ASSERT_THROWS("fail() should throw an int", fail(), int);
        TS_FAIL("You shouldn't see this if --abort-on-fail is used");
    }

    void testAssertThrowsAborts()
    {
        TS_ASSERT_THROWS(succeed(), int);
        TS_FAIL("You shouldn't see this if --abort-on-fail is used");
    }

    void testMessageAssertThrowsAborts()
    {
        TSM_ASSERT_THROWS("succeed() should throw an int", succeed(), int);
        TS_FAIL("You shouldn't see this if --abort-on-fail is used");
    }

    void testAssertThrowsNothingPassesAbort()
    {
        TS_ASSERT_THROWS_NOTHING(fail());
        TS_FAIL("You shouldn't see this if --abort-on-fail is used");
    }

    void testMessageAssertThrowsNothingPassesAbort()
    {
        TSM_ASSERT_THROWS_NOTHING("fail() shouldn't throw anything", fail());
        TS_FAIL("You shouldn't see this if --abort-on-fail is used");
    }

    void testAssertThrowsNothingAborts()
    {
        TS_ASSERT_THROWS_NOTHING(throwSomething());
        TS_FAIL("You shouldn't see this if --abort-on-fail is used");
    }

    void testMessageAssertThrowsNothingAborts()
    {
        TSM_ASSERT_THROWS_NOTHING("fail() shouldn't throw anything", throwSomething());
        TS_FAIL("You shouldn't see this if --abort-on-fail is used");
    }

    void testAssertThrowsAnything()
    {
        TS_ASSERT_THROWS_ANYTHING(succeed());
        TS_FAIL("You shouldn't see this if --abort-on-fail is used");
    }

    void testMessageAssertThrowsAnything()
    {
        TSM_ASSERT_THROWS_ANYTHING("succeed() should throw something", succeed());
        TS_FAIL("You shouldn't see this if --abort-on-fail is used");
    }

    void fail()
    {
        TS_ASSERT_EQUALS(0, 1);
    }

    void throwSomething()
    {
        throw "something";
    }

    void succeed()
    {
        TS_ASSERT_EQUALS(1, 1);
    }
};
