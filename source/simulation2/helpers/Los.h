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

#ifndef INCLUDED_LOS
#define INCLUDED_LOS

// It doesn't seem worth moving the implementation to c++ and early-declaring Grid
// since files must include "Los.h" explicitly, and that's only done in .cpp files.
#include "Grid.h"

/**
 * Computing LOS data at a very high resolution is not necessary and quite slow.
 * This is the size, in meters, separating each LOS vertex.
 * (Note that this also means it is the minimal meaningful resolution of any vision range change).
 */
static constexpr i32 LOS_TILE_SIZE = 4;

enum class LosState : u8
{
	UNEXPLORED = 0,
	EXPLORED = 1,
	VISIBLE = 2,
	MASK = 3
};

/**
 * Object providing efficient abstracted access to the LOS state.
 * This depends on some implementation details of CCmpRangeManager.
 *
 * This *ignores* the GetLosRevealAll flag - callers should check that explicitly.
 */
class CLosQuerier
{
private:
	friend class CCmpRangeManager;
	friend class TestLOSTexture;

	CLosQuerier(u32 playerMask, const Grid<u32>& data, ssize_t verticesPerSide) :
	m_Data(data), m_PlayerMask(playerMask), m_VerticesPerSide(verticesPerSide)
	{
	}

	const CLosQuerier& operator=(const CLosQuerier&); // not implemented

public:
	/**
	 * Returns whether the given vertex is visible (i.e. is within a unit's LOS).
	 */
	inline bool IsVisible(ssize_t i, ssize_t j) const
	{
		if (!(i >= 0 && j >= 0 && i < m_VerticesPerSide && j < m_VerticesPerSide))
			return false;

		// Check high bit of each bit-pair
		if ((m_Data.get(i, j) & m_PlayerMask) & 0xAAAAAAAAu)
			return true;
		else
			return false;
	}

	/**
	 * Returns whether the given vertex is explored (i.e. was (or still is) within a unit's LOS).
	 */
	inline bool IsExplored(ssize_t i, ssize_t j) const
	{
		if (!(i >= 0 && j >= 0 && i < m_VerticesPerSide && j < m_VerticesPerSide))
			return false;

		// Check low bit of each bit-pair
		if ((m_Data.get(i, j) & m_PlayerMask) & 0x55555555u)
			return true;
		else
			return false;
	}

	/**
	 * Returns whether the given vertex is visible (i.e. is within a unit's LOS).
	 * i and j must be in the range [0, verticesPerSide), else behaviour is undefined.
	 */
	inline bool IsVisible_UncheckedRange(ssize_t i, ssize_t j) const
	{
#ifndef NDEBUG
		ENSURE(i >= 0 && j >= 0 && i < m_VerticesPerSide && j < m_VerticesPerSide);
#endif
		// Check high bit of each bit-pair
		if ((m_Data.get(i, j) & m_PlayerMask) & 0xAAAAAAAAu)
			return true;
		else
			return false;
	}

	/**
	 * Returns whether the given vertex is explored (i.e. was (or still is) within a unit's LOS).
	 * i and j must be in the range [0, verticesPerSide), else behaviour is undefined.
	 */
	inline bool IsExplored_UncheckedRange(ssize_t i, ssize_t j) const
	{
#ifndef NDEBUG
		ENSURE(i >= 0 && j >= 0 && i < m_VerticesPerSide && j < m_VerticesPerSide);
#endif
		// Check low bit of each bit-pair
		if ((m_Data.get(i, j) & m_PlayerMask) & 0x55555555u)
			return true;
		else
			return false;
	}

private:
	u32 m_PlayerMask;
	const Grid<u32>& m_Data;
	ssize_t m_VerticesPerSide;
};

#endif // INCLUDED_LOS
