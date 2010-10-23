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

#ifndef INCLUDED_SPATIAL
#define INCLUDED_SPATIAL

#include "simulation2/serialization/SerializeTemplates.h"

/**
 * A very basic subdivision scheme for finding items in ranges.
 * Items are stored in lists in fixed-size divisions.
 * Items have a size (min/max values of their axis-aligned bounding box)
 * and are stored in all divisions overlapping that area.
 *
 * It is the caller's responsibility to ensure items are only added
 * once, aren't removed unless they've been added, etc, and that
 * Move/Remove are called with the same coordinates originally passed
 * to Add (since this class doesn't remember which divisions an item
 * occupies).
 *
 * (TODO: maybe an adaptive quadtree would be better than fixed sizes?)
 */
template<typename T>
class SpatialSubdivision
{
public:
	SpatialSubdivision() :
		m_DivisionsW(0), m_DivisionsH(0)
	{
	}

	void Reset(entity_pos_t maxX, entity_pos_t maxZ, entity_pos_t divisionSize)
	{
		m_DivisionSize = divisionSize;
		m_DivisionsW = (maxX / m_DivisionSize).ToInt_RoundToInfinity();
		m_DivisionsH = (maxZ / m_DivisionSize).ToInt_RoundToInfinity();
		m_Divisions.clear();
		m_Divisions.resize(m_DivisionsW * m_DivisionsH);
	}

	/**
	 * Add an item with the given 'to' size.
	 * The item must not already be present.
	 */
	void Add(T item, CFixedVector2D toMin, CFixedVector2D toMax)
	{
		debug_assert(toMin.X <= toMax.X && toMin.Y <= toMax.Y);

		size_t i0 = GetI0(toMin.X);
		size_t j0 = GetJ0(toMin.Y);
		size_t i1 = GetI1(toMax.X);
		size_t j1 = GetJ1(toMax.Y);
		for (size_t j = j0; j <= j1; ++j)
		{
			for (size_t i = i0; i <= i1; ++i)
			{
				std::vector<T>& div = m_Divisions.at(i + j*m_DivisionsW);
				div.push_back(item);
			}
		}
	}

	/**
	 * Remove an item with the given 'from' size.
	 * The item should already be present.
	 * The size must match the size that was last used when adding the item.
	 */
	void Remove(T item, CFixedVector2D fromMin, CFixedVector2D fromMax)
	{
		debug_assert(fromMin.X <= fromMax.X && fromMin.Y <= fromMax.Y);

		size_t i0 = GetI0(fromMin.X);
		size_t j0 = GetJ0(fromMin.Y);
		size_t i1 = GetI1(fromMax.X);
		size_t j1 = GetJ1(fromMax.Y);
		for (size_t j = j0; j <= j1; ++j)
		{
			for (size_t i = i0; i <= i1; ++i)
			{
				std::vector<T>& div = m_Divisions.at(i + j*m_DivisionsW);

				for (size_t n = 0; n < div.size(); ++n)
				{
					if (div[n] == item)
					{
						// Delete by swapping with the last element then popping
						div[n] = div.back();
						div.pop_back();
						break;
					}
				}
			}
		}
	}

	/**
	 * Equivalent to Remove() then Add(), but potentially faster.
	 */
	void Move(T item, CFixedVector2D fromMin, CFixedVector2D fromMax, CFixedVector2D toMin, CFixedVector2D toMax)
	{
		// Skip the work if we're staying in the same divisions
		if (GetIndex0(fromMin) == GetIndex0(toMin) && GetIndex1(fromMax) == GetIndex1(toMax))
			return;

		Remove(item, fromMin, fromMax);
		Add(item, toMin, toMax);
	}

	/**
	 * Convenience function for Add() of individual points.
	 * (Note that points on a boundary may occupy multiple divisions.)
	 */
	void Add(T item, CFixedVector2D to)
	{
		Add(item, to, to);
	}

	/**
	 * Convenience function for Remove() of individual points.
	 */
	void Remove(T item, CFixedVector2D from)
	{
		Remove(item, from, from);
	}

	/**
	 * Convenience function for Move() of individual points.
	 */
	void Move(T item, CFixedVector2D from, CFixedVector2D to)
	{
		Move(item, from, from, to, to);
	}

