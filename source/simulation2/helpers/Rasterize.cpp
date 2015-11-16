/* Copyright (C) 2015 Wildfire Games.
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

#include "Rasterize.h"

#include "simulation2/helpers/Geometry.h"

void SimRasterize::RasterizeRectWithClearance(Spans& spans,
	const ICmpObstructionManager::ObstructionSquare& shape,
	entity_pos_t clearance, entity_pos_t cellSize)
{
	// A long-standing issue with the pathfinding has been that the long-range one
	// uses a AA navcell grid, while the short-range uses an accurate vector representation.
	// This means we could get paths accepted by one but not both pathfinders.
	// Since the new pathfinder, the short-range pathfinder's representation was usually
	// encompassing the rasterisation of the long-range one for a building.
	// This means that we could never get quite as close as the long-range pathfinder wanted.
	// This could mean units tried going through impassable paths.
	// To fix this, we need to make sure that the short-range pathfinder is always mostly
	// included in the rasterisation. The easiest way is to rasterise more, thus this variable
	// Since this is a very complicated subject, check out logs on 31/10/2015 for more detailled info.
	// or ask wraitii about it.
	// If the short-range pathfinder is sufficiently changed, this could become unnecessary and thus removed.
	// A side effect is that the basic clearance has been set to 0.8, so removing this constant should be done
	// in parallel with setting clearance back to 1 for the default passability class (though this isn't strictly necessary).
	// Also: the code detecting foundation obstruction in CcmpObstructionManager had to be changed similarly.
	entity_pos_t rasterClearance = clearance + Pathfinding::CLEARANCE_EXTENSION_RADIUS;
	
	// Get the bounds of cells that might possibly be within the shape
	// (We'll then test each of those cells more precisely)
	CFixedVector2D shapeHalfSize(CFixedVector2D(shape.hw, shape.hh));
	CFixedVector2D halfSize(shape.hw + rasterClearance, shape.hh + rasterClearance);
	CFixedVector2D halfBound = Geometry::GetHalfBoundingBox(shape.u, shape.v, halfSize);
	i16 i0 = ((shape.x - halfBound.X) / cellSize).ToInt_RoundToNegInfinity();
	i16 j0 = ((shape.z - halfBound.Y) / cellSize).ToInt_RoundToNegInfinity();
	i16 i1 = ((shape.x + halfBound.X) / cellSize).ToInt_RoundToInfinity();
	i16 j1 = ((shape.z + halfBound.Y) / cellSize).ToInt_RoundToInfinity();

	if (j1 <= j0)
		return; // empty bounds - this shouldn't happen


	rasterClearance = rasterClearance.Multiply(rasterClearance);

	spans.reserve(j1 - j0);
	
	for (i16 j = j0; j < j1; ++j)
	{
		// Find the min/max range of cells that are strictly inside the square+rasterClearance.
		// (Since the square+rasterClearance is a convex shape, we can just test each
		// corner of each cell is inside the shape.)
		// When looping on i, if the previous cell was inside, no need to check again the left corners.
		// and we can stop the loop when exiting the shape.
		// Futhermore if one of the right corners of a cell is outside, no need to check the following cell
		i16 spanI0 = std::numeric_limits<i16>::max();
		i16 spanI1 = std::numeric_limits<i16>::min();
		bool previousInside = false;
		bool skipNextCell = false;
		for (i16 i = i0; i < i1; ++i)
		{
			if (skipNextCell)
			{
				skipNextCell = false;
				continue;
			}

			if (Geometry::DistanceToSquareSquared(CFixedVector2D(cellSize*(i+1)-shape.x, cellSize*j-shape.z),
									shape.u, shape.v, shapeHalfSize, true) > rasterClearance)
			{
				if (previousInside)
					break;
				skipNextCell = true;
				continue;
			}

			if (Geometry::DistanceToSquareSquared(CFixedVector2D(cellSize*(i+1)-shape.x, cellSize*(j+1)-shape.z),
									shape.u, shape.v, shapeHalfSize, true) > rasterClearance)
			{
				if (previousInside)
					break;
				skipNextCell = true;
				continue;
			}

			if (!previousInside)
			{
				if (Geometry::DistanceToSquareSquared(CFixedVector2D(cellSize*i-shape.x, cellSize*j-shape.z),
									shape.u, shape.v, shapeHalfSize, true) > rasterClearance)
					continue;

				if (Geometry::DistanceToSquareSquared(CFixedVector2D(cellSize*i-shape.x, cellSize*(j+1)-shape.z),
									shape.u, shape.v, shapeHalfSize, true) > rasterClearance)
					continue;

				previousInside = true;
				spanI0 = i;
			}

			spanI1 = i+1;
		}

		// Add non-empty spans onto the list
		if (spanI0 < spanI1)
		{
			Span span = { spanI0, spanI1, j };
			spans.push_back(span);
		}
	}
}
