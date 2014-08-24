//
// This is a test of CxxTest's Mock framework (not a mock test).
//
#include <cxxtest/TestSuite.h>

//
// Here are the "real" functions
//
static int one(void) { return 1; }
static void two(int *p) { *p = 2; }

namespace NameSpace
{
static int identity(int i) { return i; }
static double identity(double d) { return d; }
}

class Opaque
{
public:
    explicit Opaque(int i) : value(i) {}
    int value;
};

static Opaque getOpaque(int i)
{
    return Opaque(i);
}

#define CXXTEST_MOCK_TEST_SOURCE_FILE
#include <cxxtest/Mock.h>

CXXTEST_MOCK_GLOBAL(int, one, (void), ());
CXXTEST_MOCK_VOID_GLOBAL(two, (int *p), (p));

CXXTEST_MOCK(intIdentity, int, identity, (int i), NameSpace::identity, (i));
CXXTEST_MOCK(doubleIdentity, double, identity, (double i), NameSpace::identity, (i));

CXXTEST_MOCK_DEFAULT_VALUE(Opaque, Opaque(42));
CXXTEST_MOCK_GLOBAL(Opaque, getOpaque, (int i), (i));

CXXTEST_SUPPLY_GLOBAL(int, supplyOne, (void), ());
CXXTEST_SUPPLY_VOID_GLOBAL(supplyTwo, (int *p), (p));

CXXTEST_SUPPLY(SupplyThree, int, doSupplyThree, (void), supplyThree, ());
CXXTEST_SUPPLY_VOID(SupplyFour, doSupplyFour, (int *p), supplyFour, (p));

class MockOne : public T::Base_one
{
public:
    MockOne(int i) : result(i) {}
    int result;
    int one() { return result; }
};

class MockIntIdentity : public T::Base_intIdentity
{
public:
    MockIntIdentity(int i) : result(i) {}
    int result;
    int identity(int) { return result; }
};

class MockDoubleIdentity : public T::Base_doubleIdentity
{
public:
    MockDoubleIdentity(double d) : result(d) {}
    double result;
    double identity(double) { return result; }
};

class MockGetOpaque : public T::Base_getOpaque
{
public:
    MockGetOpaque(int i) : result(i) {}
    Opaque result;
    Opaque getOpaque(int) { return result; }
};

class SupplyOne : public T::Base_supplyOne
{
public:
    SupplyOne(int i) : result(i) {}
    int result;
    int supplyOne() { return result; }
};

class SupplyTwo : public T::Base_supplyTwo
{
public:
    SupplyTwo(int i) : result(i) {}
    int result;
    void supplyTwo(int *p) { *p = result; }
};

class SupplyThree : public T::Base_SupplyThree
{
public:
    SupplyThree(int i) : result(i) {}
    int result;
    int doSupplyThree() { return result; }
};

class SupplyFour : public T::Base_SupplyFour
{
public:
    SupplyFour(int i) : result(i) {}
    int result;
    void doSupplyFour(int *p) { *p = result; }
};

class MockTest : public CxxTest::TestSuite
{
public:
    void test_Mock()
    {
        MockOne mockOne(2);
        TS_ASSERT_EQUALS(T::one(), 2);
    }

    void test_Real()
    {
        T::Real_one realOne;
        TS_ASSERT_EQUALS(T::one(), 1);
    }

    void test_Unimplemented()
    {
        TS_ASSERT_EQUALS(T::one(), 1);
    }

    void test_More_complex_mock()
    {
        MockIntIdentity mii(53);
        MockDoubleIdentity mdi(71);

        TS_ASSERT_EQUALS(T::identity((int)5), 53);
        TS_ASSERT_EQUALS(T::identity((double)5.0), 71);
    }

    void test_Mock_traits()
    {
        TS_ASSERT_EQUALS(T::getOpaque(3).value, 72);
    }

    void test_Override()
    {
        MockOne *two = new MockOne(2);
        MockOne *three = new MockOne(3);
        MockOne *four = new MockOne(4);
        TS_ASSERT_EQUALS(T::one(), 4);
        delete three;
        TS_ASSERT_EQUALS(T::one(), 4);
        delete four;
        TS_ASSERT_EQUALS(T::one(), 2);
        delete two;
        TS_ASSERT_EQUALS(T::one(), 1);
    }

    void test_Supply()
    {
        SupplyOne s(2);
        TS_ASSERT_EQUALS(supplyOne(), 2);
    }

    void test_Unimplemented_supply()
    {
        TS_ASSERT_EQUALS(supplyOne(), 1);
    }

    void test_More_complex_supply()
    {
        SupplyThree st(28);
        SupplyFour sf(53);

        TS_ASSERT_EQUALS(supplyThree(), 28);

        int i;
        supplyFour(&i);
        TS_ASSERT_EQUALS(i, 53);
    }
};
