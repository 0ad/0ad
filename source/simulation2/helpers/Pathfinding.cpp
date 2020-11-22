/* Copyright (C) 2018 Wildfire Games.
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

#include "precompiled.h"

#include "Pathfinding.h"

#include "graphics/Terrain.h"
#include "ps/CLogger.h"
#include "simulation2/helpers/Grid.h"
#include "simulation2/system/ParamNode.h"

namespace Pathfinding
{
	bool CheckLineMovement(entity_pos_t x0, entity_pos_t z0, entity_pos_t x1, entity_pos_t z1,
								  pass_class_t passClass, const Grid<NavcellData>& grid)
	{
		// We shouldn't allow lines between diagonally-adjacent navcells.
		// It doesn't matter whether we allow lines precisely along the edge
		// of an impassable navcell.

		// To rasterise the line:
		// If the line is (e.g.) aiming up-right, then we start at the navcell
		// containing the start point and the line must either end in that navcell
		// or else exit along the top edge or the right edge (or through the top-right corner,
		// which we'll arbitrary treat as the horizontal edge).
		// So we jump into the adjacent navcell across that edge, and continue.

		// To handle the special case of units that are stuck on impassable cells,
		// we allow them to move from an impassable to a passable cell (but not
		// vice versa).

		u16 i0, j0, i1, j1;
		NearestNavcell(x0, z0, i0, j0, grid.m_W, grid.m_H);
		NearestNavcell(x1, z1, i1, j1, grid.m_W, grid.m_H);

		// Find which direction the line heads in
		int di = (i0 < i1 ? +1 : i1 < i0 ? -1 : 0);
		int dj = (j0 < j1 ? +1 : j1 < j0 ? -1 : 0);

		u16 i = i0;
		u16 j = j0;

		bool currentlyOnImpassable = !IS_PASSABLE(grid.get(i0, j0), passClass);

		while (true)
		{
			// Make sure we are still in the limits
			ENSURE(
				   ((di > 0 && i0 <= i && i <= i1) || (di < 0 && i1 <= i && i <= i0) || (di == 0 && i == i0)) &&
				   ((dj > 0 && j0 <= j && j <= j1) || (dj < 0 && j1 <= j && j <= j0) || (dj == 0 && j == j0)));

			// Fail if we're moving onto an impassable navcell
			bool passable = IS_PASSABLE(grid.get(i, j), passClass);
			if (passable)
				currentlyOnImpassable = false;
			else if (!currentlyOnImpassable)
				return false;

			// Succeed if we're at the target
			if (i == i1 && j == j1)
				return true;

			// If we can only move horizontally/vertically, then just move in that direction
			// If we are reaching the limits, we can go straight to the end
			if (di == 0 || i == i1)
			{
				j += dj;
				continue;
			}
			else if (dj == 0 || j == j1)
			{
				i += di;
				continue;
			}

			// Otherwise we need to check which cell to move into:

			// Check whether the line intersects the horizontal (top/bottom) edge of
			// the current navcell.
			// Horizontal edge is (i, j + (dj>0?1:0)) .. (i + 1, j + (dj>0?1:0))
			// Since we already know the line is moving from this navcell into a different
			// navcell, we simply need to test that the edge's endpoints are not both on the
			// same side of the line.

			// If we are crossing exactly a vertex of the grid, we will get dota or dotb equal
			// to 0. In that case we arbitrarily choose to move of dj.
			// This only works because we handle the case (i == i1 || j == j1) beforehand.
			// Otherwise we could go outside the j limits and never reach the final navcell.

			entity_pos_t xia = entity_pos_t::FromInt(i).Multiply(Pathfinding::NAVCELL_SIZE);
			entity_pos_t xib = entity_pos_t::FromInt(i+1).Multiply(Pathfinding::NAVCELL_SIZE);
			entity_pos_t zj = entity_pos_t::FromInt(j + (dj+1)/2).Multiply(Pathfinding::NAVCELL_SIZE);

			CFixedVector2D perp = CFixedVector2D(x1 - x0, z1 - z0).Perpendicular();
			entity_pos_t dota = (CFixedVector2D(xia, zj) - CFixedVector2D(x0, z0)).Dot(perp);
			entity_pos_t dotb = (CFixedVector2D(xib, zj) - CFixedVector2D(x0, z0)).Dot(perp);

			// If the horizontal edge is fully on one side of the line, so the line doesn't
			// intersect it, we should move across the vertical edge instead
			if ((dota < entity_pos_t::Zero() && dotb < entity_pos_t::Zero()) ||
				(dota > entity_pos_t::Zero() && dotb > entity_pos_t::Zero()))
				i += di;
			else
				j += dj;
		}
	}
}

PathfinderPassability::PathfinderPassability(pass_class_t mask, const CParamNode& node) : m_Mask(mask)
{
	if (node.GetChild("MinWaterDepth").IsOk())
		m_MinDepth = node.GetChild("MinWaterDepth").ToFixed();
	else
		m_MinDepth = std::numeric_limits<fixed>::min();

	if (node.GetChild("MaxWaterDepth").IsOk())
		m_MaxDepth = node.GetChild("MaxWaterDepth").ToFixed();
	else
		m_MaxDepth = std::numeric_limits<fixed>::max();

	if (node.GetChild("MaxTerrainSlope").IsOk())
		m_MaxSlope = node.GetChild("MaxTerrainSlope").ToFixed();
	else
		m_MaxSlope = std::numeric_limits<fixed>::max();

	if (node.GetChild("MinShoreDistance").IsOk())
		m_MinShore = node.GetChild("MinShoreDistance").ToFixed();
	else
		m_MinShore = std::numeric_limits<fixed>::min();

	if (node.GetChild("MaxShoreDistance").IsOk())
		m_MaxShore = node.GetChild("MaxShoreDistance").ToFixed();
	else
		m_MaxShore = std::numeric_limits<fixed>::max();

	if (node.GetChild("Clearance").IsOk())
	{
		m_Clearance = node.GetChild("Clearance").ToFixed();

		/* According to Philip who designed the original doc (in docs/pathfinder.pdf),
		 * clearance should usually be integer to ensure consistent behavior when rasterizing
		 * the passability map.
		 * This seems doubtful to me and my pathfinder fix makes having a clearance of 0.8 quite convenient
		 * so I comment out this check, but leave it here for the purpose of documentation should a bug arise.

		 if (!(m_Clearance % Pathfinding::NAVCELL_SIZE).IsZero())
		 {
		 // If clearance isn't an integer number of navcells then we'll
		 // probably get weird behaviour when expanding the navcell grid
		 // by clearance, vs expanding static obstructions by clearance
		 LOGWARNING("Pathfinder passability class has clearance %f, should be multiple of %f",
		 m_Clearance.ToFloat(), Pathfinding::NAVCELL_SIZE.ToFloat());
		 }*/
	}
	else
		m_Clearance = fixed::Zero();

	if (node.GetChild("Obstructions").IsOk())
	{
		std::wstring obstructions = node.GetChild("Obstructions").ToString();
		if (obstructions == L"none")
			m_Obstructions = NONE;
		else if (obstructions == L"pathfinding")
			m_Obstructions = PATHFINDING;
		else if (obstructions == L"foundation")
			m_Obstructions = FOUNDATION;
		else
		{
			LOGERROR("Invalid value for Obstructions in pathfinder.xml for pass class %d", mask);
			m_Obstructions = NONE;
		}
	}
	else
		m_Obstructions = NONE;
}
