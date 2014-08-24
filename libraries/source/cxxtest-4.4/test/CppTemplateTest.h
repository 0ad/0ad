#include <cxxtest/TestSuite.h>

template <class T>
class Tests
{
public:

    CXXTEST_STD(list)<T>* cache;

    void setUp()
    {
        this->cache = new CXXTEST_STD(list)<T>();
    }

    void tearDown()
    {
        delete this->cache;
    }

    void test_size()
    {
        TS_ASSERT_EQUALS(cache->size(), 0);
    }

    void test_insert()
    {
        this->cache->push_back(1);
        TS_ASSERT_EQUALS(cache->size(), 1);
    }

};

class IntTests: public Tests<int>, public CxxTest::TestSuite
{
public:

    void setUp()    { Tests<int>::setUp(); }
    void tearDown() { Tests<int>::tearDown(); }
};
