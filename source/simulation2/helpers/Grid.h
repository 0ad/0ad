/* Copyright (C) 2020 Wildfire Games.
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

#ifndef INCLUDED_GRID
#define INCLUDED_GRID

#include "simulation2/serialization/SerializeTemplates.h"

#include <cstring>

#ifdef NDEBUG
#define GRID_BOUNDS_DEBUG 0
#else
#define GRID_BOUNDS_DEBUG 1
#endif

/**
 * Basic 2D array, intended for storing tile data, plus support for lazy updates
 * by ICmpObstructionManager.
 * @c T must be a POD type that can be initialised with 0s.
 */
template<typename T>
class Grid
{
	friend struct SerializeHelper<Grid<T>>;
protected:
	// Tag-dispatching internal utilities for convenience.
	struct default_type{};
	struct is_pod { operator default_type() { return default_type{}; }};
	struct is_container { operator default_type() { return default_type{}; }};

	// helper to detect value_type
	template <typename U, typename = int> struct has_value_type : std::false_type { };
	template <typename U> struct has_value_type <U, decltype(std::declval<typename U::value_type>(), 0)> : std::true_type { };

	template <typename U, typename A, typename B> using if_ = typename std::conditional<U::value, A, B>::type;

	template<typename U>
	using dispatch = if_< std::is_pod<U>, is_pod,
						  if_<has_value_type<U>, is_container,
						default_type>>;

public:
	Grid() : m_W(0), m_H(0), m_Data(NULL)
	{
	}

	Grid(u16 w, u16 h) : m_W(w), m_H(h), m_Data(NULL)
	{
		resize(w, h);
	}

	Grid(const Grid& g) : m_W(0), m_H(0), m_Data(NULL)
	{
		*this = g;
	}

	using value_type = T;
public:

	// Ensure that o and this are the same size before calling.
	void copy_data(T* o, default_type) { std::copy(o, o + m_H*m_W, &m_Data[0]); }
	void copy_data(T* o, is_pod) { memcpy(m_Data, o, m_W*m_H*sizeof(T)); }

	Grid& operator=(const Grid& g)
	{
		if (this == &g)
			return *this;

		if (m_W == g.m_W && m_H == g.m_H)
		{
			copy_data(g.m_Data, dispatch<T>{});
			return *this;
		}

		m_W = g.m_W;
		m_H = g.m_H;
		SAFE_ARRAY_DELETE(m_Data);
		if (g.m_Data)
		{
			m_Data = new T[m_W * m_H];
			copy_data(g.m_Data, dispatch<T>{});
		}
		return *this;
	}

	void swap(Grid& g)
	{
		std::swap(m_Data, g.m_Data);
		std::swap(m_H, g.m_H);
		std::swap(m_W, g.m_W);
	}

	~Grid()
	{
		SAFE_ARRAY_DELETE(m_Data);
	}

	// Ensure that o and this are the same size before calling.
	bool compare_data(T* o, default_type) const { return std::equal(&m_Data[0], &m_Data[m_W*m_H], o); }
	bool compare_data(T* o, is_pod) const { return memcmp(m_Data, o, m_W*m_H*sizeof(T)) == 0; }

	bool operator==(const Grid& g) const
	{
		if (!compare_sizes(&g))
			return false;

		return compare_data(g.m_Data, dispatch<T>{});
	}
	bool operator!=(const Grid& g) const { return !(*this==g); }

	bool blank() const
	{
		return m_W == 0 && m_H == 0;
	}

	u16 width() const { return m_W; };
	u16 height() const { return m_H; };


	bool _any_set_in_square(int, int, int, int, default_type) const
	{
		static_assert(!std::is_same<T, T>::value, "Not implemented.");
		return false; // Fix warnings.
	}
	bool _any_set_in_square(int i0, int j0, int i1, int j1, is_pod) const
	{
#if GRID_BOUNDS_DEBUG
		ENSURE(i0 >= 0 && j0 >= 0 && i1 <= m_W && j1 <= m_H);
#endif
		for (int j = j0; j < j1; ++j)
		{
			int sum = 0;
			for (int i = i0; i < i1; ++i)
				sum += m_Data[j*m_W + i];
			if (sum > 0)
				return true;
		}
		return false;
	}

