/* Copyright (C) 2023 Wildfire Games.
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

#ifndef INCLUDED_PS_SPAN
#define INCLUDED_PS_SPAN

#include <cstddef>
#include <iterator>
#include <type_traits>

namespace PS
{

/**
 * Simplifed version of std::span (C++20) as we don't support the original one
 * yet. The naming intentionally follows the STL version to make the future
 * replacement easier with less blame changing.
 * It supports only very basic subset of std::span functionality.
 * TODO: remove as soon as std::span become available.
 */
template<typename T>
class span
{
public:
	using element_type = T;
	using value_type = std::remove_cv_t<T>;
	using size_type = size_t;
	using pointer = T*;
	using reference = T&;
	using iterator = pointer;

	constexpr span()
		: m_Pointer(nullptr), m_Extent(0) {}

	constexpr span(iterator first, size_type extent)
		: m_Pointer(first), m_Extent(extent) {}

	constexpr span(iterator first, iterator last)
		: m_Pointer(first), m_Extent(static_cast<size_type>(last - first)) {}

	template<typename OtherT, size_t N>
	constexpr span(const std::array<OtherT, N>& arr)
		: m_Pointer(arr.data()), m_Extent(arr.size()) {}

	template<typename ContinuousRange>
	constexpr span(ContinuousRange& range)
		: m_Pointer(range.data()), m_Extent(range.size()) {}

	constexpr span(const span& other) = default;

	constexpr span& operator=(const span& other) = default;

	~span() = default;

	constexpr size_type size() const { return m_Extent; }
	constexpr bool empty() const { return size() == 0; }
	constexpr reference operator[](size_type index) const { return *(m_Pointer + index); }
	constexpr pointer data() const { return m_Pointer; }

	constexpr iterator begin() const { return m_Pointer; }
	constexpr iterator end() const { return m_Pointer + m_Extent; }

	constexpr span subspan(size_type offset) const { return {m_Pointer + offset, m_Extent - offset}; }

private:
	pointer m_Pointer;
	size_type m_Extent;
};

template<typename T, size_t N>
span(const std::array<T, N>&) -> span<const T>;

} // namespace PS

#endif // INCLUDED_PS_SPAN
