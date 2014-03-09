#ifndef LINKEDLIST_TEST_H
#define LINKEDLIST_TEST_H

#include <cxxtest/LinkedList.h>

class TestLink : public CxxTest::Link
{
public:
    bool setUp() { return true; }
    bool tearDown() { return true; }
};

#include <cxxtest/TestSuite.h>
class LinkedList_test : public CxxTest::TestSuite
{
public:
    void test_initialize()
    {
        CxxTest::List list;
        list.initialize();
        TS_ASSERT_EQUALS((CxxTest::Link*)0, list.head());
        TS_ASSERT_EQUALS((CxxTest::Link*)0, list.tail());
        TS_ASSERT_EQUALS(0, list.size());
        TS_ASSERT(list.empty());
    }

    void test_attach()
    {
        CxxTest::List list;
        TestLink link;

        list.initialize();
        link.attach(list);

        TS_ASSERT_EQUALS(1, list.size());
        TS_ASSERT_EQUALS((CxxTest::Link*)&link, list.head());
        TS_ASSERT_EQUALS((CxxTest::Link*)&link, list.tail());
    }

    void test_detach()
    {
        CxxTest::List list;
        TestLink link;

        list.initialize();
        link.attach(list);
        link.detach(list);

        TS_ASSERT_EQUALS((CxxTest::Link*)0, list.head());
        TS_ASSERT_EQUALS((CxxTest::Link*)0, list.tail());
        TS_ASSERT_EQUALS(0, list.size());
        TS_ASSERT(list.empty());
    }
};


#endif // __SIMPLETEST_H
