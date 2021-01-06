/* Copyright (C) 2021 Wildfire Games.
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

#include "lib/allocators/DynamicArena.h"
#include "lib/allocators/STLAllocators.h"

#include <functional>
#include <map>
#include <unordered_map>
#include <vector>

class TestSTLAllocators : public CxxTest::TestSuite
{
public:
	struct SomeData
	{
		SomeData(int) {};
		int padding[10];
	};

	template<template<typename... Args> typename Alloc, typename D>
	using Allocator = Alloc<D, Allocators::DynamicArena<4096>>;

	template<typename K, typename V, template<typename... Args> typename Alloc>
	using Map = std::map<K, V, std::less<K>, Allocator<Alloc, typename std::map<K, V>::value_type>>;

	template<typename K, typename V, template<typename... Args> typename Alloc>
	using UMap = std::unordered_map<K, V, std::hash<K>, std::equal_to<K>, Allocator<Alloc, typename std::unordered_map<K, V>::value_type>>;

	void test_CustomAllocator()
	{
		Map<int, SomeData, STLAllocator> map;
		UMap<int, SomeData, STLAllocator> umap;
		std::vector<SomeData, STLAllocator<SomeData, Allocators::DynamicArena<4096>>> vec;

		map.emplace(4, 5);
		umap.emplace(4, 5);
		vec.emplace_back(5);

		map.clear();
		umap.clear();
		vec.clear();
	}

	void test_ProxyAllocator()
	{
		Allocators::DynamicArena<4096> arena;
		Map<int, SomeData, ProxyAllocator> map(arena);
		UMap<int, SomeData, ProxyAllocator> umap(arena);
		std::vector<SomeData, Allocator<ProxyAllocator, SomeData>> vec(arena);

		map.emplace(4, 5);
		umap.emplace(4, 5);
		vec.emplace_back(5);

		map.clear();
		umap.clear();
		vec.clear();
	}
};