	bool any_set_in_square(int i0, int j0, int i1, int j1) const
	{
		return _any_set_in_square(i0, j0, i1, j1, dispatch<T>{});
	}

	void reset_data(default_type) { std::fill(&m_Data[0], &m_Data[m_H*m_W], T{}); }
	void reset_data(is_pod) { memset(m_Data, 0, m_W*m_H*sizeof(T)); }

	// Reset the data to its default-constructed value (usually 0), not changing size.
	void reset()
	{
		if (m_Data)
			reset_data(dispatch<T>{});
	}

	// Clear the grid setting the size to 0 and freeing any data.
	void clear()
	{
		resize(0, 0);
	}

	void resize(u16 w, u16 h)
	{
		SAFE_ARRAY_DELETE(m_Data);
		m_W = w;
		m_H = h;

		if (!m_W && !m_H)
			return;

		m_Data = new T[m_W * m_H];
		ENSURE(m_Data);
		reset();
	}

	// Add two grids of the same size
	void add(const Grid& g)
	{
#if GRID_BOUNDS_DEBUG
		ENSURE(g.m_W == m_W && g.m_H == m_H);
#endif
		for (int i=0; i < m_H*m_W; ++i)
			m_Data[i] += g.m_Data[i];
	}

	void bitwise_or(const Grid& g)
	{
		if (this == &g)
			return;

#if GRID_BOUNDS_DEBUG
		ENSURE(g.m_W == m_W && g.m_H == m_H);
#endif
		for (int i = 0; i < m_H*m_W; ++i)
			m_Data[i] |= g.m_Data[i];
	}

	void set(int i, int j, const T& value)
	{
#if GRID_BOUNDS_DEBUG
		ENSURE(0 <= i && i < m_W && 0 <= j && j < m_H);
#endif
		m_Data[j*m_W + i] = value;
	}

	T& operator[](std::pair<u16, u16> coords) { return get(coords.first, coords.second); }
	T& get(std::pair<u16, u16> coords) { return get(coords.first, coords.second); }

	T& operator[](std::pair<u16, u16> coords) const { return get(coords.first, coords.second); }
	T& get(std::pair<u16, u16> coords) const { return get(coords.first, coords.second); }

	T& get(int i, int j)
	{
#if GRID_BOUNDS_DEBUG
		ENSURE(0 <= i && i < m_W && 0 <= j && j < m_H);
#endif
		return m_Data[j*m_W + i];
	}

	T& get(int i, int j) const
	{
#if GRID_BOUNDS_DEBUG
		ENSURE(0 <= i && i < m_W && 0 <= j && j < m_H);
#endif
		return m_Data[j*m_W + i];
	}

	template<typename U>
	bool compare_sizes(const Grid<U>* g) const
	{
		return g && m_W == g->m_W && m_H == g->m_H;
	}

	u16 m_W, m_H;
	T* m_Data;
};


/**
 * Serialize a grid, applying a simple RLE compression that is assumed efficient.
 */
template<typename T>
struct SerializeHelper<Grid<T>>
{
	void operator()(ISerializer& serialize, const char* name, Grid<T>& value)
	{
		size_t len = value.m_H * value.m_W;
		serialize.NumberU16_Unbounded("width", value.m_W);
		serialize.NumberU16_Unbounded("height", value.m_H);
		if (len == 0)
			return;
		u32 count = 1;
		T prevVal = value.m_Data[0];
		for (size_t i = 1; i < len; ++i)
		{
			if (prevVal == value.m_Data[i])
			{
				count++;
				continue;
			}
			serialize.NumberU32_Unbounded("#", count);
			Serializer(serialize, name, prevVal);
			count = 1;
			prevVal = value.m_Data[i];
		}
		serialize.NumberU32_Unbounded("#", count);
		Serializer(serialize, name, prevVal);
	}

