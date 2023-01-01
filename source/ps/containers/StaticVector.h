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

#ifndef INCLUDED_PS_STATICVECTOR
#define INCLUDED_PS_STATICVECTOR

#include <algorithm>
#include <array>
#include <cstdint>
#include <fmt/core.h>
#include <initializer_list>
#include <limits>
#include <memory>
#include <new>
#include <stdexcept>

namespace PS
{

struct CapacityExceededException : public std::length_error
{
	using std::length_error::length_error;
};

template<size_t N>
constexpr auto MakeSmallestCapableUnsigned()
{
	if constexpr (N <= std::numeric_limits<uint_fast8_t>::max())
		return static_cast<uint_fast8_t>(0);
	else if constexpr (N <= std::numeric_limits<uint_fast16_t>::max())
		return static_cast<uint_fast16_t>(0);
	else if constexpr (N <= std::numeric_limits<uint_fast32_t>::max())
		return static_cast<uint_fast32_t>(0);
	else if constexpr (N <= std::numeric_limits<uint_fast64_t>::max())
		return static_cast<uint_fast64_t>(0);
	else
	{
		static_assert(N <= std::numeric_limits<uintmax_t>::max());
		return static_cast<uintmax_t>(0);
	}
}

template<size_t N>
constexpr auto MakeSmallestCapableSigned()
{
	// TODO C++20: Use std::cmp_*
	if constexpr (N <= static_cast<uintmax_t>(std::numeric_limits<int_fast8_t>::max()) &&
		-static_cast<intmax_t>(N) >= std::numeric_limits<int_fast8_t>::min())
		return static_cast<int_fast8_t>(0);
	else if constexpr (N <= static_cast<uintmax_t>(std::numeric_limits<int_fast16_t>::max()) &&
		-static_cast<intmax_t>(N) >= std::numeric_limits<int_fast16_t>::min())
		return static_cast<int_fast16_t>(0);
	else if constexpr (N <= static_cast<uintmax_t>(std::numeric_limits<int_fast32_t>::max()) &&
		-static_cast<intmax_t>(N) >= std::numeric_limits<int_fast32_t>::min())
		return static_cast<int_fast32_t>(0);
	else if constexpr (N <= static_cast<uintmax_t>(std::numeric_limits<int_fast64_t>::max()) &&
		-static_cast<intmax_t>(N) >= std::numeric_limits<int_fast64_t>::min())
		return static_cast<int_fast64_t>(0);
	else
	{
		static_assert(N <= static_cast<uintmax_t>(std::numeric_limits<intmax_t>::max()) &&
			-static_cast<intmax_t>(N) >= std::numeric_limits<intmax_t>::min());
		return static_cast<intmax_t>(0);
	}
}

/**
 * A conntainer close to std::vector but the elements are stored in place:
 * There is a fixed capacity and there is no dynamic memory allocation.
 * Note: moving a StaticVector will be slower than moving a std::vector in
 * case of sizeof(StaticVector) > sizeof(std::vector).
 */
template<typename T, size_t N>
class StaticVector
{
public:
	static_assert(std::is_nothrow_destructible_v<T>);

	using value_type = T;
	using size_type = decltype(MakeSmallestCapableUnsigned<N>());
	using difference_type = decltype(MakeSmallestCapableSigned<N>());
	using reference = value_type&;
	using const_reference = const value_type&;
	using pointer = value_type*;
	using const_pointer = const value_type*;
	using iterator = pointer;
	using const_iterator = const_pointer;
	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;


	StaticVector() = default;
	StaticVector(const StaticVector& other) noexcept(std::is_nothrow_copy_constructible_v<T>)
		: m_Size{other.size()}
	{
		std::uninitialized_copy(other.begin(), other.end(), begin());
	}

	template<size_t OtherN>
	explicit StaticVector(const StaticVector<T, OtherN>& other) noexcept(
		std::is_nothrow_copy_constructible_v<T>)
		: m_Size{other.size()}
	{
		static_assert(OtherN < N);

		std::uninitialized_copy(other.begin(), other.end(), begin());
	}

