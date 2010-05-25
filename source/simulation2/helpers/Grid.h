/* Copyright (C) 2010 Wildfire Games.
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
	Grid(u16 w, u16 h) : m_W(w), m_H(h), m_DirtyID(0)
	{
		m_Data = new T[m_W * m_H];
		reset();
	}

	~Grid()
	{
		delete[] m_Data;
	}

	void reset()
	{
		memset(m_Data, 0, m_W*m_H*sizeof(T));
	}

	void set(size_t i, size_t j, const T& value)
	{
#if GRID_BOUNDS_DEBUG
		debug_assert(i < m_W && j < m_H);
#endif
		m_Data[j*m_W + i] = value;
	}

	T& get(size_t i, size_t j)
	{
#if GRID_BOUNDS_DEBUG
		debug_assert(i < m_W && j < m_H);
#endif
		return m_Data[j*m_W + i];
	}

	u16 m_W, m_H;
	T* m_Data;

	size_t m_DirtyID; // if this is < the id maintained by ICmpObstructionManager then it needs to be updated
};

#endif // INCLUDED_GRID
