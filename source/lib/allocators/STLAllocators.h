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

#ifndef INCLUDED_STL_ALLOCATORS
#define INCLUDED_STL_ALLOCATORS

#include <memory>
#include <type_traits>

/**
 * Adapt a 0 A.D.-style allocator for usage in STL containers.
 * Use 'Backend' as an underlying allocator.
 */
template<typename T, class Backend>
class STLAllocator
{
	template<typename A, class B>
	friend class STLAllocator;
public:
	using value_type = T;
	using pointer = T*;
	using is_always_equal = std::false_type;

	STLAllocator() : allocator(std::make_shared<Backend>())
	{
	}

	template<typename V>
	STLAllocator(const STLAllocator<V, Backend>& proxy) : allocator(proxy.allocator)
	{
	}

	template<typename V>
	struct rebind
	{
		using other = STLAllocator<V, Backend>;
	};

	T* allocate(size_t n)
	{
		return static_cast<T*>(allocator->allocate(n * sizeof(T), nullptr, alignof(T)));
	}

	void deallocate(T* ptr, const size_t n)
	{
		return allocator->deallocate(static_cast<void*>(ptr), n * sizeof(T));
	}

private:
	std::shared_ptr<Backend> allocator;
};


/**
 * Proxies allocation to another allocator.
 * This allows a single allocator to serve multiple STL containers.
 */
template<typename T, class Backend>
class ProxyAllocator
{
	template<typename A, class B>
	friend class ProxyAllocator;
public:
	using value_type = T;
	using pointer = T*;
	using is_always_equal = std::false_type;

	ProxyAllocator(Backend& alloc) : allocator(alloc)
	{
	}

	template<typename V>
	ProxyAllocator(const ProxyAllocator<V, Backend>& proxy) : allocator(proxy.allocator)
	{
	}

	template<typename V>
	struct rebind
	{
		using other = ProxyAllocator<V, Backend>;
	};

	T* allocate(size_t n)
	{
		return static_cast<T*>(allocator.allocate(n * sizeof(T), nullptr, alignof(T)));
	}

	void deallocate(T* ptr, const size_t n)
	{
		return allocator.deallocate(static_cast<void*>(ptr), n * sizeof(T));
	}

private:
	Backend& allocator;
};

#endif	// INCLUDED_STL_ALLOCATORS