	StaticVector& operator=(const StaticVector& other) noexcept(std::is_nothrow_copy_constructible_v<T>
		&& std::is_nothrow_copy_assignable_v<T>)
	{
		const size_type initializedCopies{std::min(other.size(), size())};
		std::copy_n(other.begin(), initializedCopies, begin());
		std::uninitialized_copy(other.begin() + initializedCopies, other.end(),
			begin() + initializedCopies);
		std::destroy(begin() + initializedCopies, end());

		m_Size = other.size();
		return *this;
	}

	template<size_t OtherN>
	StaticVector& operator=(const StaticVector<T, OtherN>& other) noexcept(
		std::is_nothrow_copy_constructible_v<T> && std::is_nothrow_copy_assignable_v<T>)
	{
		static_assert(OtherN < N);

		const size_type initializedCopies{std::min(other.size(), size())};
		std::copy_n(other.begin(), initializedCopies, begin());
		std::uninitialized_copy(other.begin() + initializedCopies, other.end(),
			begin() + initializedCopies);
		std::destroy(begin() + initializedCopies, end());

		m_Size = other.size();
		return *this;
	}

	StaticVector(StaticVector&& other) noexcept(std::is_nothrow_move_constructible_v<T>)
		: m_Size{other.size()}
	{
		std::uninitialized_move(other.begin(), other.end(), begin());
	}

	template<size_t OtherN>
	explicit StaticVector(StaticVector<T, OtherN>&& other)
		noexcept(std::is_nothrow_move_constructible_v<T>)
		: m_Size{other.size()}
	{
		static_assert(OtherN < N);

		std::uninitialized_move(other.begin(), other.end(), begin());
	}

	StaticVector& operator=(StaticVector&& other) noexcept(std::is_nothrow_move_constructible_v<T> &&
		std::is_nothrow_move_assignable_v<T>)
	{
		const size_type initializedMoves{std::min(other.size(), size())};
		std::move(other.begin(), other.begin() + initializedMoves, begin());
		std::uninitialized_move(other.begin() + initializedMoves, other.end(),
			begin() + initializedMoves);
		std::destroy(begin() + initializedMoves, end());

		m_Size = other.size();
		return *this;
	}

	template<size_t OtherN>
	StaticVector& operator=(StaticVector<T, OtherN>&& other) noexcept(
		std::is_nothrow_move_constructible_v<T> && std::is_nothrow_move_assignable_v<T>)
	{
		static_assert(OtherN < N);

		const size_type initializedMoves{std::min(other.size(), size())};
		std::move(other.begin(), other.begin() + initializedMoves, begin());
		std::uninitialized_move(other.begin() + initializedMoves, other.end(),
			begin() + initializedMoves);
		std::destroy(begin() + initializedMoves, end());

		m_Size = other.size();
		return *this;
	}

	~StaticVector()
	{
		clear();
	}

	StaticVector(const size_type count, const T& value)
		: m_Size{count}
	{
		if (count > N)
			throw CapacityExceededException{fmt::format(
				"Tried to construct a StaticVector with a size of {} but the capacity is only {}",
				count, N)};

		std::uninitialized_fill(begin(), end(), value);
	}

	StaticVector(const size_type count)
		: m_Size{count}
	{
		if (count > N)
			throw CapacityExceededException{fmt::format(
				"Tried to construct a StaticVector with a size of {} but the capacity is only {}",
				count, N)};

		std::uninitialized_default_construct(begin(), end());
	}

	StaticVector(const std::initializer_list<T> init)
		: m_Size{static_cast<size_type>(init.size())} // Will be tested below.
	{
		if (init.size() > N)
			throw CapacityExceededException{fmt::format(
				"Tried to construct a StaticVector with a size of {} but the capacity is only {}",
				init.size(), N)};

		std::uninitialized_copy(init.begin(), init.end(), begin());
	}

