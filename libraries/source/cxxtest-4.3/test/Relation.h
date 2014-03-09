#include <cxxtest/TestSuite.h>

struct MyNegative
{
    bool operator()(const int &i) const { return i < 0; }
};

template<class T>
struct MyLess
{
    bool operator()(const T &x, const T &y) const { return x < y; }
};

class Relation : public CxxTest::TestSuite
{
public:
    void testPredicate()
    {
        TS_ASSERT_PREDICATE(MyNegative, 1);
        TSM_ASSERT_PREDICATE("1 <? 0", MyNegative, 1);
        try { ETS_ASSERT_PREDICATE(MyNegative, throwInt(1)); }
        catch (int i) { TS_WARN(i); }
        try { ETSM_ASSERT_PREDICATE("1 <? 0", MyNegative, throwInt(1)); }
        catch (int i) { TS_WARN(i); }
    }

    void testRelation()
    {
        TS_ASSERT_RELATION(MyLess<int>, 2, 1);
        TSM_ASSERT_RELATION("2 <? 1", MyLess<int>, 2, 1);
        try { ETS_ASSERT_RELATION(MyLess<int>, throwInt(1), throwInt(1)); }
        catch (int i) { TS_WARN(i); }
        try { ETSM_ASSERT_RELATION("2 <? 1", MyLess<int>, throwInt(1), throwInt(1)); }
        catch (int i) { TS_WARN(i); }
    }

    int throwInt(int i)
    {
        throw i;
    }
};
