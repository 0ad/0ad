/* Copyright (C) 2022 Wildfire Games.
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

#include "ps/containers/StaticVector.h"

#include <algorithm>
#include <numeric>
#include <type_traits>
#include <utility>

class TestStaticVector : public CxxTest::TestSuite
{
public:
	class ConstructionCounter
	{
	public:
		ConstructionCounter(size_t& count) :
			m_Count{count}
		{
			++m_Count;
		}

		ConstructionCounter(const ConstructionCounter& other) :
			m_Count{other.m_Count}
		{
			++m_Count;
		}

		ConstructionCounter& operator=(const ConstructionCounter&)
		{
			return *this;
		}

		ConstructionCounter(ConstructionCounter&& other) :
			m_Count{other.m_Count}
		{
			++m_Count;
		}

		ConstructionCounter& operator=(ConstructionCounter&&)
		{
			return *this;
		}

		~ConstructionCounter()
		{
			--m_Count;
		}

		friend bool operator==(const ConstructionCounter&, const ConstructionCounter&)
		{
			return true;
		}

	private:
		size_t& m_Count;
	};

	void test_construction()
	{
		size_t count{0};
		{
			PS::StaticVector<ConstructionCounter, 18> vec{ConstructionCounter{count}};
			TS_ASSERT_EQUALS(count, 1);
			TS_ASSERT_EQUALS(vec.size(), 1);

			vec.pop_back();
			TS_ASSERT_EQUALS(count, 0);
			TS_ASSERT_EQUALS(vec.size(), 0);

			vec.push_back(count);
			TS_ASSERT_EQUALS(count, 1);
			{
				ConstructionCounter lval{count};
				vec.push_back(lval);
				vec.push_back(lval);
			}
			TS_ASSERT_EQUALS(count, 3);
			TS_ASSERT_EQUALS(vec.end() - vec.begin(), 3);

			vec.pop_back();
			TS_ASSERT_EQUALS(count, 2);

			vec.clear();
			TS_ASSERT_EQUALS(count, 0);
			TS_ASSERT(vec.empty());

			vec.emplace_back(count);
			vec.insert(vec.begin(), ConstructionCounter{count});
			TS_ASSERT_EQUALS(count, 2);
		}
		TS_ASSERT_EQUALS(count, 0);

		PS::StaticVector<ConstructionCounter, 10> vec0(6, count);
		TS_ASSERT_EQUALS(count, 6);

		PS::StaticVector<ConstructionCounter, 10> vec1(vec0);
		TS_ASSERT_EQUALS(count, 12);

		PS::StaticVector<ConstructionCounter, 12> vec2(vec1);
		TS_ASSERT_EQUALS(count, 18);
		vec2.clear();

		PS::StaticVector<ConstructionCounter, 12> vec3(vec2);
		TS_ASSERT_EQUALS(count, 12);
	}

	void test_assigne()
	{
		size_t count{0};

		PS::StaticVector<ConstructionCounter, 6> vec0(3, count);
		TS_ASSERT_EQUALS(count, 3);
		PS::StaticVector<ConstructionCounter, 6> vec1(vec0);
		TS_ASSERT_EQUALS(count, 6);
		vec0 = vec1;
		TS_ASSERT_EQUALS(count, 6);
		vec0.emplace_back(count);
		vec1 = vec0;
		TS_ASSERT_EQUALS(vec0, vec1);
		TS_ASSERT_EQUALS(count, 8);

		vec1.pop_back();
		vec1.pop_back();
		vec0 = vec1;
		TS_ASSERT_EQUALS(count, 4);
	}

	void test_exception()
	{
		TS_ASSERT_THROWS((PS::StaticVector<int, 3>(4)), PS::CapacityExceededException&);
		PS::StaticVector<int, 3> vec0(3);
		TS_ASSERT_THROWS(vec0.emplace_back(), PS::CapacityExceededException&);

		PS::StaticVector<int, 3> vec1;
		TS_ASSERT_THROWS(vec1.at(0), std::out_of_range&);
	}

	void test_ordering()
	{
		PS::StaticVector<int, 36> vec(36);
		std::iota(vec.begin(), vec.end(), 0);
		TS_ASSERT_EQUALS(vec[2], 2);
		TS_ASSERT_EQUALS(vec[24], 24);

		std::rotate(vec.begin(), vec.begin() + 12, vec.end());
		TS_ASSERT_EQUALS(vec.front(), 12);
		TS_ASSERT_EQUALS(vec[23], 35);
		TS_ASSERT_EQUALS(vec[24], 0);

		const auto pred
		{
			[](const int elem)
			{
				return elem < 10;
			}
		};

		std::partition(vec.begin(), vec.end(), pred);
		TS_ASSERT_LESS_THAN(vec[2], vec[10]);
		TS_ASSERT_LESS_THAN(vec[5], vec[20]);
		TS_ASSERT_EQUALS(std::partition_point(vec.begin(), vec.end(), pred), vec.begin() + 10);

		std::sort(vec.begin(), vec.end());
		TS_ASSERT(std::is_sorted(vec.begin(), vec.end()));
	}

	void test_compare()
	{
		TS_ASSERT((PS::StaticVector<int, 20>{0, 1, 2, 3} == PS::StaticVector<int, 20>{0, 1, 2, 3}));
		TS_ASSERT((PS::StaticVector<int, 20>{0, 1, 2, 3} == PS::StaticVector<int, 12>{0, 1, 2, 3}));
		TS_ASSERT((PS::StaticVector<int, 20>{0, 1, 2, 3} != PS::StaticVector<int, 20>{0, 1, 2, 4}));
		TS_ASSERT((PS::StaticVector<int, 20>{0, 1, 2, 3} != PS::StaticVector<int, 12>{0, 1, 2, 4}));
		TS_ASSERT((PS::StaticVector<int, 20>{0, 1, 2, 3} != PS::StaticVector<int, 20>{0, 1, 2}));
		TS_ASSERT((PS::StaticVector<int, 5>{0, 1, 2, 3} != PS::StaticVector<int, 1>{0}));
		TS_ASSERT((PS::StaticVector<int, 20>{0, 1, 2, 3} != PS::StaticVector<int, 20>{3, 2, 1, 0}));
	}

	// Types
	static_assert(std::is_same_v<decltype(std::declval<PS::StaticVector<int, 1>>().begin()), int*>);
	static_assert(std::is_same_v<decltype(std::declval<const PS::StaticVector<int, 1>>().begin()),
		const int*>);
	static_assert(std::is_same_v<decltype(std::declval<PS::StaticVector<int, 1>>().end()), int*>);
	static_assert(std::is_same_v<decltype(std::declval<const PS::StaticVector<int, 1>>().end()),
		const int*>);
	static_assert(std::is_same_v<decltype(std::declval<PS::StaticVector<int, 1>>().data()), int*>);
	static_assert(std::is_same_v<decltype(std::declval<const PS::StaticVector<int, 1>>().data()),
		const int*>);
	static_assert(std::is_same_v<decltype(std::declval<PS::StaticVector<int, 1>>().front()), int&>);
	static_assert(std::is_same_v<decltype(std::declval<const PS::StaticVector<int, 1>>().front()),
		const int&>);

	// Size-types
	static_assert(std::is_same_v<PS::StaticVector<int, 1>::size_type, uint_fast8_t>);
	static_assert(std::numeric_limits<PS::StaticVector<int, 500>::size_type>::max() > 255);
	static_assert(std::numeric_limits<PS::StaticVector<int, 1000>::difference_type>::min() < -128);

	static_assert(std::is_same_v<PS::StaticVector<int, 180>::size_type, uint_fast8_t>);
	static_assert(std::is_same_v<PS::StaticVector<int, 180>::difference_type, int_fast16_t>);
};
