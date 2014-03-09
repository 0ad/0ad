//
// This file tests CxxTest global fixtures
//

#include <cxxtest/TestSuite.h>
#include <cxxtest/GlobalFixture.h>

//
// Fixture1 counts its setUp()s and tearDown()s
//
class Fixture1 : public CxxTest::GlobalFixture
{
    unsigned _setUpCount;
    unsigned _tearDownCount;

public:
    Fixture1() { _setUpCount = _tearDownCount = 0; }
    bool setUp() { ++ _setUpCount; return true; }
    bool tearDown() { ++ _tearDownCount; return true; }
    unsigned setUpCount() const { return _setUpCount; }
    unsigned tearDownCount() const { return _tearDownCount; }
};

//
// We can rely on this file being included exactly once
// and declare this global variable in the header file.
//
static Fixture1 fixture1;

//
// Fixture2 counts its setUp()s and tearDown()s and makes sure
// its setUp() is called after Fixture1 and its tearDown() before.
//
class Fixture2 : public Fixture1
{
public:
    bool setUp()
    {
        TS_ASSERT_EQUALS(setUpCount(), fixture1.setUpCount() - 1);
        TS_ASSERT_EQUALS(tearDownCount(), fixture1.tearDownCount());
        return Fixture1::setUp();
    }

    bool tearDown()
    {
        TS_ASSERT_EQUALS(setUpCount(), fixture1.setUpCount());
        TS_ASSERT_EQUALS(tearDownCount(), fixture1.tearDownCount());
        return Fixture1::tearDown();
    }
};

static Fixture2 fixture2;

class TestGlobalFixture : public CxxTest::TestSuite
{
public:
    void testCountsFirstTime()
    {
        TS_ASSERT_EQUALS(fixture1.setUpCount(), 1);
        TS_ASSERT_EQUALS(fixture1.tearDownCount(), 0);
        TS_ASSERT_EQUALS(fixture2.setUpCount(), 1);
        TS_ASSERT_EQUALS(fixture2.tearDownCount(), 0);
    }

    void testCountsSecondTime()
    {
        TS_ASSERT_EQUALS(fixture1.setUpCount(), 2);
        TS_ASSERT_EQUALS(fixture1.tearDownCount(), 1);
        TS_ASSERT_EQUALS(fixture2.setUpCount(), 2);
        TS_ASSERT_EQUALS(fixture2.tearDownCount(), 1);
    }
};