	void operator()(IDeserializer& deserialize, const char* name, Grid<T>& value)
	{
		u16 w, h;
		deserialize.NumberU16_Unbounded("width", w);
		deserialize.NumberU16_Unbounded("height", h);
		u32 len = h * w;
		value.resize(w, h);
		for (size_t i = 0; i < len;)
		{
			u32 count;
			deserialize.NumberU32_Unbounded("#", count);
			T el;
			Serializer(deserialize, name, el);
			std::fill(&value.m_Data[i], &value.m_Data[i+count], el);
			i += count;
		}
	}
};


/**
 * Similar to Grid, except optimised for sparse usage (the grid is subdivided into
 * buckets whose contents are only initialised on demand, to save on memset cost).
 */
template<typename T>
class SparseGrid
{
	NONCOPYABLE(SparseGrid);

	enum { BucketBits = 4, BucketSize = 1 << BucketBits };

	T* GetBucket(int i, int j)
	{
		size_t b = (j >> BucketBits) * m_BW + (i >> BucketBits);
		if (!m_Data[b])
		{
			m_Data[b] = new T[BucketSize*BucketSize]();
		}
		return m_Data[b];
	}

public:
	SparseGrid(u16 w, u16 h) : m_W(w), m_H(h), m_DirtyID(0)
	{
		ENSURE(m_W && m_H);

		m_BW = (u16)((m_W + BucketSize-1) >> BucketBits);
		m_BH = (u16)((m_H + BucketSize-1) >> BucketBits);

		m_Data = new T*[m_BW*m_BH]();
	}

	~SparseGrid()
	{
		reset();
		SAFE_ARRAY_DELETE(m_Data);
	}

	void reset()
	{
		for (size_t i = 0; i < (size_t)(m_BW*m_BH); ++i)
			SAFE_ARRAY_DELETE(m_Data[i]);

		// Reset m_Data by value-constructing in place with placement new.
		m_Data = new (m_Data) T*[m_BW*m_BH]();
	}

	void set(int i, int j, const T& value)
	{
#if GRID_BOUNDS_DEBUG
		ENSURE(0 <= i && i < m_W && 0 <= j && j < m_H);
#endif
		GetBucket(i, j)[(j % BucketSize)*BucketSize + (i % BucketSize)] = value;
	}

	T& get(int i, int j)
	{
#if GRID_BOUNDS_DEBUG
		ENSURE(0 <= i && i < m_W && 0 <= j && j < m_H);
#endif
		return GetBucket(i, j)[(j % BucketSize)*BucketSize + (i % BucketSize)];
	}

	u16 m_W, m_H;
	u16 m_BW, m_BH;
	T** m_Data;

	size_t m_DirtyID; // if this is < the id maintained by ICmpObstructionManager then it needs to be updated
};

/**
 * Structure holding grid dirtiness informations, for clever updates.
 */
struct GridUpdateInformation
{
	bool dirty;
	bool globallyDirty;
	Grid<u8> dirtinessGrid;

	/**
	 * Update the information with additionnal needed updates, then erase the source of additions.
	 * This can usually be optimized through a careful memory management.
	 */
	void MergeAndClear(GridUpdateInformation& b)
	{
		ENSURE(dirtinessGrid.compare_sizes(&b.dirtinessGrid));

		bool wasDirty = dirty;

		dirty |= b.dirty;
		globallyDirty |= b.globallyDirty;

		// If the current grid is useless, swap it
		if (!wasDirty)
			dirtinessGrid.swap(b.dirtinessGrid);
		// If the new grid isn't used, don't bother updating it
		else if (dirty && !globallyDirty)
			dirtinessGrid.bitwise_or(b.dirtinessGrid);

		b.Clean();
	}

	/**
	 * Mark everything as clean
	 */
	void Clean()
	{
		dirty = false;
		globallyDirty = false;
		dirtinessGrid.reset();
	}
};

#endif // INCLUDED_GRID
