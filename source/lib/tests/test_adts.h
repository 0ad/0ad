#include "lib/self_test.h"

#include "lib/adts.h"
#include "lib/rand.h"

class TestRingbuf : public CxxTest::TestSuite 
{
	static const size_t N = 49;	// RingBuf capacity
	static const int S = 100;	// number of test items
public:
	void test_insert_remove()
	{
		RingBuf<int, N> buf;
		for(int i = 1; i < S; i++)
		{
			buf.push_back(i);
			TS_ASSERT_EQUALS(buf.front(), i);
			buf.pop_front();
		}
		TS_ASSERT(buf.size() == 0 && buf.empty());
	}

	void test_fill_overwrite_old()
	{
		RingBuf<int, N> buf;
		for(int i = 1; i < S; i++)
			buf.push_back(i);
		TS_ASSERT_EQUALS(buf.size(), N);
		int first = buf.front();
		TS_ASSERT_EQUALS(first, (int)(S-1 -N +1));
		for(size_t i = 0; i < N; i++)
		{
			TS_ASSERT_EQUALS(buf.front(), first);
			first++;
			buf.pop_front();
		}
		TS_ASSERT(buf.size() == 0 && buf.empty());
	}

	void test_randomized_insert_remove()
	{
		srand(1);
		RingBuf<int, N> buf;
		std::deque<int> deq;
		for(size_t rep = 0; rep < 1000; rep++)
		{
			size_t rnd_op = rand(0, 10);
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

		TS_ASSERT_EQUALS(buf.size(), deq.size());
		RingBuf<int, N>::iterator begin = buf.begin(), end = buf.end();
		TS_ASSERT(equal(begin, end, deq.begin()));
	}
};