	StaticVector& operator=(const std::initializer_list<T> init)
	{
		if (init.size() > N)
			throw CapacityExceededException{fmt::format(
				"Tried to construct a StaticVector with a size of {} but the capacity is only {}",
				init.size(), N)};

		clear();
		std::uninitialized_copy(init.begin(), init.end(), begin());
		m_Size = init.size();
	}



	reference at(const size_type index)
	{
		if (index >= m_Size)
			throw std::out_of_range{fmt::format("Called at({}) but there are only {} elements.",
				index, size())};

		return (*this)[index];
	}

	const_reference at(const size_type index) const
	{
		if (index >= size())
			throw std::out_of_range{fmt::format("Called at({}) but there are only {} elements.",
				index, size())};

		return (*this)[index];
	}

	reference operator[](const size_type index) noexcept
	{
		ASSERT(index < size());
		return *(begin() + index);
	}

	const_reference operator[](const size_type index) const noexcept
	{
		ASSERT(index < size());
		return *(begin() + index);
	}

	reference front() noexcept
	{
		ASSERT(!empty());
		return *begin();
	}

	const_reference front() const noexcept
	{
		ASSERT(!empty());
		return *begin();
	}

	reference back() noexcept
	{
		ASSERT(!empty());
		return *std::prev(end());
	}

	const_reference back() const noexcept
	{
		ASSERT(!empty());
		return *std::prev(end());
	}

	pointer data() noexcept
	{
		return std::launder(reinterpret_cast<pointer>(m_Data.data()));
	}

	const_pointer data() const noexcept
	{
		return std::launder(reinterpret_cast<const_pointer>(m_Data.data()));
	}


	iterator begin() noexcept
	{
		return data();
	}

	const_iterator begin() const noexcept
	{
		return cbegin();
	}

	const_iterator cbegin() const noexcept
	{
		return data();
	}

	iterator end() noexcept
	{
		return begin() + size();
	}

	const_iterator end() const noexcept
	{
		return cend();
	}

	const_iterator cend() const noexcept
	{
		return cbegin() + size();
	}

	reverse_iterator rbegin() noexcept
	{
		return std::make_reverse_iterator(end());
	}

	const_reverse_iterator rbegin() const noexcept
	{
		return crbegin();
	}

	const_reverse_iterator crbegin() const noexcept
	{
		return std::make_reverse_iterator(end());
	}

	reverse_iterator rend() noexcept
	{
		return std::make_reverse_iterator(begin());
	}

	const_reverse_iterator rend() const noexcept
	{
		return crend();
	}

	const_reverse_iterator crend() const noexcept
	{
		return std::make_reverse_iterator(cbegin());
	}

	bool empty() const noexcept
	{
		return size() == 0;
	}

	bool full() const noexcept
	{
		return size() == N;
	}

	size_type size() const noexcept
	{
		return m_Size;
	}

	constexpr size_type capacity() const noexcept
	{
		return N;
	}


	void clear() noexcept
	{
		std::destroy(begin(), end());
		m_Size = 0;
	}

	/**
	 * Inserts an element at location. The elements which were in the range
	 * [ location, end() ) get moved no the next position.
	 *
	 * Exceptions:
	 * If an exception is thrown when inserting an element at the end this
	 * function has no effect (strong exception guarantee).
	 * Otherwise the program is in a valid state (Basic exception guarantee).
	 */
	iterator insert(const const_iterator location, const T& value)
	{
		if (full())
			throw CapacityExceededException{"Called insert but the StaticVector is already full"};

		if (location == end())
			return std::addressof(emplace_back(value));

		new(end()) T{std::move(back())};
		++m_Size;

		const iterator mutableLocation{MutableIter(location)};
		std::move_backward(mutableLocation, std::prev(end(), 2), std::prev(end(), 1));

		*mutableLocation = value;
		return mutableLocation;
	}

