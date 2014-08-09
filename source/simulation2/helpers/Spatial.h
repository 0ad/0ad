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

#include "simulation2/system/Component.h"
#include "simulation2/serialization/SerializeTemplates.h"

/**
 * A very basic subdivision scheme for finding items in ranges.
 * Items are stored in lists in dynamic-sized divisions.
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
class SpatialSubdivision
{
	struct SubDivisionGrid
	{
		std::vector<uint32_t> items;

		inline void push_back(uint32_t value)
		{
			items.push_back(value);
		}

		inline void erase(int index)
		{
			// Delete by swapping with the last element then popping
			if ((int)items.size() > 1) // but only if we have more than 1 elements
				items[index] = items.back();
			items.pop_back();
		}

		void copy_items_at_end(std::vector<uint32_t>& out)
		{
			out.insert(out.end(), items.begin(), items.end());
		}
	};


	entity_pos_t m_DivisionSize;
	SubDivisionGrid* m_Divisions;
	uint32_t m_DivisionsW;
	uint32_t m_DivisionsH;

	friend struct SerializeSubDivisionGrid;
	friend struct SerializeSpatialSubdivision;

public:
	SpatialSubdivision() : m_Divisions(NULL), m_DivisionsW(0), m_DivisionsH(0)
	{
	}
	~SpatialSubdivision()
	{
		delete[] m_Divisions;
	}
	SpatialSubdivision(const SpatialSubdivision& rhs)
	{
		m_DivisionSize = rhs.m_DivisionSize;
		m_DivisionsW = rhs.m_DivisionsW;
		m_DivisionsH = rhs.m_DivisionsH;
		size_t n = m_DivisionsW * m_DivisionsH;
		m_Divisions = new SubDivisionGrid[n];
		for (size_t i = 0; i < n; ++i)
			m_Divisions[i] = rhs.m_Divisions[i]; // just fall back to piecemeal copy
	}
	SpatialSubdivision& operator=(const SpatialSubdivision& rhs)
	{
		if (this != &rhs)
		{
			m_DivisionSize = rhs.m_DivisionSize;
			size_t n = rhs.m_DivisionsW * rhs.m_DivisionsH;
			if (m_DivisionsW != rhs.m_DivisionsW || m_DivisionsH != rhs.m_DivisionsH)
				Create(n); // size changed, recreate
			
			m_DivisionsW = rhs.m_DivisionsW;
			m_DivisionsH = rhs.m_DivisionsH;
			for (size_t i = 0; i < n; ++i)
				m_Divisions[i] = rhs.m_Divisions[i]; // just fall back to piecemeal copy
		}
		return *this;
	}

	inline entity_pos_t GetDivisionSize() const { return m_DivisionSize; }
	inline uint32_t GetWidth() const { return m_DivisionsW; }
	inline uint32_t GetHeight() const { return m_DivisionsH; }

	void Create(size_t count)
	{
		if (m_Divisions) delete[] m_Divisions;
		m_Divisions = new SubDivisionGrid[count];
	}

	/**
	 * Equivalence test (ignoring order of items within each subdivision)
	 */
	bool operator==(const SpatialSubdivision& rhs)
	{
		if (m_DivisionSize != rhs.m_DivisionSize || m_DivisionsW != rhs.m_DivisionsW || m_DivisionsH != rhs.m_DivisionsH)
			return false;
		
		uint32_t n = m_DivisionsH * m_DivisionsW;
		for (uint32_t i = 0; i < n; ++i)
		{
			if (m_Divisions[i].items.size() != rhs.m_Divisions[i].items.size())
				return false;

			// don't bother optimizing this, this is only used in the TESTING SUITE
			std::vector<uint32_t> a = m_Divisions[i].items;
			std::vector<uint32_t> b = rhs.m_Divisions[i].items;
			std::sort(a.begin(), a.end());
			std::sort(b.begin(), b.end());
			if (a != b)
				return false;
		}
		return true;
	}

	inline bool operator!=(const SpatialSubdivision& rhs)
	{
		return !(*this == rhs);
	}

	void Reset(entity_pos_t maxX, entity_pos_t maxZ, entity_pos_t divisionSize)
	{
		m_DivisionSize = divisionSize;
		m_DivisionsW = (maxX / m_DivisionSize).ToInt_RoundToInfinity();
		m_DivisionsH = (maxZ / m_DivisionSize).ToInt_RoundToInfinity();

		Create(m_DivisionsW * m_DivisionsH);
	}


	/**
	 * Add an item with the given 'to' size.
	 * The item must not already be present.
	 */
	void Add(uint32_t item, CFixedVector2D toMin, CFixedVector2D toMax)
	{
		ENSURE(toMin.X <= toMax.X && toMin.Y <= toMax.Y);

		u32 i0 = GetI0(toMin.X);
		u32 j0 = GetJ0(toMin.Y);
		u32 i1 = GetI1(toMax.X);
		u32 j1 = GetJ1(toMax.Y);
		for (u32 j = j0; j <= j1; ++j)
		{
			for (u32 i = i0; i <= i1; ++i)
			{
				m_Divisions[i + j*m_DivisionsW].push_back(item);
			}
		}
	}

	/**
	 * Remove an item with the given 'from' size.
	 * The item should already be present.
	 * The size must match the size that was last used when adding the item.
	 */
	void Remove(uint32_t item, CFixedVector2D fromMin, CFixedVector2D fromMax)
	{
		ENSURE(fromMin.X <= fromMax.X && fromMin.Y <= fromMax.Y);

		u32 i0 = GetI0(fromMin.X);
		u32 j0 = GetJ0(fromMin.Y);
		u32 i1 = GetI1(fromMax.X);
		u32 j1 = GetJ1(fromMax.Y);
		for (u32 j = j0; j <= j1; ++j)
		{
			for (u32 i = i0; i <= i1; ++i)
			{
				SubDivisionGrid& div = m_Divisions[i + j*m_DivisionsW];
				int size = div.items.size();
				for (int n = 0; n < size; ++n)
				{
					if (div.items[n] == item)
					{
						div.erase(n);
						break;
					}
				}
			}
		}
	}

	/**
	 * Equivalent to Remove() then Add(), but potentially faster.
	 */
	void Move(uint32_t item, CFixedVector2D fromMin, CFixedVector2D fromMax, CFixedVector2D toMin, CFixedVector2D toMax)
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
	void Add(uint32_t item, CFixedVector2D to)
	{
		Add(item, to, to);
	}

	/**
	 * Convenience function for Remove() of individual points.
	 */
	void Remove(uint32_t item, CFixedVector2D from)
	{
		Remove(item, from, from);
	}

	/**
	 * Convenience function for Move() of individual points.
	 */
	void Move(uint32_t item, CFixedVector2D from, CFixedVector2D to)
	{
		Move(item, from, from, to, to);
	}

	/**
	 * Returns a sorted list of unique items that includes all items
	 * within the given axis-aligned square range.
	 */
	void GetInRange(std::vector<uint32_t>& out, CFixedVector2D posMin, CFixedVector2D posMax)
	{
		out.clear();
		ENSURE(posMin.X <= posMax.X && posMin.Y <= posMax.Y);

		u32 i0 = GetI0(posMin.X);
		u32 j0 = GetJ0(posMin.Y);
		u32 i1 = GetI1(posMax.X);
		u32 j1 = GetJ1(posMax.Y);
		for (u32 j = j0; j <= j1; ++j)
		{
			for (u32 i = i0; i <= i1; ++i)
			{
				m_Divisions[i + j*m_DivisionsW].copy_items_at_end(out);
			}
		}
		// some buildings span several tiles, so we need to make it unique
		std::sort(out.begin(), out.end());
		out.erase(std::unique(out.begin(), out.end()), out.end());
	}

	/**
	 * Returns a sorted list of unique items that includes all items
	 * within the given circular distance of the given point.
	 */
	void GetNear(std::vector<uint32_t>& out, CFixedVector2D pos, entity_pos_t range)
	{
		// TODO: be cleverer and return a circular pattern of divisions,
		// not this square over-approximation
		CFixedVector2D r(range, range);
		GetInRange(out, pos - r, pos + r);
	}

