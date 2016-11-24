/* Copyright (c) 2010 Wildfire Games
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "lib/self_test.h"

#include "lib/adts/cache_adt.h"
#include "lib/rand.h"

class TestCache: public CxxTest::TestSuite
{
public:
	void test_cache_perf()
	{
		Cache<int, int, Landlord_Naive> c1;
		Cache<int, int, Landlord_Naive, Divider_Recip> c1r;
		Cache<int, int, Landlord_Cached> c2;
		Cache<int, int, Landlord_Cached, Divider_Recip> c2r;
		Cache<int, int, Landlord_Lazy> c3;
		Cache<int, int, Landlord_Lazy, Divider_Recip> c3r;

#if defined(ENABLE_CACHE_POLICY_BENCHMARK) || 0
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
				size_t cost = (size_t)rand(1,100);\
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
	}

	// ensures all 3 variants of Landlord<> behave the same
	// [PT: disabled because it's far too slow]
	void DISABLED_test_cache_policies()
	{
		Cache<int, int, Landlord_Naive > c1;
		Cache<int, int, Landlord_Cached> c2;
		Cache<int, int, Landlord_Lazy  > c3;

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
				size_t cost = (size_t)rand(1,100);
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
				TS_ASSERT_EQUALS(removed1, removed2);
				TS_ASSERT_EQUALS(removed2, removed3);
				if (removed1)
				{
					TS_ASSERT_EQUALS(size1, size2);
					TS_ASSERT_EQUALS(value1, value2);
					TS_ASSERT_EQUALS(size2, size3);
					TS_ASSERT_EQUALS(value2, value3);
				}
			}	// else
		}	// for i
	}
};
