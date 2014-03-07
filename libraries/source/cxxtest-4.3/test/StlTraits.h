#include <cxxtest/TestSuite.h>

class StlTraits : public CxxTest::TestSuite
{
public:
    typedef CXXTEST_STD(string) String;
    typedef CXXTEST_STD(pair)<int, String> IntString;
    typedef CXXTEST_STD(pair)<String, double> StringDouble;

    void test_Pair()
    {
        IntString three(3, "Three");
        TS_FAIL(three);
        StringDouble four("Four", 4.0);
        TS_FAIL(four);
    }

    void test_Vector()
    {
        CXXTEST_STD(vector)<int> v;
        TS_TRACE(v);
        v.push_back(1);
        v.push_back(2);
        v.push_back(3);
        TS_FAIL(v);

        CXXTEST_STD(vector)<String> w;
        TS_TRACE(w);
        w.push_back("One");
        w.push_back("Two");
        w.push_back("Three");
        TS_FAIL(w);

        CXXTEST_STD(vector)<IntString> vw;
        TS_TRACE(vw);
        vw.push_back(IntString(1, "One"));
        vw.push_back(IntString(2, "Two"));
        vw.push_back(IntString(3, "Three"));
        TS_FAIL(vw);
    }

    void test_List()
    {
        CXXTEST_STD(list)<int> v;
        TS_TRACE(v);
        v.push_back(1);
        v.push_back(2);
        v.push_back(3);
        TS_FAIL(v);

        CXXTEST_STD(list)<String> w;
        TS_TRACE(w);
        w.push_back("One");
        w.push_back("Two");
        w.push_back("Three");
        TS_FAIL(w);

        CXXTEST_STD(list)<IntString> vw;
        TS_TRACE(vw);
        vw.push_back(IntString(1, "One"));
        vw.push_back(IntString(2, "Two"));
        vw.push_back(IntString(3, "Three"));
        TS_FAIL(vw);
    }

    void test_Set()
    {
        CXXTEST_STD(set)<int> v;
        TS_TRACE(v);
        v.insert(1);
        v.insert(2);
        v.insert(3);
        TS_FAIL(v);

        CXXTEST_STD(set)<String> w;
        TS_TRACE(w);
        w.insert("One");
        w.insert("Two");
        w.insert("Three");
        TS_FAIL(w);

        CXXTEST_STD(set)<IntString> vw;
        TS_TRACE(vw);
        vw.insert(IntString(1, "One"));
        vw.insert(IntString(2, "Two"));
        vw.insert(IntString(3, "Three"));
        TS_FAIL(vw);
    }

    void test_Map()
    {
        CXXTEST_STD(map)<String, String> m;
        TS_TRACE(m);

        m["Jack"] = "Jill";
        m["Humpty"] = "Dumpty";
        m["Ren"] = "Stimpy";

        TS_FAIL(m);

        CXXTEST_STD(map)< unsigned, CXXTEST_STD(list)<unsigned> > n;
        TS_TRACE(n);

        n[6].push_back(2);
        n[6].push_back(3);
        n[210].push_back(2);
        n[210].push_back(3);
        n[210].push_back(5);
        n[210].push_back(7);

        TS_FAIL(n);
    }

    void test_Deque()
    {
        CXXTEST_STD(deque)<int> d;
        TS_TRACE(d);
        d.push_front(1);
        d.push_front(2);
        d.push_front(3);
        d.push_front(4);
        TS_FAIL(d);
    }

    void test_MultiMap()
    {
        CXXTEST_STD(multimap)<String, double> mm;
        TS_TRACE(mm);

        mm.insert(StringDouble("One", 1.0));
        mm.insert(StringDouble("Two", 2.0));
        TS_FAIL(mm);
    }

    void test_MultiSet()
    {
        CXXTEST_STD(multiset)<int> ms;
        TS_TRACE(ms);

        ms.insert(123);
        ms.insert(456);
        TS_FAIL(ms);
    }

    void test_Complex()
    {
        typedef CXXTEST_STD(complex)<double> Complex;
        TS_FAIL(Complex(3.14, 2.71));
        TS_FAIL(Complex(0.0, 1.0));
        TS_FAIL(Complex(1.0, 0.0));
    }
};
