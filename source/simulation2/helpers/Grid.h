/* Copyright (C) 2017 Wildfire Games.
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
public:
	Grid() : m_W(0), m_H(0), m_Data(NULL), m_DirtyID(0)
	{
	}

	Grid(u16 w, u16 h) : m_W(w), m_H(h), m_Data(NULL), m_DirtyID(0)
	{
		if (m_W || m_H)
			m_Data = new T[m_W * m_H];
		reset();
	}

	Grid(const Grid& g)
	{
		m_Data = NULL;
		*this = g;
	}

	Grid& operator=(const Grid& g)
	{
		if (this == &g)
			return *this;

		m_DirtyID = g.m_DirtyID;
		if (m_W == g.m_W && m_H == g.m_H)
		{
			memcpy(m_Data, g.m_Data, m_W*m_H*sizeof(T));
			return *this;
		}

		m_W = g.m_W;
		m_H = g.m_H;
		delete[] m_Data;
		if (g.m_Data)
		{
			m_Data = new T[m_W * m_H];
			memcpy(m_Data, g.m_Data, m_W*m_H*sizeof(T));
		}
		else
			m_Data = NULL;
		return *this;
	}

	void swap(Grid& g)
	{
		std::swap(m_DirtyID, g.m_DirtyID);
		std::swap(m_Data, g.m_Data);
		std::swap(m_H, g.m_H);
		std::swap(m_W, g.m_W);
	}

	~Grid()
	{
		delete[] m_Data;
	}

	void reset()
	{
		if (m_Data)
			memset(m_Data, 0, m_W*m_H*sizeof(T));
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

	void set(int i, int j, const T& value)
	{
#if GRID_BOUNDS_DEBUG
		ENSURE(0 <= i && i < m_W && 0 <= j && j < m_H);
#endif
		m_Data[j*m_W + i] = value;
	}

	T& get(int i, int j) const
	{
#if GRID_BOUNDS_DEBUG
		ENSURE(0 <= i && i < m_W && 0 <= j && j < m_H);
#endif
		return m_Data[j*m_W + i];
	}

	u16 m_W, m_H;
	T* m_Data;

	size_t m_DirtyID; // if this is < the id maintained by ICmpObstructionManager then it needs to be updated
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
			m_Data[b] = new T[BucketSize*BucketSize];
			memset(m_Data[b], 0, BucketSize*BucketSize*sizeof(T));
		}
		return m_Data[b];
	}

public:
	SparseGrid(u16 w, u16 h) : m_W(w), m_H(h), m_DirtyID(0)
	{
		ENSURE(m_W && m_H);

		m_BW = (u16)((m_W + BucketSize-1) >> BucketBits);
		m_BH = (u16)((m_H + BucketSize-1) >> BucketBits);

		m_Data = new T*[m_BW*m_BH];
		memset(m_Data, 0, m_BW*m_BH*sizeof(T*));
	}

	~SparseGrid()
	{
		reset();
		delete[] m_Data;
	}

	void reset()
	{
		for (size_t i = 0; i < (size_t)(m_BW*m_BH); ++i)
			delete[] m_Data[i];

		memset(m_Data, 0, m_BW*m_BH*sizeof(T*));
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
 * Structure holding grid dirtiness informations, for clever updates
 *
 * Note: globallyDirty should be used to know which parts of the grid have been changed after an update,
 * whereas globalRecompute should be used during the update itself, to avoid unnecessary recomputations.
 */
struct GridUpdateInformation
{
	bool dirty;
	bool globallyDirty;
	bool globalRecompute;
	Grid<u8> dirtinessGrid;

	void swap(GridUpdateInformation& b)
	{
		std::swap(dirty, b.dirty);
		std::swap(globallyDirty, b.globallyDirty);
		std::swap(globalRecompute, b.globalRecompute);
		dirtinessGrid.swap(b.dirtinessGrid);
	}

	/**
	 * Mark everything as clean
	 */
	void Clean()
	{
		dirty = false;
		globallyDirty = false;
		globalRecompute = false;
		dirtinessGrid.reset();
	}
};

#endif // INCLUDED_GRID
