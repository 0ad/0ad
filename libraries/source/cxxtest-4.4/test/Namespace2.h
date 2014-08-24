#ifndef __NAMESPACE2_H
#define __NAMESPACE2_H

#include <cxxtest/TestSuite.h>

//
// A simple test suite: Just inherit CxxTest::TestSuite and write tests!
//

// Declare Tests in the appropriate namespace
namespace foo { namespace bar { class Tests; } }

// Use explicit namespace declaration
class foo::bar::Tests : public CxxTest::TestSuite {
public:
    void testEquality() {
        TS_ASSERT_EQUALS(1, 1);
        TS_ASSERT_EQUALS(1, 2);
        TS_ASSERT_EQUALS('a', 'A');
        TS_ASSERT_EQUALS(1.0, -12345678900000000000000000000000000000000000000000.1234);
    }

    void testAddition() {
        TS_ASSERT_EQUALS(1 + 1, 2);
        TS_ASSERT_EQUALS(2 + 2, 5);
    }

    void TestMultiplication() {
        TS_ASSERT_EQUALS(2 * 2, 4);
        TS_ASSERT_EQUALS(4 * 4, 44);
        TS_ASSERT_DIFFERS(-2 * -2, 4);
    }

    void testComparison() {
        TS_ASSERT_LESS_THAN((int)1, (unsigned long)2);
        TS_ASSERT_LESS_THAN(-1, -2);
    }

    void testTheWorldIsCrazy() {
        TS_ASSERT_EQUALS(true, false);
    }

    void test_Failure() {
        TS_FAIL("Not implemented");
        TS_FAIL(1569779912);
    }

    void test_TS_WARN_macro() {
        TS_WARN("Just a friendly warning");
        TS_WARN("Warnings don't abort the test");
    }
};

// Declare Tests in the appropriate namespace
namespace FOO { namespace BAR { class Tests; } }

// Use explicit namespace declaration
class FOO::BAR::Tests : public CxxTest::TestSuite {
public:
    void testEquality() {
        TS_ASSERT_EQUALS(1, 1);
        TS_ASSERT_EQUALS(1, 2);
        TS_ASSERT_EQUALS('a', 'A');
        TS_ASSERT_EQUALS(1.0, -12345678900000000000000000000000000000000000000000.1234);
    }
};

#endif // __NAMESPACE2_H
