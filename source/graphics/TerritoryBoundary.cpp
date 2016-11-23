/* Copyright (C) 2012 Wildfire Games.
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
#include "TerritoryBoundary.h"

#include <algorithm> // for reverse

#include "graphics/Terrain.h"
#include "simulation2/helpers/Pathfinding.h"
#include "simulation2/components/ICmpTerritoryManager.h"

std::vector<STerritoryBoundary> CTerritoryBoundaryCalculator::ComputeBoundaries(const Grid<u8>* territory)
{
	std::vector<STerritoryBoundary> boundaries;

	// Copy the territories grid so we can mess with it
	Grid<u8> grid(*territory);

	// Some constants for the border walk
	CVector2D edgeOffsets[] = {
		CVector2D(0.5f, 0.0f),
		CVector2D(1.0f, 0.5f),
		CVector2D(0.5f, 1.0f),
		CVector2D(0.0f, 0.5f)
	};

	// syntactic sugar
	const u8 TILE_BOTTOM = 0;
	const u8 TILE_RIGHT = 1;
	const u8 TILE_TOP = 2;
	const u8 TILE_LEFT = 3;

	const int CURVE_CW = -1;
	const int CURVE_CCW = 1;

	// === Find territory boundaries ===
	//
	// The territory boundaries delineate areas of tiles that belong to the same player, and that all have the same
	// connected-to-a-root-influence-entity status (see also STerritoryBoundary for a more wordy definition). Note that the grid
	// values contain bit-packed information (i.e. not just the owning player ID), so we must be careful to only compare grid
	// values using the player ID and connected flag bits. The joint mask to select these is referred to as the discriminator mask.
	//
	// The idea is to scan the (i,j)-grid going up row by row and look for tiles that have a different territory assignment from
	// the one right underneath it (or, if it's a tile on the first row, they need only have a territory assignment). These tiles
	// are necessarily edge tiles of a territory, and hence a territory boundary must pass through their bottom edge. Therefore,
	// we start tracing the outline of the territory starting from said bottom edge, and go CCW around the territory boundary.
	// Tracing continues until the starting point is reached, at which point the boundary is complete.
	//
	// While tracing a boundary, every tile in which the boundary passes through the bottom edge are marked as 'processed', so that
	// we know not to start a new run from these tiles when scanning continues (when the boundary is complete). This information
	// is maintained in the grid values themselves by means of the 'processed' bit mask (stressing the importance of using the
	// discriminator mask to compare only player ID and connected flag).
	//
	// Thus, we can identify the following conditions for starting a trace from a tile (i,j). Let g(i,j) indicate the
	// discriminator grid value at position (i,j); then the conditions are:
	//     - g(i,j) != 0; the tile must not be neutral
	//     - j=0 or g(i,j) != g(i,j-1);  the tile directly underneath it must have a different owner and/or connected flag
	//     - the tile must not already be marked as 'processed'
	//
	// Additionally, there is one more point to be made; the algorithm initially assumes it's tracing CCW around the territory.
	// If it's tracing an inner edge, however, this will actually cause it to trace in the CW direction (because inner edges curve
	// 'backwards' compared to the outer edges when starting the trace in the same direction). This turns out to actually be
	// exactly what the renderer needs to render two territory boundaries on the same edge back-to-back (instead of overlapping
	// each other).
	//
	// In either case, we keep track of the way the outline curves while we're tracing to determine whether we're going CW or CCW.
	// If at some point we ever need to revert the winding order or external code needs to know about it explicitly, then we can
	// do this by looking at a curvature value which we define to start at 0, and which is incremented by 1 for every CCW turn and
	// decremented by 1 for every CW turn. Hence, a negative multiple of 4 means a CW winding order, and a positive one means CCW.

	const int TERRITORY_DISCR_MASK = (ICmpTerritoryManager::TERRITORY_BLINKING_MASK | ICmpTerritoryManager::TERRITORY_PLAYER_MASK);

	// Try to find an assigned tile
	for (u16 j = 0; j < grid.m_H; ++j)
	{
		for (u16 i = 0; i < grid.m_W; ++i)
		{
			// saved tile state; from MSB to LSB:
			// processed bit, blinking bit, player ID
			u8 tileState = grid.get(i, j);
			u8 tileDiscr = (tileState & TERRITORY_DISCR_MASK);

			// ignore neutral tiles (note that tiles without an owner should never have the blinking bit set)
			if (!tileDiscr)
				continue;

			bool tileProcessed = ((tileState & ICmpTerritoryManager::TERRITORY_PROCESSED_MASK) != 0);
			bool tileEligible = (j == 0 || tileDiscr != (grid.get(i, j-1) & TERRITORY_DISCR_MASK));

			if (tileProcessed || !tileEligible)
				continue;

			// Found the first tile (which must be the lowest j value of any non-zero tile);
			// start at the bottom edge of it and chase anticlockwise around the border until
			// we reach the starting point again

			int curvature = 0; // +1 for every CCW 90 degree turn, -1 for every CW 90 degree turn; must be multiple of 4 at the end

			boundaries.push_back(STerritoryBoundary());
			boundaries.back().owner = (tileState & ICmpTerritoryManager::TERRITORY_PLAYER_MASK);
			boundaries.back().blinking = (tileState & ICmpTerritoryManager::TERRITORY_BLINKING_MASK) != 0;
			std::vector<CVector2D>& points = boundaries.back().points;

			u8 dir = TILE_BOTTOM;

			u8 cdir = dir;
			u16 ci = i, cj = j;

			u16 maxi = (u16)(grid.m_W-1);
			u16 maxj = (u16)(grid.m_H-1);

			// Size of a territory tile in metres
			float territoryTileSize = (Pathfinding::NAVCELL_SIZE * ICmpTerritoryManager::NAVCELLS_PER_TERRITORY_TILE).ToFloat();

			while (true)
			{
				points.push_back((CVector2D(ci, cj) + edgeOffsets[cdir]) * territoryTileSize);

				// Given that we're on an edge on a continuous boundary and aiming anticlockwise,
				// we can either carry on straight or turn left or turn right, so examine each
				// of the three possible cases (depending on initial direction):
				switch (cdir)
				{
				case TILE_BOTTOM:

					// mark tile as processed so we don't start a new run from it after this one is complete
					ENSURE(!(grid.get(ci, cj) & ICmpTerritoryManager::TERRITORY_PROCESSED_MASK));
					grid.set(ci, cj, grid.get(ci, cj) | ICmpTerritoryManager::TERRITORY_PROCESSED_MASK);

					if (ci < maxi && cj > 0 && (grid.get(ci+1, cj-1) & TERRITORY_DISCR_MASK) == tileDiscr)
					{
						++ci;
						--cj;
						cdir = TILE_LEFT;
						curvature += CURVE_CW;
					}
					else if (ci < maxi && (grid.get(ci+1, cj) & TERRITORY_DISCR_MASK) == tileDiscr)
						++ci;
					else
					{
						cdir = TILE_RIGHT;
						curvature += CURVE_CCW;
					}
					break;

				case TILE_RIGHT:
					if (ci < maxi && cj < maxj && (grid.get(ci+1, cj+1) & TERRITORY_DISCR_MASK) == tileDiscr)
					{
						++ci;
						++cj;
						cdir = TILE_BOTTOM;
						curvature += CURVE_CW;
					}
					else if (cj < maxj && (grid.get(ci, cj+1) & TERRITORY_DISCR_MASK) == tileDiscr)
						++cj;
					else
					{
						cdir = TILE_TOP;
						curvature += CURVE_CCW;
					}
					break;

				case TILE_TOP:
					if (ci > 0 && cj < maxj && (grid.get(ci-1, cj+1) & TERRITORY_DISCR_MASK) == tileDiscr)
					{
						--ci;
						++cj;
						cdir = TILE_RIGHT;
						curvature += CURVE_CW;
					}
					else if (ci > 0 && (grid.get(ci-1, cj) & TERRITORY_DISCR_MASK) == tileDiscr)
						--ci;
					else
					{
						cdir = TILE_LEFT;
						curvature += CURVE_CCW;
					}
					break;

				case TILE_LEFT:
					if (ci > 0 && cj > 0 && (grid.get(ci-1, cj-1) & TERRITORY_DISCR_MASK) == tileDiscr)
					{
						--ci;
						--cj;
						cdir = TILE_TOP;
						curvature += CURVE_CW;
					}
					else if (cj > 0 && (grid.get(ci, cj-1) & TERRITORY_DISCR_MASK) == tileDiscr)
						--cj;
					else
					{
						cdir = TILE_BOTTOM;
						curvature += CURVE_CCW;
					}
					break;
				}

				// Stop when we've reached the starting point again
				if (ci == i && cj == j && cdir == dir)
					break;
			}

			ENSURE(curvature != 0 && abs(curvature) % 4 == 0);
		}
	}

	return boundaries;
}
