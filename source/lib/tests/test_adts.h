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

#include "lib/adts/ring_buf.h"
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

			TS_ASSERT_EQUALS(buf.size(), deq.size());
			RingBuf<int, N>::iterator begin = buf.begin(), end = buf.end();
			TS_ASSERT(std::equal(begin, end, deq.begin()));
		}
	}
};
