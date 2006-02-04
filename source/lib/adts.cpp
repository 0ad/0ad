#include "precompiled.h"

#include "adts.h"
#include <deque>

//-----------------------------------------------------------------------------
// built-in self test
//-----------------------------------------------------------------------------

#if SELF_TEST_ENABLED
namespace test {

static void test_ringbuf()
{
	const size_t N = 49;	// RingBuf capacity
	const size_t S = 100;	// number of test items

	// insert and remove immediately
	{
	RingBuf<int, N> buf;
	for(int i = 1; i < S; i++)
	{
		buf.push_back(i);
		TEST(buf.front() == i);
		buf.pop_front();
	}
	TEST(buf.size() == 0 && buf.empty());
	}

	// fill buffer and overwrite old items
	{
	RingBuf<int, N> buf;
	for(int i = 1; i < S; i++)
		buf.push_back(i);
	TEST(buf.size() == N);
	int first = buf.front();
	TEST(first == (int)(S-1 -N +1));
	for(size_t i = 0; i < N; i++)
	{
		TEST(buf.front() == first);
		first++;
		buf.pop_front();
	}
	TEST(buf.size() == 0 && buf.empty());
	}

	// randomized insert/remove; must behave as does std::deque
	{
	srand(1);
	RingBuf<int, N> buf;
	std::deque<int> deq;
	for(uint rep = 0; rep < 1000; rep++)
	{
		uint rnd_op = rand(0, 10);
		// 70% - insert
		if(rnd_op >= 3)
		{
			int item = rand();
			buf.push_back(item);

			deq.push_back(item);
			int excess_items = (int)deq.size() - N;
			if(excess_items > 0)
			{
				for(int i = 0; i < excess_items; i++)
				{
					deq.pop_front();
				}
			}
		}
		// 30% - pop front (only if not empty)
		else if(!deq.empty())
		{
			buf.pop_front();
			deq.pop_front();
		}
	}
	TEST(buf.size() == deq.size());
	RingBuf<int, N>::iterator begin = buf.begin(), end = buf.end();
	TEST(equal(begin, end, deq.begin()));
	}
}

static void self_test()
{
	test_ringbuf();
}

SELF_TEST_RUN;

}	// namespace test
#endif	// #if SELF_TEST_ENABLED