	/**
	 * Returns a sorted list of unique items that includes all items
	 * within the given axis-aligned square range.
	 */
	std::vector<T> GetInRange(CFixedVector2D posMin, CFixedVector2D posMax)
	{
		std::vector<T> ret;

		debug_assert(posMin.X <= posMax.X && posMin.Y <= posMax.Y);

		size_t i0 = GetI0(posMin.X);
		size_t j0 = GetJ0(posMin.Y);
		size_t i1 = GetI1(posMax.X);
		size_t j1 = GetJ1(posMax.Y);
		for (size_t j = j0; j <= j1; ++j)
		{
			for (size_t i = i0; i <= i1; ++i)
			{
				std::vector<T>& div = m_Divisions.at(i + j*m_DivisionsW);
				ret.insert(ret.end(), div.begin(), div.end());
			}
		}

		// Remove duplicates
		std::sort(ret.begin(), ret.end());
		ret.erase(std::unique(ret.begin(), ret.end()), ret.end());

		return ret;
	}

	/**
	 * Returns a sorted list of unique items that includes all items
	 * within the given circular distance of the given point.
	 */
	std::vector<T> GetNear(CFixedVector2D pos, entity_pos_t range)
	{
		// TODO: be cleverer and return a circular pattern of divisions,
		// not this square over-approximation

		return GetInRange(pos - CFixedVector2D(range, range), pos + CFixedVector2D(range, range));
	}

private:
	// Helper functions for translating coordinates into division indexes
	// (avoiding out-of-bounds accesses, and rounding correctly so that
	// points precisely between divisions are counted in both):

	size_t GetI0(entity_pos_t x)
	{
		return Clamp((x / m_DivisionSize).ToInt_RoundToInfinity()-1, 0, (int)m_DivisionsW-1);
	}

	size_t GetJ0(entity_pos_t z)
	{
		return Clamp((z / m_DivisionSize).ToInt_RoundToInfinity()-1, 0, (int)m_DivisionsH-1);
	}

	size_t GetI1(entity_pos_t x)
	{
		return Clamp((x / m_DivisionSize).ToInt_RoundToNegInfinity(), 0, (int)m_DivisionsW-1);
	}

	size_t GetJ1(entity_pos_t z)
	{
		return Clamp((z / m_DivisionSize).ToInt_RoundToNegInfinity(), 0, (int)m_DivisionsH-1);
	}

	size_t GetIndex0(CFixedVector2D pos)
	{
		return GetI0(pos.X) + GetJ0(pos.Y)*m_DivisionsW;
	}

	size_t GetIndex1(CFixedVector2D pos)
	{
		return GetI1(pos.X) + GetJ1(pos.Y)*m_DivisionsW;
	}

	entity_pos_t m_DivisionSize;
	std::vector<std::vector<T> > m_Divisions;
	size_t m_DivisionsW;
	size_t m_DivisionsH;

	template<typename ELEM> friend struct SerializeSpatialSubdivision;
};

/**
 * Serialization helper template for SpatialSubdivision
 */
template<typename ELEM>
struct SerializeSpatialSubdivision
{
	template<typename T>
	void operator()(ISerializer& serialize, const char* UNUSED(name), SpatialSubdivision<T>& value)
	{
		serialize.NumberFixed_Unbounded("div size", value.m_DivisionSize);
		SerializeVector<SerializeVector<ELEM> >()(serialize, "divs", value.m_Divisions);
		serialize.NumberU32_Unbounded("divs w", value.m_DivisionsW);
		serialize.NumberU32_Unbounded("divs h", value.m_DivisionsH);
	}

	template<typename T>
	void operator()(IDeserializer& serialize, const char* UNUSED(name), SpatialSubdivision<T>& value)
	{
		serialize.NumberFixed_Unbounded("div size", value.m_DivisionSize);
		SerializeVector<SerializeVector<ELEM> >()(serialize, "divs", value.m_Divisions);

		u32 w, h;
		serialize.NumberU32_Unbounded("divs w", w);
		serialize.NumberU32_Unbounded("divs h", h);
		value.m_DivisionsW = w;
		value.m_DivisionsH = h;
	}
};

#endif // INCLUDED_SPATIAL
