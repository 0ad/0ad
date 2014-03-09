#include <cxxtest/TestSuite.h>

class Thing
{
    int _i;
public:
    Thing(int argI) : _i(argI) {}
    int i() const { return _i; }
};

class Fail
{
public:
    bool operator()(int) const { return false; }
    bool operator()(int, int) const { return false; }
};

class ThrowsAssert : public CxxTest::TestSuite
{
public:
    void test_TS_ASSERT_THROWS_EQUALS()
    {
        TS_ASSERT_THROWS_EQUALS( { throw 1; }, int i, i, 2);
        TS_ASSERT_THROWS_EQUALS( { throw Thing(1); }, const Thing & thing, thing.i(), 2);
    }

    void test_TS_ASSERT_THROWS_DIFFERS()
    {
        TS_ASSERT_THROWS_DIFFERS( { throw 1; }, int i, i, 1);
        TS_ASSERT_THROWS_DIFFERS( { throw Thing(1); }, const Thing & thing, thing.i(), 1);
    }

    void test_TS_ASSERT_THROWS_SAME_DATA()
    {
        TS_ASSERT_THROWS_SAME_DATA( { throw "123"; }, const char * s, s, "456", 3);
    }

    void test_TS_ASSERT_THROWS_LESS_THAN()
    {
        TS_ASSERT_THROWS_LESS_THAN( { throw 1; }, int i, i, 1);
        TS_ASSERT_THROWS_LESS_THAN( { throw Thing(1); }, const Thing & thing, thing.i(), 1);
    }

    void test_TS_ASSERT_THROWS_LESS_THAN_EQUALS()
    {
        TS_ASSERT_THROWS_LESS_THAN_EQUALS( { throw 1; }, int i, i, 0);
        TS_ASSERT_THROWS_LESS_THAN_EQUALS( { throw Thing(1); }, const Thing & thing, thing.i(), 0);
    }

    void test_TS_ASSERT_THROWS_PREDICATE()
    {
        TS_ASSERT_THROWS_PREDICATE( { throw 1; }, int i, Fail, i);
        TS_ASSERT_THROWS_PREDICATE( { throw Thing(1); }, const Thing & thing, Fail, thing.i());
    }

    void test_TS_ASSERT_THROWS_RELATION()
    {
        TS_ASSERT_THROWS_RELATION( { throw 1; }, int i, Fail, i, 1);
        TS_ASSERT_THROWS_RELATION( { throw Thing(1); }, const Thing & thing, Fail, thing.i(), 1);
    }

    void test_TS_ASSERT_THROWS_DELTA()
    {
        TS_ASSERT_THROWS_DELTA( { throw 1; }, int i, i, 3, 1);
        TS_ASSERT_THROWS_DELTA( { throw Thing(1); }, const Thing & thing, thing.i(), 3, 1);
    }

    void test_TS_ASSERT_THROWS_ASSERT()
    {
        TS_ASSERT_THROWS_ASSERT( { throw 1; }, int i,
                                 TS_ASSERT_EQUALS(i, 2));

        TS_ASSERT_THROWS_ASSERT( { throw Thing(1); }, const Thing & thing,
                                 TS_ASSERT_EQUALS(thing.i(), 2));

        TS_ASSERT_THROWS_ASSERT( { throw Thing(1); }, const Thing & thing,
                                 TS_FAIL(thing.i()));

        TS_ASSERT_THROWS_ASSERT( { throw Thing(1); }, const Thing & thing,
                                 TS_ASSERT(thing.i() - 1));

        char zero = 0, one = 1;
        TS_ASSERT_THROWS_ASSERT( { throw Thing(1); }, const Thing &,
                                 TS_ASSERT_SAME_DATA(&zero, &one, sizeof(char)));

        TS_ASSERT_THROWS_ASSERT( { throw Thing(1); }, const Thing & thing,
                                 TS_ASSERT_DELTA(thing.i(), 5, 2));

        TS_ASSERT_THROWS_ASSERT( { throw Thing(1); }, const Thing & thing,
                                 TS_ASSERT_DIFFERS(thing.i(), 1));

        TS_ASSERT_THROWS_ASSERT( { throw Thing(1); }, const Thing & thing,
                                 TS_ASSERT_LESS_THAN(thing.i(), 1));

        TS_ASSERT_THROWS_ASSERT( { throw Thing(1); }, const Thing & thing,
                                 TS_ASSERT_PREDICATE(Fail, thing.i()));

        TS_ASSERT_THROWS_ASSERT( { throw Thing(1); }, const Thing & thing,
                                 TS_ASSERT_RELATION(Fail, thing.i(), 33));
    }
};