private:
	// Helper functions for translating coordinates into division indexes
	// (avoiding out-of-bounds accesses, and rounding correctly so that
	// points precisely between divisions are counted in both):

	uint32_t GetI0(entity_pos_t x)
	{
		return Clamp((x / m_DivisionSize).ToInt_RoundToInfinity()-1, 0, (int)m_DivisionsW-1);
	}

	uint32_t GetJ0(entity_pos_t z)
	{
		return Clamp((z / m_DivisionSize).ToInt_RoundToInfinity()-1, 0, (int)m_DivisionsH-1);
	}

	uint32_t GetI1(entity_pos_t x)
	{
		return Clamp((x / m_DivisionSize).ToInt_RoundToNegInfinity(), 0, (int)m_DivisionsW-1);
	}

	uint32_t GetJ1(entity_pos_t z)
	{
		return Clamp((z / m_DivisionSize).ToInt_RoundToNegInfinity(), 0, (int)m_DivisionsH-1);
	}

	uint32_t GetIndex0(CFixedVector2D pos)
	{
		return GetI0(pos.X) + GetJ0(pos.Y)*m_DivisionsW;
	}

	uint32_t GetIndex1(CFixedVector2D pos)
	{
		return GetI1(pos.X) + GetJ1(pos.Y)*m_DivisionsW;
	}
};

/**
 * Serialization helper template for SpatialSubdivision
 */
struct SerializeSpatialSubdivision
{
	void operator()(ISerializer& serialize, const char* UNUSED(name), SpatialSubdivision& value)
	{
		serialize.NumberFixed_Unbounded("div size", value.m_DivisionSize);
		serialize.NumberU32_Unbounded("divs w", value.m_DivisionsW);
		serialize.NumberU32_Unbounded("divs h", value.m_DivisionsH);

		size_t count = value.m_DivisionsH * value.m_DivisionsW;
		for (size_t i = 0; i < count; ++i)
			SerializeVector<SerializeU32_Unbounded>()(serialize, "subdiv items", value.m_Divisions[i].items);
	}

	void operator()(IDeserializer& serialize, const char* UNUSED(name), SpatialSubdivision& value)
	{
		serialize.NumberFixed_Unbounded("div size", value.m_DivisionSize);
		serialize.NumberU32_Unbounded("divs w", value.m_DivisionsW);
		serialize.NumberU32_Unbounded("divs h", value.m_DivisionsH);

		size_t count = value.m_DivisionsW * value.m_DivisionsH;
		value.Create(count);
		for (size_t i = 0; i < count; ++i)
			SerializeVector<SerializeU32_Unbounded>()(serialize, "subdiv items", value.m_Divisions[i].items);
	}
};

#endif // INCLUDED_SPATIAL
