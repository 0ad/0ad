/* Copyright (C) 2019 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "lib/self_test.h"
#include "lib/timer.h"

#include "simulation2/serialization/ISerializer.h"
#include "simulation2/serialization/IDeserializer.h"

#include "simulation2/system/EntityMap.h"

class TestEntityMap : public CxxTest::TestSuite
{
public:
	void setUp()
	{
	}

	void tearDown()
	{
	}

	void test_insert_or_assign()
	{
		EntityMap<int, 1> test;

		TS_ASSERT(test.empty());

		test.insert_or_assign(1,1);
		test.insert_or_assign(2,2);
		test.insert_or_assign(3,3);
		test.insert_or_assign(4,4);
		test.insert_or_assign(4,5);
		test.insert_or_assign(4,6);

		TS_ASSERT(test.m_Data.size() == 5);
		TS_ASSERT(test.m_Data.back().first != INVALID_ENTITY);
		TS_ASSERT(test.size() == 4);
		TS_ASSERT(test.find(3)->second == 3);
		TS_ASSERT(test.find(4)->second == 6);

		test.insert_or_assign(10,7);
		TS_ASSERT(test.m_Data.size() == 11);
		TS_ASSERT(test.m_Data.back().first != INVALID_ENTITY);
		TS_ASSERT(test.size() == 5);
		TS_ASSERT(test.find(4)->second == 6);
		TS_ASSERT(test.find(5) == test.end());
		TS_ASSERT(test.find(6) == test.end());
		TS_ASSERT(test.find(7) == test.end());
		TS_ASSERT(test.find(8) == test.end());
		TS_ASSERT(test.find(9) == test.end());
		TS_ASSERT(test.find(10)->second == 7);

		EntityMap<int, 5> test2;

		test2.insert_or_assign(8,5);
		TS_ASSERT(test2.find(8)->second == 5);
		TS_ASSERT(test2.m_Data.size() == 5);
		TS_ASSERT(test2.size() == 1);

	}
	void test_iterators()
	{
		EntityMap<int, 1> test;

		test.insert_or_assign(1,1);
		test.insert_or_assign(2,2);
		test.insert_or_assign(3,3);
		test.insert_or_assign(4,4);

		EntityMap<int, 1>::iterator it = test.begin();
		TS_ASSERT(it->first == 1);
		it++;
		TS_ASSERT(it->first == 2);
		++it;
		TS_ASSERT(it->first == 3);
		it = test.end();
		TS_ASSERT(it->first == test.m_Data.back().first);

		EntityMap<int, 1>::const_iterator cit = test.begin();
		TS_ASSERT(cit->first == 1);
		cit = test.end();
		TS_ASSERT(cit->first == test.m_Data.back().first);

		size_t iter = 0;
		for (EntityMap<int, 1>::value_type& v : test)
		{
			++iter;
			TS_ASSERT(test.find(iter)->second == (int)iter);
			TS_ASSERT(test.find(iter)->second == v.second);
		}
		TS_ASSERT(iter == 4);

		test.clear();

		test.insert_or_assign(10,1);
		test.insert_or_assign(20,2);
		test.insert_or_assign(30,3);
		test.insert_or_assign(40,4);

		it = test.begin();
		TS_ASSERT(it->second == 1);
		it++;
		TS_ASSERT(it->second == 2);
		++it;
		TS_ASSERT(it->second == 3);
		it = test.end();
		TS_ASSERT(it->first == test.m_Data.back().first);

	}

	void test_erase()
	{
		EntityMap<int> test;
		test.insert_or_assign(1,1);
		test.insert_or_assign(2,2);
		test.insert_or_assign(3,3);
		test.insert_or_assign(4,4);

		test.erase(2);

		TS_ASSERT(test.m_Data.size() == 5);
		TS_ASSERT(test.size() == 3);
		TS_ASSERT(test.find(2) == test.end());

		test.erase(1);
		test.erase(3);
		TS_ASSERT(test.erase(4) == 1);

		TS_ASSERT(test.m_Data.size() == 5);
		TS_ASSERT(test.size() == 0);

		TS_ASSERT(test.erase(5) == 0);

		test.insert_or_assign(1,1);
		test.insert_or_assign(2,2);
		test.insert_or_assign(3,3);
		test.insert_or_assign(4,4);

		test.erase(test.begin());
		TS_ASSERT(test.m_Data.size() == 5);
		TS_ASSERT(test.size() == 3);
		TS_ASSERT(test.find(1) == test.end());

		TS_ASSERT(test.erase(test.end()) == 0);
		TS_ASSERT(test.m_Data.back().first != INVALID_ENTITY);
	}

	void test_clear()
	{
		EntityMap<int> test;
		test.insert_or_assign(1,1);
		test.insert_or_assign(2,2);
		test.insert_or_assign(3,3);
		test.insert_or_assign(4,4);

		test.clear();

		TS_ASSERT(test.m_Data.size() == 1);
		TS_ASSERT(test.size() == 0);
	}

	void test_find()
	{
		EntityMap<int> test;
		test.insert_or_assign(1,1);
		test.insert_or_assign(2,2);
		test.insert_or_assign(3,3);
		test.insert_or_assign(40,4);

		TS_ASSERT(test.find(1)->second == 1);
		TS_ASSERT(test.find(40)->second == 4);
		TS_ASSERT(test.find(30) == test.end());
	}

	void test_perf_DISABLED()
	{
		EntityMap<int> test;
		printf("Testing performance of EntityMap\n");

		double t = timer_Time();
		for (int i = 1; i <= 200000; ++i)
			test.insert_or_assign(i,i);
		double tt = timer_Time() - t;
		printf("insert_or_assigning 200K elements in order: %lfs\n", tt);

		t = timer_Time();
		for (int i = 1; i <= 200000; ++i)
			test.erase(i);
		tt = timer_Time() - t;
		printf("Erasing 200K elements, by key: %lfs\n", tt);

		t = timer_Time();
		for (int i = 200000; i >= 1; --i)
			test.insert_or_assign(i,i);
		tt = timer_Time() - t;
		printf("insert_or_assigning 200K elements in reverse order: %lfs\n", tt);

		t = timer_Time();
		for (auto i = test.begin(); i != test.end(); ++i)
			test.erase(i);
		tt = timer_Time() - t;
		printf("Erasing 200K elements, by iterator: %lfs\n", tt);

		t = timer_Time();
		for (int i = 1; i <= 200000; ++i)
			test.erase(i);
		tt = timer_Time() - t;
		printf("Erasing 200K non-existing elements: %lfs\n", tt);

		// prep random vector
		std::vector<int> vec;
		for (int i = 1; i <= 200000; ++i)
			vec.push_back(i);
		std::random_shuffle(vec.begin(), vec.end());

		for (int i = 1; i <= 200000; ++i)
			test.insert_or_assign(i,i);

		t = timer_Time();
		for (int i = 1; i <= 200000; ++i)
			test.find(vec[i])->second = 3;
		tt = timer_Time() - t;
		printf("200K random lookups in random order: %lfs\n", tt);

		t = timer_Time();
		for (auto& p : test)
			p.second = 3;
		tt = timer_Time() - t;
		printf("auto iteration on 200K continuous entitymap: %lfs\n", tt);

		test.clear();

		for (int i = 1; i <= 200000; ++i)
			test.insert_or_assign(i*5,i);

		t = timer_Time();
		for (EntityMap<int>::value_type& p : test)
			p.second = 3;
		tt = timer_Time() - t;
		printf("auto iteration on 200K sparse (holes of 5): %lfs\n", tt);

		test.clear();

		for (int i = 1; i <= 4000; ++i)
			test.insert_or_assign(i*50,i);

		t = timer_Time();
		for (EntityMap<int>::value_type& p : test)
			p.second = 3;
		tt = timer_Time() - t;
		printf("auto iteration on 4K sparse (holes of 50): %lfs\n", tt);

		test.clear();

		for (int i = 1; i <= 200000; ++i)
			test.insert_or_assign(i*50,i);

		t = timer_Time();
		for (EntityMap<int>::value_type& p : test)
			p.second = 3;
		tt = timer_Time() - t;
		printf("auto iteration on 200K sparse (holes of 50): %lfs\n", tt);

		test.clear();

		for (int i = 1; i <= 2000000; ++i)
			test.insert_or_assign(i*50,i);

		t = timer_Time();
		for (EntityMap<int>::iterator i = test.begin(); i != test.end(); ++i)
			i->second = 3;
		tt = timer_Time() - t;
		printf("manual ++iteration on 2000K sparse (holes of 50) (warmup 1): %lfs\n", tt);

		t = timer_Time();
		for (EntityMap<int>::iterator i = test.begin(); i != test.end(); i++)
			i->second = 3;
		tt = timer_Time() - t;
		printf("manual ++iteration on 2000K sparse (holes of 50) (warmup 2): %lfs\n", tt);

		t = timer_Time();
		for (EntityMap<int>::iterator i = test.begin(); i != test.end(); ++i)
			i->second = 3;
		tt = timer_Time() - t;
		printf("manual ++iteration on 2000K sparse (holes of 50): %lfs\n", tt);

		t = timer_Time();
		for (EntityMap<int>::iterator i = test.begin(); i != test.end(); i++)
			i->second = 3;
		tt = timer_Time() - t;
		printf("manual iteration++ on 2000K sparse (holes of 50): %lfs\n", tt);
	}
};
