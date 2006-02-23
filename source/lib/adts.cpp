#include "precompiled.h"

#include <deque>

#include "adts.h"
#include "posix.h"
#include "lib/timer.h"

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



// ensures all 3 variants of Landlord<> behave the same
static void test_cache_removal()
{
	Cache<int, int, Landlord_Naive> c1;
	Cache<int, int, Landlord_Naive, Divider_Recip> c1r;
	Cache<int, int, Landlord_Cached> c2;
	Cache<int, int, Landlord_Cached, Divider_Recip> c2r;
	Cache<int, int, Landlord_Lazy> c3;
	Cache<int, int, Landlord_Lazy, Divider_Recip> c3r;

#if 0
	// set max priority, to reduce interference while measuring.
	int old_policy; static sched_param old_param;	// (static => 0-init)
	pthread_getschedparam(pthread_self(), &old_policy, &old_param);
	static sched_param max_param;
	max_param.sched_priority = sched_get_priority_max(SCHED_FIFO);
	pthread_setschedparam(pthread_self(), SCHED_FIFO, &max_param);

#define MEASURE(c, desc)\
{\
srand(1);\
int cnt = 1;\
TIMER_BEGIN(desc);\
for(int i = 0; i < 30000; i++)\
{\
	/* 70% add (random objects) */\
	bool add = rand(1,10) < 7;\
	if(add)\
	{\
		int key = cnt++;\
		int val = cnt++;\
		size_t size = (size_t)rand(1,100);\
		uint cost = (uint)rand(1,100);\
		c.add(key, val, size, cost);\
	}\
	else\
	{\
		size_t size;\
		int value;\
		c.remove_least_valuable(&value, &size);\
	}\
}\
TIMER_END(desc);\
}
	MEASURE(c1, "naive")
	MEASURE(c1r, "naiverecip")
	MEASURE(c2, "cached")
	MEASURE(c2r, "cachedrecip")
	MEASURE(c3, "lazy")
	MEASURE(c3r, "lazyrecip")

	// restore previous policy and priority.
	pthread_setschedparam(pthread_self(), old_policy, &old_param);
exit(1134);
#endif


	srand(1);
	int cnt = 1;
	for(int i = 0; i < 1000; i++)
	{
		// 70% add (random objects)
		bool add = rand(1,10) < 7;
		if(add)
		{
			int key = cnt++;
			int val = cnt++;
			size_t size = (size_t)rand(1,100);
			uint cost = (uint)rand(1,100);
			c1.add(key, val, size, cost);
			c2.add(key, val, size, cost);
			c3.add(key, val, size, cost);
		}
		// 30% delete - make sure "least valuable" was same for all
		else
		{
			size_t size1, size2, size3;
			int value1, value2, value3;
			bool removed1, removed2, removed3;
			removed1 = c1.remove_least_valuable(&value1, &size1);
			removed2 = c2.remove_least_valuable(&value2, &size2);
			removed3 = c3.remove_least_valuable(&value3, &size3);
			TEST(removed1 == removed2);
			TEST(size1 == size2);
			TEST(value1 == value2);
			TEST(removed2 == removed3);
			TEST(size2 == size3);
			TEST(value2 == value3);

		}	// else
	}	// for i
}


static void self_test()
{
	test_ringbuf();
	test_cache_removal();
}

SELF_TEST_REGISTER;

}	// namespace test
#endif	// #if SELF_TEST_ENABLED