	/**
	 * Same as above but the new element is move-constructed.
	 *
	 * If an exception is thrown when inserting an element at the end this
	 * function has no effect (strong exception guarantee).
	 * If an exception is thrown the program is in a valid state
	 * (Basic exception guarantee).
	 */
	iterator insert(const const_iterator location, T&& value)
	{
		if (full())
			throw CapacityExceededException{"Called insert but the StaticVector is already full"};

		if (location == end())
			return std::addressof(emplace_back(std::move(value)));

		const iterator mutableLocation{MakeMutableIterator(location)};
		new(end()) T{std::move(back())};
		++m_Size;

		std::move_backward(mutableLocation, end() - 2, end() -1);

		*mutableLocation = std::move(value);
		return mutableLocation;
	}

	/**
	 * If an exception is thrown this function has no effect
	 * (strong exception guarantee).
	 */
	void push_back(const T& value)
	{
		emplace_back(value);
	}

	/**
	 * If an exception is thrown this function has no effect
	 * (strong exception guarantee).
	 */
	void push_back(T&& value)
	{
		emplace_back(std::move(value));
	}

	/**
	 * If an exception is thrown this function has no effect
	 * (strong exception guarantee).
	 */
	template<typename... Args>
	reference emplace_back(Args&&... args)
	{
		if (full())
			throw CapacityExceededException{
				"Called emplace_back but the StaticVector is already full"};

		const iterator location{begin() + size()};
		new(location) T{std::forward<Args>(args)...};
		++m_Size;
		return *location;
	}

	void pop_back() noexcept
	{
		ASSERT(!empty());
		std::destroy_at(std::addressof(back()));
		--m_Size;
	}

	/**
	 * Constructs or destructs elements to adjust to newSize. After this call
	 * the StaticVector contains newSize elements. Unlike std::vector the
	 * capacity does not get changed. If newSize is bigger then the capacity
	 * a CapacityExceededException is thrown.
	 *
	 * If newSize is smaller than size() (shrinking) no exception is thrown
	 * (Nothrow exception guarantee).
	 * If an exception is thrown this function has no effect.
	 * (strong exception guarantee)
	 */
	void resize(const size_type newSize)
	{
		if (newSize > N)
			throw CapacityExceededException{fmt::format(
				"Can not resize StaticVector to {} the capacity is {}", newSize, N)};

		if (newSize > size())
			std::uninitialized_default_construct(end(), begin() + newSize);
		else
			std::destroy(begin() + newSize, end());

		m_Size = newSize;
	}

	/**
	 * Same as above but uses value to copy-construct the new elements.
	 *
	 * If newSize is smaller than size() (shrinking) no exception is thrown
	 * (Nothrow exception guarantee).
	 * If an exception is thrown this function has no effect.
	 * (strong exception guarantee)
	 */
	void resize(const size_type newSize, const T& value)
	{
		if (newSize > N)
			throw CapacityExceededException{fmt::format(
				"Can't resize the StaticVector to {} the capacity is {}", newSize, N)};

		if (newSize > size())
			std::uninitialized_fill(end(), begin() + newSize, value);
		else
			std::destroy(begin() + newSize, end());

		m_Size = newSize;
	}

	template<size_t OtherN>
	friend bool operator==(const StaticVector<T, N>& lhs, const StaticVector<T, OtherN>& rhs)
	{
		return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
	}

	template<size_t OtherN>
	friend bool operator!=(const StaticVector<T, N>& lhs, const StaticVector<T, OtherN>& rhs)
	{
		return !(lhs == rhs);
	}

private:
	iterator MakeMutableIterator(const const_iterator iter) noexcept
	{
		return begin() + (iter - begin());
	}

	using EagerInitialized = std::array<T, N>;
	alignas(EagerInitialized) std::array<std::byte, sizeof(T) * N> m_Data;
	size_type m_Size{0};
};

} // namespace PS

#endif // INCLUDED_PS_STATICVECTOR
