#include <cxxtest/TestSuite.h>

//
// These test suites are examples of unhandled exceptions and errors in dynamic suites
//

class NullCreate : public CxxTest::TestSuite
{
public:
    static NullCreate *createSuite() { return 0; }
    static void destroySuite(NullCreate *) { TS_FAIL("Should not be called"); }

    void testNothing()
    {
        TS_FAIL("Test called although no suite");
    }
};

class ThrowCreate : public CxxTest::TestSuite
{
public:
    static ThrowCreate *createSuite() { throw - 3; }
    static void destroySuite(ThrowCreate *) { TS_FAIL("Should not be called"); }

    void testNothing()
    {
        TS_FAIL("Test called although no suite");
    }
};

class ThrowDestroy : public CxxTest::TestSuite
{
public:
    static ThrowDestroy *createSuite() { return new ThrowDestroy; }
    static void destroySuite(ThrowDestroy *suite) { delete suite; throw 42; }

    void testNothing() {}
};

class ThrowSetUp : public CxxTest::TestSuite
{
public:
    void setUp() { throw 5; }
    void tearDown() { TS_FAIL("Shouldn't get here"); }

    void testNothing() { TS_FAIL("Shouldn't get here"); }
};

class ThrowTearDown : public CxxTest::TestSuite
{
public:
    void setUp() {}
    void tearDown() { throw 5; }

    void testNothing() {}
};

class TestThrowFromTest : public CxxTest::TestSuite
{
public:
    void testThrowSomething()
    {
        throw 582;
    }

    void testMoveOn()
    {
        TS_TRACE("One failed test doesn't affect the others");
    }
};
